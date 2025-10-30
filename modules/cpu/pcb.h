#pragma once
#include <iostream>
#include <string>

using namespace std;

class PCB {
public:
    string name;
    int tiempoEjecucion;

    PCB(const string &name, int tiempoEjecucion);

    void ejecutar(int quantum);

    bool terminado() const;
};