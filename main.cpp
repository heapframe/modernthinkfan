#include <QCoreApplication>
#include <iostream>
#include <dlfcn.h>
#include <unistd.h>

using Fn = unsigned int (*)();
using Constructor = void (*)(void*);
using GetRotationSpeed = int (*)(void*, unsigned short*);
using SetRotation = int (*)(void*);

int main(int argc, char** argv)
{
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

    unsigned short rpm = 0;
    int result = getRPM(object, &rpm);

    std::cout << "Fan controllable: " << isControllable() << '\n';
    std::cout << "GetRotationSpeed result: " << result << '\n';
    std::cout << "RPM: " << rpm << '\n';

    dlclose(lib);
}