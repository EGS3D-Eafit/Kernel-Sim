#pragma once
#include "pcb.h"
#include <queue>

class Scheduler {
private:
    std::queue<PCB*> cola;

public:
    void agregarProceso(PCB* proceso);
    void ejecutar(int quantum);
};
