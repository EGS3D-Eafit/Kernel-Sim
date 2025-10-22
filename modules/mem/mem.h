#pragma once
#include <vector>

class Memoria {
private:
    std::vector<int> bloques;
public:
    Memoria(int tamaño);
    int asignar(int cantidad);
    void liberar(int inicio, int cantidad);
    void mostrar();
};
