#include "cpu.h"
#include "scheduler.h"
#include "pcb.h"
#include <iostream>

CPU::CPU(int q) : quantum(q) {
    scheduler = new Scheduler;
}

void CPU::add_process(PCB* pcb) {
    scheduler->agregarProceso(pcb);
}

void CPU::ejecutarRoundRobin(int tick) { //Aqui cambie para que tome los ticks que le da el usuario
    // std::cout << "=== Iniciando ejecución Round Robin ===" << std::endl; obsoletos
    scheduler->ejecutar(quantum, tick);
    // std::cout << "=== Ejecución finalizada ===" << std::endl;
}

void CPU::listarProcesos() const {
    scheduler->listarProcesos();
}
