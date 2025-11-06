#pragma once
#include "scheduler.h"
#include "pcb.h"

#ifndef CPU_H
#define CPU_H

class CPU {
private:
    Scheduler* scheduler; // cambio importante de hacer
    int quantum; //Se inicializa un quantum pre establecido

public:
    CPU(int q);
    void ejecutarRoundRobin(int tick);
    void add_process(PCB * pcb);
    void listarProcesos() const;
};
#endif