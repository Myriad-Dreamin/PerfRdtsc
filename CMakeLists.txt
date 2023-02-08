

cmake_minimum_required(VERSION 3.10)

project(perfRdtsc
        VERSION 1.0.0
        DESCRIPTION "Use tsc on x86 Linux / Linux Kernel / Windows"
        LANGUAGES CXX)

add_library(perfRdtsc INTERFACE)
target_include_directories(perfRdtsc INTERFACE include)
