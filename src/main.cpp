#include <QCoreApplication>
#include <iostream>
#include <unistd.h>
#include "args.hxx"
#include "FanModule.h"

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

    FanModule fan;
    if (!fan.ok()) {
        std::cerr << fan.error() << std::endl;
        return 1;
    }

    if (readFan) {
        unsigned short rpm = 0;
        int result = fan.getRotationSpeed(rpm);

        std::cout << "Fan controllable: " << fan.isControllable() << std::endl;
        std::cout << "GetRotationSpeed result: " << result  << std::endl;
        std::cout << "RPM: " << rpm << std::endl;
    }

    if (setFan) {
        if (!fan.isControllable()) {
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
}
