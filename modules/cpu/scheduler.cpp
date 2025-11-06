#include "scheduler.h"
#include "pcb.h"
#include <iostream>

using namespace std;

void Scheduler::agregarProceso(PCB* proceso) {
    cola.push_back(proceso);
}

void Scheduler::ejecutar(int quantum, int tick) { // Varios cambios a la funcion con logica reciclada del pcb
    int tiempoRestante = tick;

    while (tiempoRestante > 0 && !cola.empty()) {

        if (curQuantum <= 0)
            curQuantum = quantum;

        PCB* proceso = cola.front();

        int ejecutarAhora = min(min(quantum, tiempoRestante), curQuantum); // para la persistencia de los ticks
        proceso->ejecutar(ejecutarAhora);
        tiempoRestante -= ejecutarAhora;
        curQuantum -= ejecutarAhora;

        if (curQuantum > 0)
            return;

        cola.pop_front();

        if (!proceso->terminado()) {
            cola.push_back(proceso);
        } else {
            cout << "Proceso " << proceso->name << " terminado." << endl;
            delete proceso;
            curQuantum = 0;
        }
    }
}


void Scheduler::listarProcesos() const {
    std::cout << "=== Procesos en cola ===" << std::endl;
    for (PCB* proceso : cola) {
        std::cout << "Proceso: " << proceso->name
                  << " | Tiempo restante: " << proceso->tiempoEjecucion << std::endl;
    }
}


