#pragma once

#include <string>

using Fn = unsigned int (*)();
using Constructor = void (*)(void*);
using GetRotationSpeed = int (*)(void*, unsigned short*);
using SetRotation = int (*)(void*);
using SetRotationVerified = bool (*)(void*);
using WriteFanControlBit4 = int (*)(void*, unsigned char);
using ReadFanControlStatus = int (*)(void*, unsigned char*);
using IsType3SupportedModel = int (*)();
using WriteToEC = int (*)(void*, unsigned short, unsigned char);

enum class FanReadResult {
    Error,
    Unavailable,
    Success
};
std::ostream& operator<<(std::ostream& os, FanReadResult result);

enum class FanControlResult {
    ReadFail,
    WriteFail,
    Success,
    WrongDevice
};
std::ostream& operator<<(std::ostream& os, FanControlResult result);


// RAII wrapper around Lenovo's module_fan.so.
// Loads the library, resolves the mangled symbols and manages the
// lifetime of an EmbeddedControllerComponentLinux instance.
class FanModule
{
public:
    FanModule();
    ~FanModule();

    FanModule(const FanModule&) = delete;
    FanModule& operator=(const FanModule&) = delete;

    bool ok() const { return m_ok; }
    const std::string& error() const { return m_error; }

    bool isControllable() const;
    FanReadResult getRotationSpeed(unsigned short& rpm) const;

    int setSlow() const;
    int setMedium() const;
    int setHigh() const;
    bool setFull() const;
    bool setAuto() const;
    FanControlResult noFan() const;
    //bool FanModule::isType3SupportedModel() const;
    int writeEC(unsigned short address, unsigned char value) const;

private:
    void* m_lib = nullptr;
    bool m_ok = false;
    std::string m_error;

    Fn isControllableFn = nullptr;
    Constructor ctorFn = nullptr;
    Constructor dtorFn = nullptr;
    GetRotationSpeed getRPMFn = nullptr;
    SetRotation setSlowFn = nullptr;
    SetRotation setMediumFn = nullptr;
    SetRotation setHighFn = nullptr;
    SetRotationVerified setFullFn = nullptr;
    SetRotationVerified setAutoFn = nullptr;
    WriteFanControlBit4 writeFanControlBit4Fn = nullptr;
    ReadFanControlStatus readFanControlStatusFn = nullptr;
    IsType3SupportedModel isType3SupportedModelFn = nullptr;
    WriteToEC writeToECFn = nullptr;

    // Storage for the EmbeddedControllerComponentLinux instance.
    // Size/alignment taken from the module's actual object layout.
    alignas(8) unsigned char m_object[0x10];

    // The EC functions take a mutable this pointer; const methods only
    // promise not to change FanModule's own loading state.
    void* object() const { return const_cast<unsigned char*>(m_object); }
};
