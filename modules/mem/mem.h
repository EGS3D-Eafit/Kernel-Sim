#pragma once
#include <vector>
#include <string>

class Memoria
{
private:
    std::vector<int> bloques;

public:
    Memoria(int tamaño);
    int asignar(int cantidad);
    void liberar(int inicio, int cantidad);
    void mostrar();
};

namespace memdemo
{
    // Ejecuta una traza de memoria leyendo cabeceras y accesos.
    void run_trace_file(const std::string &path);

    // Imprime estadísticas y resumen del estado.
    void stat();
}
