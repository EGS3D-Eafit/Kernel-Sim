cmake_minimum_required(VERSION 3.31)
project(Kernel-Sim)

set(CMAKE_CXX_STANDARD 20)

add_executable(Kernel-Sim
        cli/main.cpp
        modules/cpu/cpu.cpp
        modules/cpu/scheduler.cpp
        modules/cpu/pcb.cpp
        modules/disk/disk.cpp
)
