#include "cpu.h"
#include "scheduler.h"
#include <iostream>

CPU::CPU(int q) : quantum(q)
{
    scheduler = new Scheduler();
}
CPU::~CPU() { delete scheduler; }

void CPU::set_algo_rr() { scheduler->set_algo(SchedAlgo::RR); }
void CPU::set_algo_sjf() { scheduler->set_algo(SchedAlgo::SJF); }

void CPU::add_process(const PCB &pcb)
{
    PCB *heap_copy = new PCB(pcb);
    scheduler->agregarProceso(heap_copy);
}

void CPU::ejecutar(int tick)
{
    scheduler->ejecutar(quantum, tick);
}

void CPU::listarProcesos() const
{
    scheduler->listarProcesos();
}

bool CPU::suspend_pid(int pid) { return scheduler->suspend_pid(pid); }
bool CPU::resume_pid(int pid) { return scheduler->resume_pid(pid); }
bool CPU::terminate_pid(int pid) { return scheduler->terminate_pid(pid); }

void CPU::irq_timer() { scheduler->irq_timer(quantum); }
bool CPU::irq_io_unblock(int pid) { return scheduler->irq_io_unblock(pid); }
