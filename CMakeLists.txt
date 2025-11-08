cmake_minimum_required(VERSION 3.15)
project(MiniKernelSim LANGUAGES CXX)

# ====== Configuración general ======
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Directorios de módulos
include_directories(
    ${CMAKE_SOURCE_DIR}/modules/cpu
    ${CMAKE_SOURCE_DIR}/modules/mem
    ${CMAKE_SOURCE_DIR}/modules/disk
    ${CMAKE_SOURCE_DIR}/modules/io
    ${CMAKE_SOURCE_DIR}/cli
)

# ====== Archivos fuente ======
set(SOURCES
    cli/main.cpp

    modules/cpu/process.cpp
    modules/cpu/scheduler.cpp

    modules/mem/paging.cpp
    modules/mem/allocation.cpp

    modules/disk/scheduling.cpp

    modules/io/device_queue.cpp
    modules/io/printer.cpp
)

# ====== Ejecutable principal ======
add_executable(kernel_cli ${SOURCES})

# ====== Configuración de salida ======
set_target_properties(kernel_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# ====== Mensaje informativo ======
message(STATUS "MiniKernelSim configurado correctamente")
message(STATUS "Compila con:   cmake -S . -B build && cmake --build build")
message(STATUS "Ejecuta con:   ./build/kernel_cli")
