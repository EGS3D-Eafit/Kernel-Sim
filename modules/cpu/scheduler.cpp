#include "scheduler.h"
#include <iostream>

void Scheduler::agregarProceso(PCB* proceso) {
    cola.push(proceso);
}

void Scheduler::ejecutar(int quantum) {
    while (!cola.empty()) {
        PCB* proceso = cola.front();
        cola.pop();

        proceso->ejecutar(quantum);

        if (!proceso->terminado()) {
            cola.push(proceso); // vuelve al final de la cola
        } else {
            std::cout << "Proceso " << proceso->id << " terminado." << std::endl;
            delete proceso;
        }
    }
}
