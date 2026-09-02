cmake_minimum_required(VERSION 3.16)

project(lenovofan LANGUAGES CXX)

add_executable(lenovofan main.cpp)
target_compile_features(lenovofan PRIVATE cxx_std_17)
