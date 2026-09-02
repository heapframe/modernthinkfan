#include <QCoreApplication>
#include <iostream>
#include <dlfcn.h>
#include <unistd.h>
#include <args.hxx>

using Fn = unsigned int (*)();
using Constructor = void (*)(void*);
using GetRotationSpeed = int (*)(void*, unsigned short*);
using SetRotation = int (*)(void*);
using SetRotationVerified = bool (*)(void*);

int main(int argc, char** argv)
{
    args::ArgumentParser parser("Modern Thinkpad Fan Control");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

    args::Flag readFan(parser, "read", "Read fan speed", {'r', "read"});
    args::ValueFlag<std::string> setFan(parser, "preset", "Set Fan Speed [slow/med/high/full/auto]", {'s', "set"});

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help)
    {
        std::cout << parser;
        return 0;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }
    catch (args::ValidationError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }


    if (geteuid() != 0) {
        std::cout << "Root permissions are required as this program works with the EC." << std::endl;
        exit(1);
    }

    QCoreApplication app(argc, argv); //to resolve fan_database.sqlite as module_fan expects a qt environment

    void* lib = dlopen("./module_fan.so", RTLD_NOW);

    if (!lib) {
        std::cerr << dlerror() << std::endl;
        return 1;
    }

    /*
    The procedure to get these mangled functions to hook is:
    1. Find what you want in ghidra
    2. Then search for it with:
        nm -D module_fan.so | grep 'FunctionYouWannaHook'
    3. Add it in here (though in the future this should be split up into multiple files)
    
    These are very dependent on the exact version of module_fan.so, although regenerating them shouldn't be too hard.
    The below code uses these versions (sha256):
    9ceea6f128dfe8208b6df00f21b27e7dc5b75bb6e66d6fa443e78f7f7e2b334b module_fan.so
    ab2f3b8d9c187f4ad3630af080e53326373c27153506cf71de3b08d3bd55c03c  libsal.so

    The exact libsal probably isn't too important but its a dependency of module_fan
    */

    auto isControllable =
        reinterpret_cast<Fn>(
            dlsym(lib, "_ZN19FanComponentManager27IsFanSpeedControllableModelEv")
        );

    auto ctor = reinterpret_cast<Constructor>(
        dlsym(lib, "_ZN32EmbeddedControllerComponentLinuxC1Ev")
    );

    auto dtor = reinterpret_cast<Constructor>(
        dlsym(lib, "_ZN32EmbeddedControllerComponentLinuxD1Ev")
    );

    auto getRPM = reinterpret_cast<GetRotationSpeed>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent16GetRotationSpeedEPt")
    );

    auto setSlow = reinterpret_cast<SetRotation>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent15SetSlowRotationEv")
    );

    auto setMedium = reinterpret_cast<SetRotation>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent17SetMediumRotationEv")
    );

    auto setHigh = reinterpret_cast<SetRotation>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent15SetHighRotationEv")
    );

    auto setFull = reinterpret_cast<SetRotationVerified>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent15SetFullRotationEv")
    );

    auto setAuto = reinterpret_cast<SetRotationVerified>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent15SetAutoRotationEv")
    );

    if (!isControllable) {
        std::cerr << "dlsym: " << dlerror() << std::endl;
        return 1;
    }


    if (!ctor || !getRPM || !setFull) {
        std::cerr << "dlsym failed: " << dlerror() << std::endl;
        return 1;
    }

    alignas(8) unsigned char object[0x10];

    ctor(object);

    if (readFan) {
        unsigned short rpm = 0;
        int result = getRPM(object, &rpm);

        std::cout << "Fan controllable: " << isControllable() << std::endl;
        std::cout << "GetRotationSpeed result: " << result  << std::endl;
        std::cout << "RPM: " << rpm << std::endl;
    }
    
    if (setFan) {
        if (!isControllable()) {
            std::cerr << "Fan control is not supported on this model." << std::endl;
            return 1;
        }

        std::string setting = args::get(setFan);

        if (setting == "slow") {
            setSlow(object);
        }else if (setting == "med") {
            setMedium(object);
        }else if (setting == "high") {
            setHigh(object);
        }else if (setting == "full") {
            setFull(object);
        }else if (setting == "auto") {
            setAuto(object);
        }else {
            std::cerr << "Unknown fan setting: " << setting << std::endl;
            return 1;
        }
    }

    dtor(object);
    dlclose(lib);
}