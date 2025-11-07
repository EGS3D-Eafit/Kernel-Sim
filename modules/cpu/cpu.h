#pragma once
#include "scheduler.h"
#include "pcb.h"

#ifndef CPU_H
#define CPU_H

class CPU
{
private:
    Scheduler *scheduler;
    int quantum;

public:
    CPU(int q);
    ~CPU();

    void set_algo_rr();
    void set_algo_sjf();

    void ejecutar(int tick); // despacha según algoritmo actual
    void add_process(const PCB &pcb);
    void listarProcesos() const;

    // wrappers CLI
    bool suspend_pid(int pid);
    bool resume_pid(int pid);
    bool terminate_pid(int pid);
    void irq_timer();
    bool irq_io_unblock(int pid);
};
#endif
