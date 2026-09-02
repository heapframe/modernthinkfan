cmake_minimum_required(VERSION 3.16)

project(modernthinkfan LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Core)

add_executable(modernthinkfan main.cpp)
target_compile_features(modernthinkfan PRIVATE cxx_std_17)

find_package(Qt5 REQUIRED COMPONENTS Core)

target_link_libraries(modernthinkfan PRIVATE
    Qt5::Core
    dl
)