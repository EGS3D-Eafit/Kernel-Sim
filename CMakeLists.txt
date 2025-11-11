cmake_minimum_required(VERSION 3.15)
project(MiniKernelSim LANGUAGES CXX)

# ====== Configuración general ======
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ====== Archivos fuente ======
set(SOURCES
    cli/main.cpp

    modules/cpu/process.cpp
    modules/cpu/scheduler.cpp

    modules/mem/paging.cpp
    modules/mem/allocation.cpp
    modules/mem/heap.cpp              # <-- nuevo (BuddyAllocator)

    modules/disk/scheduling.cpp

    modules/io/device_queue.cpp     # <-- quítalo si NO existe en tu repo
    modules/io/printer.cpp            # <-- nueva impresora
    modules/sync/producer_consumer.cpp
    modules/sync/philosophers.cpp
)

# ====== Ejecutable principal ======
add_executable(kernel_cli ${SOURCES})

# ====== Includes del target ======
target_include_directories(kernel_cli PRIVATE
    ${CMAKE_SOURCE_DIR}/cli
    ${CMAKE_SOURCE_DIR}/modules/common     # <-- nuevo (common.hpp movido aquí)
    ${CMAKE_SOURCE_DIR}/modules/cpu
    ${CMAKE_SOURCE_DIR}/modules/mem
    ${CMAKE_SOURCE_DIR}/modules/disk
    ${CMAKE_SOURCE_DIR}/modules/io
    ${CMAKE_SOURCE_DIR}/modules/sync       # <-- nuevo (producer/philosophers)
)

# ====== Threads portables ======
find_package(Threads REQUIRED)
target_link_libraries(kernel_cli PRIVATE Threads::Threads)

# ====== Config de salida ======
set_target_properties(kernel_cli PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# ====== Mensajes ======
message(STATUS "KernelSim configurado correctamente")
message(STATUS "Compila con:   cmake -S . -B build && cmake --build build")
message(STATUS "Ejecuta con:   ./build/kernel_cli")
