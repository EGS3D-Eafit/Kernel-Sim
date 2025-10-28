cmake_minimum_required(VERSION 3.31)
project(Kernel_Sim)

set(CMAKE_CXX_STANDARD 14)

#include_directories(cli)
include_directories(modules)
include_directories(modules/cpu)
include_directories(modules/disk)
include_directories(modules/io)
include_directories(modules/mem)
#include_directories(tests)

add_executable(Kernel_Sim
        cli/main.cpp) #Si necesitamos otros por alguna razon, solo quiten el parentesis y listo
        #modules/cpu/cpu.cpp
        #modules/cpu/cpu.h
        #modules/disk/disk.cpp
        #modules/disk/disk.h
        #modules/io/io.cpp
        #modules/io/io.h
        #modules/mem/mem.cpp
        #modules/mem/mem.h
        #tests/test_cpu.cpp
        #tests/test_mem.cpp)
