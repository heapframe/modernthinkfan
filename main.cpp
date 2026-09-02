#include <QCoreApplication>
#include <iostream>
#include <dlfcn.h>
#include <unistd.h>
#include <args.hxx>

using Fn = unsigned int (*)();
using Constructor = void (*)(void*);
using GetRotationSpeed = int (*)(void*, unsigned short*);
using SetRotation = int (*)(void*);

int main(int argc, char** argv)
{
    args::ArgumentParser parser("Modern Thinkpad Fan Control");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

    args::Flag readFan(parser, "read", "Read fan speed", {'r', "read"});
    args::ValueFlag<std::string> setFan(parser, "preset", "Set Fan Speed [high/full/auto]", {'s', "set"});

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
        std::cerr << dlerror() << '\n';
        return 1;
    }

    /*
    The procedure to get these mangled functions to hook is:
    1. Find what you want in ghidra
    2. Then search for it with:
        nm -D module_fan.so | grep 'FunctionYouWannaHook'
    3. Add it in here (though in the future this should be split up into multiple files)
    */

    auto isControllable =
        reinterpret_cast<Fn>(
            dlsym(lib, "_ZN19FanComponentManager27IsFanSpeedControllableModelEv")
        );

    auto ctor = reinterpret_cast<Constructor>(
        dlsym(lib, "_ZN32EmbeddedControllerComponentLinuxC1Ev")
    );

    auto getRPM = reinterpret_cast<GetRotationSpeed>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent16GetRotationSpeedEPt")
    );

    auto full = reinterpret_cast<SetRotation>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent15SetFullRotationEv")
    );

    auto setHigh = reinterpret_cast<SetRotation>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent15SetHighRotationEv")
    );

    auto setFull = reinterpret_cast<bool (*)(void*)>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent15SetFullRotationEv")
    );

    auto setAuto = reinterpret_cast<bool (*)(void*)>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent15SetAutoRotationEv")
    );

    if (!isControllable) {
        std::cerr << "dlsym: " << dlerror() << '\n';
        return 1;
    }


    if (!ctor || !getRPM || !full) {
        std::cerr << "dlsym failed: " << dlerror() << '\n';
        return 1;
    }

    alignas(8) unsigned char object[0x10];

    ctor(object);

    if (readFan) {
        unsigned short rpm = 0;
        int result = getRPM(object, &rpm);

        std::cout << "Fan controllable: " << isControllable() << '\n';
        std::cout << "GetRotationSpeed result: " << result << '\n';
        std::cout << "RPM: " << rpm << '\n';
    }
    if (setFan) {
        std::string setting = args::get(setFan);

        if (setting == "high") {
            setHigh(object);
        }else if (setting == "full") {
            setFull(object);
        } else {
            std::cout << "setting to auto" << std::endl;
            setAuto(object);
        }
    }

    dlclose(lib);
}