#include <iostream>
#include <dlfcn.h>

using Constructor = void (*)(void*);
using GetRotationSpeed = int (*)(void*, unsigned short*);
using SetRotation = int (*)(void*);

int main()
{
    void* lib = dlopen("./module_fan.so", RTLD_NOW);

    if (!lib) {
        std::cerr << dlerror() << '\n';
        return 1;
    }

    auto ctor = reinterpret_cast<Constructor>(
        dlsym(lib, "_ZN32EmbeddedControllerComponentLinuxC1Ev")
    );

    auto getRPM = reinterpret_cast<GetRotationSpeed>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent16GetRotationSpeedEPt")
    );

    auto full = reinterpret_cast<SetRotation>(
        dlsym(lib, "_ZN27EmbeddedControllerComponent15SetFullRotationEv")
    );

    if (!ctor || !getRPM || !full) {
        std::cerr << "dlsym failed: " << dlerror() << '\n';
        return 1;
    }

    alignas(8) unsigned char object[0x10];

    ctor(object);

    unsigned short rpm = 0;
    int result = getRPM(object, &rpm);

    std::cout << "GetRotationSpeed result: " << result << '\n';
    std::cout << "RPM: " << rpm << '\n';

    dlclose(lib);
}