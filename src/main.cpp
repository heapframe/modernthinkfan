#include <QCoreApplication>
#include <iostream>
#include <unistd.h>
#include <ctime>
#include <thread>
#include <chrono>
#include "args.hxx"
#include "FanModule.h"
#include <filesystem>

int main(int argc, char** argv)
{
    args::ArgumentParser parser("Modern Thinkpad Fan Control", "Disclamer:\nSet fan speed to auto to hand control back to the EC.\n\n\
Preventing the EC from controlling the fan can result in hardware damage during overheating");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

    args::Flag readFan(parser, "read", "Read fan speed", {'r', "read"});
    args::Flag streamFan(parser, "stream", "Repeatedly read fan speed", {"stream"});
    args::ValueFlag<std::string> setFan(parser, "preset", "Set Fan Speed [slow/med/high/full/auto]", {'s', "set"});
    args::Flag noFan(parser, "nofan", "Stops the EC from interfering with the fan speed/sets fanspeed to 0 (experimental)", {'n', "nofan"});

    args::Flag iDontCare(parser, "iDontCare", "Ignore EC read failures & Try set fan speed anyways on unsupported models", {"iDontCare"});

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

    const std::array<std::string, 3> requiredFiles = {
        "fan_database.sqlite",
        "libsal.so",
        "module_fan.so"
    };

    for (const auto& file : requiredFiles) {
        if (!std::filesystem::exists(file)) {
            std::cerr << "Missing required file: " << file << std::endl;
            return 1;
        }
    }

    if (geteuid() != 0) {
        std::cout << "Root permissions are required as this program works with the EC." << std::endl;
        exit(1);
    }

    QCoreApplication app(argc, argv); //to resolve fan_database.sqlite as module_fan expects a qt environment

    FanModule fan;
    if (!fan.ok()) {
        std::cerr << fan.error() << std::endl;
        return 1;
    }

    if (noFan) {
        FanControlResult result = fan.noFan();
        if (result != FanControlResult::Success) {
            std::cerr << "Error: " << result  << std::endl;
        }
    }

    if (readFan) {
        unsigned short rpm = 0;
        FanReadResult result = fan.getRotationSpeed(rpm);

        if (result != FanReadResult::Success){
            std::cout << "GetRotationSpeed result: " << result  << std::endl;
            
        }else {
            std::cout << "RPM: " << rpm << std::endl;
        }
    }

    if (setFan) {
        if (!fan.isControllable() && !iDontCare) {
            std::cerr << "Fan control is not supported on this model." << std::endl;
            return 1;
        }

        std::string setting = args::get(setFan);

        if (setting == "slow") {
            fan.setSlow();
        }else if (setting == "med") {
            fan.setMedium();
        }else if (setting == "high") {
            fan.setHigh();
        }else if (setting == "full") {
            fan.setFull();
        }else if (setting == "auto") {
            fan.setAuto();
        }else {
            std::cerr << "Unknown fan setting: " << setting << std::endl;
            return 1;
        }
    }

    if (streamFan) {
        unsigned short rpm = 0;
        while (true) {
            unsigned short rpm = 0;

            const auto now = std::chrono::system_clock::now();
            const auto timestamp =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now.time_since_epoch()
                ).count();
            
            switch (fan.getRotationSpeed(rpm)) {
                case FanReadResult::Success:
                    std::cout << timestamp << " | RPM: " << rpm << std::endl;
                    break;

                case FanReadResult::Unavailable:
                    std::cout << timestamp << " | " << FanReadResult::Unavailable << std::endl;
                    break;

                case FanReadResult::Error:
                    std::cerr << timestamp << " | " << FanReadResult::Error << std::endl;
                    if (!iDontCare)
                        return 1;
                    break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
