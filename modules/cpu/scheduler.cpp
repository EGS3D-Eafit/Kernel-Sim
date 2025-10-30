#include "scheduler.h"
#include "pcb.h"
#include <iostream>

void Scheduler::agregarProceso(PCB* proceso) {
    cola.push_back(proceso);
}

void Scheduler::ejecutar(int quantum) {
    while (!cola.empty()) {
        PCB* proceso = cola.front();
        cola.pop_front();

        proceso->ejecutar(quantum);

        if (!proceso->terminado()) {
            cola.push_back(proceso); // vuelve al final de la cola
        } else {
            std::cout << "Proceso " << proceso->name << " terminado." << std::endl;
            delete proceso;
        }
    }
}
