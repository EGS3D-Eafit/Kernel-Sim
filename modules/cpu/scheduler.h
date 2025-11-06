#pragma once
#include "pcb.h"
#include <deque>

using namespace std;

class Scheduler {
public:
    void agregarProceso(PCB* proceso);
    void ejecutar(int quantum, int tick);
    void listarProcesos()const;

private:
    deque<PCB*> cola;
    int curQuantum = 0;
};