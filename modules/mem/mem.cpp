#include "mem.h"
#include <iostream>

Memoria::Memoria(int tamaño) : bloques(tamaño, 0) {}

int Memoria::asignar(int cantidad) {
    for (size_t i = 0; i < bloques.size(); ++i) {
        if (bloques[i] == 0) {
            bloques[i] = 1;
            return i;
        }
    }
    return -1;
}

void Memoria::liberar(int inicio, int cantidad) {
    for (int i = inicio; i < inicio + cantidad && i < bloques.size(); ++i)
        bloques[i] = 0;
}

void Memoria::mostrar() {
    for (auto b : bloques)
        std::cout << (b ? "#" : "_");
    std::cout << "\n";
}
