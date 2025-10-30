#include "cpu.h"
#include "scheduler.h"
#include "pcb.h"
#include <iostream>

CPU::CPU(int q) : quantum(q) {}

void CPU::add_process(PCB* pcb) {
    scheduler.agregarProceso(pcb);
}

void CPU::ejecutarRoundRobin() {
    std::cout << "=== Iniciando ejecución Round Robin ===" << std::endl;
    scheduler.ejecutar(quantum);
    std::cout << "=== Ejecución finalizada ===" << std::endl;
}
