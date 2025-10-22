#include "cpu.h"
#include <iostream>
#include <thread>
#include <chrono>

void CPU::ejecutarProceso(const std::string& nombre) {
    std::cout << "Ejecutando proceso: " << nombre << "...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Proceso " << nombre << " finalizado.\n";
}
