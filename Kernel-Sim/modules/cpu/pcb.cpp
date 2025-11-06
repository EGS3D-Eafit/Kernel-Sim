#include "pcb.h"
#include <algorithm>
#include <iostream>

int PCB::next_id = 1;

PCB::PCB(const std::string &name, int tiempoEjecucion)
    : id(next_id++), name(name), total_cpu_restante(tiempoEjecucion) {}

PCB::PCB(const std::string &name, const std::vector<int> &rafagas)
    : id(next_id++), name(name), rafagasCPU(rafagas), total_cpu_restante(0) {}

void PCB::ejecutar_un_tick()
{
    if (!rafagasCPU.empty())
    {
        // Usando ráfagas: consume de la primera ráfaga
        if (rafagasCPU.front() > 0)
        {
            rafagasCPU.front() -= 1;
        }
        if (!rafagasCPU.empty() && rafagasCPU.front() == 0)
        {
            rafagasCPU.erase(rafagasCPU.begin());
        }
    }
    else
    {
        if (total_cpu_restante > 0)
        {
            total_cpu_restante -= 1;
        }
    }
}

bool PCB::terminado() const
{
    if (!rafagasCPU.empty())
    {
        return false;
    }
    return total_cpu_restante <= 0;
}
