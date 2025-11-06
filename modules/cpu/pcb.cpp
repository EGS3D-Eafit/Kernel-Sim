#include <iostream>
#include <algorithm> // Para std::min
#include "pcb.h"

PCB::PCB(const string &name, int tiempoEjecucion)
    : name(name), tiempoEjecucion(tiempoEjecucion) {}

void PCB::ejecutar(int tick) { //Cambio parametro quantum -> tick
    if (tiempoEjecucion > 0) {
        int ejecutarAhora = min(tick, tiempoEjecucion);
        tiempoEjecucion -= ejecutarAhora;
        cout << "Proceso " << name << " ejecuta " << ejecutarAhora
             << " unidades. Restante: " << tiempoEjecucion << endl;
    }
}

bool PCB::terminado() const {
    return tiempoEjecucion <= 0;
}
