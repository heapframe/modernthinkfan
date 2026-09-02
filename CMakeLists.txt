cmake_minimum_required(VERSION 3.16)

project(modernthinkfan LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Core)

add_executable(modernthinkfan
    src/main.cpp
    src/FanModule.cpp
)
target_compile_features(modernthinkfan PRIVATE cxx_std_17)

target_include_directories(modernthinkfan PRIVATE
    third_party
)

target_link_libraries(modernthinkfan PRIVATE
    Qt5::Core
    dl
)
