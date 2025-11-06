cmake_minimum_required(VERSION 3.31)
project(Kernel-Sim)
set(CMAKE_CXX_STANDARD 20)

add_executable(Kernel-Sim
    cli/main.cpp
    modules/cpu/cpu.cpp
    modules/cpu/scheduler.cpp
    modules/cpu/pcb.cpp
    modules/disk/disk.cpp
    modules/mem/paging.cpp
    modules/disk_sched/disk_sched.cpp
    modules/io/device.cpp
    modules/sync/philosophers.cpp
)

# Heap module
set(HEAP_SOURCES modules/heap/buddy.cpp)
target_sources(kernel_sim PRIVATE ${HEAP_SOURCES})
