#include <iostream>
#include <algorithm>
#include "pcb.h"

int PCB::next_pid = 1;

PCB::PCB(const string &name, int tiempoEjecucion)
    : pid(next_pid++), name(name), tiempoEjecucion(max(0, tiempoEjecucion)), state(ProcState::NEW) {}

int PCB::ejecutar(int tick)
{
    if (tiempoEjecucion > 0)
    {
        int ejecutarAhora = min(tick, tiempoEjecucion);
        tiempoEjecucion -= ejecutarAhora;
        std::cout << "Proceso " << name << " [PID " << pid << "] ejecuta "
                  << ejecutarAhora << " unidades. Restante: " << tiempoEjecucion << "\n";
    }
}

bool PCB::terminado() const
{
    return tiempoEjecucion <= 0 || state == ProcState::TERMINATED;
}

void PCB::suspend()
{
    if (state != ProcState::TERMINATED)
        state = ProcState::SUSPENDED;
}
void PCB::resume()
{
    if (state == ProcState::SUSPENDED)
        state = ProcState::READY;
}
void PCB::block()
{
    if (state == ProcState::RUNNING)
        state = ProcState::BLOCKED;
}
void PCB::unblock()
{
    if (state == ProcState::BLOCKED)
        state = ProcState::READY;
}
void PCB::terminate() { state = ProcState::TERMINATED; }
