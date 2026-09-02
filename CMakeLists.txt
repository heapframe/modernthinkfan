cmake_minimum_required(VERSION 3.16)

project(lenovofan LANGUAGES CXX)

find_package(Qt5 REQUIRED COMPONENTS Core)

add_executable(lenovofan main.cpp)
target_compile_features(lenovofan PRIVATE cxx_std_17)

find_package(Qt5 REQUIRED COMPONENTS Core)

target_link_libraries(lenovofan PRIVATE
    Qt5::Core
    dl
)