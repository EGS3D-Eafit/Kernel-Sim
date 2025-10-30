#pragma once
#include "pcb.h"
#include <deque>

class Scheduler {
public:
    void agregarProceso(PCB* proceso);
    void ejecutar(int quantum);

private:
    std::deque<PCB*> cola;
};