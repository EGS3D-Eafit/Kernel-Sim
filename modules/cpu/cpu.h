#pragma once
#include "scheduler.h"

class CPU {
private:
    Scheduler scheduler;
    int quantum;

public:
    CPU(int q);
    void agregarProceso(const PCB& proceso);
    void ejecutarRoundRobin();
};
