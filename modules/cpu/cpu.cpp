#include "cpu.h"
#include <iostream>

CPU::CPU(int q) : quantum(q) {}

void CPU::agregarProceso(const PCB& proceso) {
    scheduler.agregarProceso(new PCB(proceso)); // se pasa una copia dinámica
}

void CPU::ejecutarRoundRobin() {
    std::cout << "=== Iniciando ejecución Round Robin ===" << std::endl;
    scheduler.ejecutar(quantum);
    std::cout << "=== Ejecución finalizada ===" << std::endl;
}
