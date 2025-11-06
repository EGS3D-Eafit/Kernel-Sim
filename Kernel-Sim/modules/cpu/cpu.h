#pragma once
#include "scheduler.h"
#include "pcb.h"

#ifndef CPU_H
#define CPU_H

#include <string>

class CPU
{
private:
    Scheduler scheduler;

public:
    explicit CPU(int q = 3) : scheduler(q) {}

    // Planificación
    void set_policy_rr() { scheduler.setPolicy(Scheduler::Policy::RR); }
    void set_policy_sjf() { scheduler.setPolicy(Scheduler::Policy::SJF); }
    void set_quantum(int q) { scheduler.setQuantum(q); }

    // Procesos
    void add_process(PCB *pcb) { scheduler.agregarProceso(pcb); }

    // Simulación
    bool tick() { return scheduler.step(); } // 1 unidad de tiempo
    void run_n(int n)
    {
        for (int i = 0; i < n; i++)
            scheduler.step();
    }
    void run_all() { scheduler.ejecutarHastaVacio(); }

    void ejecutarRoundRobin() { run_all(); } // compatibilidad con el módulo disk

    // Estado
    std::string ps() const { return scheduler.ps(); }

    // Métricas (lectura)
    const std::vector<ProcStats> &stats() const { return scheduler.stats(); }
};

#endif
