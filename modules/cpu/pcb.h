#pragma once
#include <iostream>

class PCB {
public:
    int id;
    int tiempoEjecucion;

    PCB(int id, int tiempoEjecucion)
        : id(id), tiempoEjecucion(tiempoEjecucion) {}

    void ejecutar(int quantum) {
        if (tiempoEjecucion > 0) {
            int ejecutarAhora = std::min(quantum, tiempoEjecucion);
            tiempoEjecucion -= ejecutarAhora;
            std::cout << "Proceso " << id << " ejecuta " << ejecutarAhora 
                      << " unidades. Restante: " << tiempoEjecucion << std::endl;
        }
    }

    bool terminado() const {
        return tiempoEjecucion <= 0;
    }
};
