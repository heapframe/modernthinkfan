#include "FanModule.h"
#include <ostream>
#include <dlfcn.h>

namespace
{
/*
The procedure to get these mangled functions to hook is:
1. Find what you want in ghidra
2. Then search for it with:
    nm -D module_fan.so | grep 'FunctionYouWannaHook'
3. Add it in here

These are very dependent on the exact version of module_fan.so, although
regenerating them shouldn't be too hard.
The below code uses these versions (sha256):
9ceea6f128dfe8208b6df00f21b27e7dc5b75bb6e66d6fa443e78f7f7e2b334b module_fan.so
ab2f3b8d9c187f4ad3630af080e53326373c27153506cf71de3b08d3bd55c03c libsal.so

The exact libsal probably isn't too important but its a dependency of module_fan.
*/
constexpr const char* kModulePath = "./module_fan.so";
}

FanModule::FanModule()
{
    m_lib = dlopen(kModulePath, RTLD_NOW);
    if (!m_lib) {
        m_error = dlerror();
        return;
    }

    isControllableFn = reinterpret_cast<Fn>(
        dlsym(m_lib, "_ZN19FanComponentManager27IsFanSpeedControllableModelEv"));

    ctorFn = reinterpret_cast<Constructor>(
        dlsym(m_lib, "_ZN32EmbeddedControllerComponentLinuxC1Ev"));

    dtorFn = reinterpret_cast<Constructor>(
        dlsym(m_lib, "_ZN32EmbeddedControllerComponentLinuxD1Ev"));

    getRPMFn = reinterpret_cast<GetRotationSpeed>(
        dlsym(m_lib, "_ZN27EmbeddedControllerComponent16GetRotationSpeedEPt"));

    setSlowFn = reinterpret_cast<SetRotation>(
        dlsym(m_lib, "_ZN27EmbeddedControllerComponent15SetSlowRotationEv"));

    setMediumFn = reinterpret_cast<SetRotation>(
        dlsym(m_lib, "_ZN27EmbeddedControllerComponent17SetMediumRotationEv"));

    setHighFn = reinterpret_cast<SetRotation>(
        dlsym(m_lib, "_ZN27EmbeddedControllerComponent15SetHighRotationEv"));

    setFullFn = reinterpret_cast<SetRotationVerified>(
        dlsym(m_lib, "_ZN27EmbeddedControllerComponent15SetFullRotationEv"));

    setAutoFn = reinterpret_cast<SetRotationVerified>(
        dlsym(m_lib, "_ZN27EmbeddedControllerComponent15SetAutoRotationEv"));
    
    writeFanControlBit4Fn = reinterpret_cast<WriteFanControlBit4>(
        dlsym(m_lib, "_ZN27EmbeddedControllerComponent19WriteFanControlBit4Eh"));

    readFanControlStatusFn = reinterpret_cast<ReadFanControlStatus>(
        dlsym(m_lib, "_ZN27EmbeddedControllerComponent20ReadFanControlStatusERh"));

    isType3SupportedModelFn = reinterpret_cast<IsType3SupportedModel>(
        dlsym(m_lib, "_ZN19FanComponentManager21IsType3SupportedModelEv"));

    writeToECFn = reinterpret_cast<WriteToEC>(
        dlsym(m_lib, "_ZN27EmbeddedControllerComponent9WriteToECEth"));

    if (!isControllableFn || !ctorFn || !dtorFn || !getRPMFn ||
        !setSlowFn || !setMediumFn || !setHighFn ||
        !setFullFn || !setAutoFn || !writeFanControlBit4Fn) {
        const char* err = dlerror();
        m_error = err ? err : "dlsym: unknown symbol resolution failure";
        dlclose(m_lib);
        m_lib = nullptr;
        return;
    }

    ctorFn(m_object);
    m_ok = true;
}

FanModule::~FanModule()
{
    if (m_ok) {
        dtorFn(m_object);
    }
    if (m_lib) {
        dlclose(m_lib);
    }
}

bool FanModule::isControllable() const
{
    return isControllableFn() != 0;
}

FanReadResult FanModule::getRotationSpeed(unsigned short& rpm) const
{
    rpm = 0;
    const int result = getRPMFn(object(), &rpm);
    switch (result) {
        case 0:
            return FanReadResult::Error;

        case 1:
            return FanReadResult::Unavailable;

        case 2:
            return FanReadResult::Success;

        default:
            return FanReadResult::Error;
    }
}

std::ostream& operator<<(std::ostream& os, FanReadResult result)
{
    switch (result) {
        case FanReadResult::Error:
            return os << "EC read failure";

        case FanReadResult::Unavailable:
            return os << "RPM unavailable";

        case FanReadResult::Success:
            return os << "Success";
    }

    return os << "Unknown";
}

int FanModule::setSlow() const
{
    return setSlowFn(object());
}

int FanModule::setMedium() const
{
    return setMediumFn(object());
}

int FanModule::setHigh() const
{
    return setHighFn(object());
}

bool FanModule::setFull() const
{
    return setFullFn(object());
}

bool FanModule::setAuto() const
{
    return setAutoFn(object());
}

/*
bool FanModule::isType3SupportedModel() const 
{
    return (isType3SupportedModelFn());
}
*/

FanControlResult FanModule::noFan() const
{
    if (!isType3SupportedModelFn()) {
        return writeEC(0x2F, 0x00) //figured this out through guesswork, cant verifiy if disabling the fan is really ALL it does.
            ? FanControlResult::Success
            : FanControlResult::WriteFail;
    }

    unsigned char status = 0;

    if (!readFanControlStatusFn(object(), &status))
        return FanControlResult::ReadFail;

    return writeFanControlBit4Fn(object(), status | 0x10)
        ? FanControlResult::Success
        : FanControlResult::WriteFail;
}

std::ostream& operator<<(std::ostream& os, FanControlResult result)
{
    switch (result) {
        case FanControlResult::ReadFail:
            return os << "EC read failure";

        case FanControlResult::WriteFail:
            return os << "Write failure";

        case FanControlResult::Success:
            return os << "Success";
    }

    return os << "Unknown";
}

int FanModule::writeEC(unsigned short address, unsigned char value) const
{
    return writeToECFn(object(), address, value);
}