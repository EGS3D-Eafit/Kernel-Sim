#pragma once
#include "scheduler.h"
#include "pcb.h"

#ifndef CPU_H
#define CPU_H

class CPU {
private:
    Scheduler scheduler;
    int quantum;

public:
    CPU(int q);
    void ejecutarRoundRobin();
    void add_process(PCB * pcb);
};
#endif