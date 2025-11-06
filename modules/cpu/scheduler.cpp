#include "scheduler.h"
#include <sstream>
#include <algorithm>

Scheduler::Scheduler(int q) : quantum(q) {}

void Scheduler::setPolicy(Policy p)
{
    pol = p;
}

void Scheduler::setQuantum(int q)
{
    if (q <= 0)
        q = 1;
    quantum = q;
}

void Scheduler::agregarProceso(PCB *proceso, int now_tick)
{
    proceso->estado = PCB::Estado::Listo;
    proceso->llegada_tick = now_tick;
    cola.push_back(proceso);
}

void Scheduler::schedule_next_if_needed()
{
    if (actual == nullptr)
    {
        if (pol == Policy::SJF)
        {
            if (!cola.empty())
            {
                // elegir el más corto (por tiempo restante total o primera ráfaga)
                auto it = std::min_element(cola.begin(), cola.end(),
                                           [](PCB *a, PCB *b)
                                           {
                                               int ra = a->rafagasCPU.empty() ? a->total_cpu_restante : a->rafagasCPU.front();
                                               int rb = b->rafagasCPU.empty() ? b->total_cpu_restante : b->rafagasCPU.front();
                                               return ra < rb;
                                           });
                actual = *it;
                cola.erase(it);
            }
        }
        else
        { // RR
            if (!cola.empty())
            {
                actual = cola.front();
                cola.pop_front();
            }
        }
        if (actual)
        {
            actual->estado = PCB::Estado::Ejecutando;
            slice_usado = 0;
            if (actual->primera_respuesta_tick < 0)
                actual->primera_respuesta_tick = reloj;
        }
    }
}

bool Scheduler::step()
{
    schedule_next_if_needed();
    if (actual == nullptr)
    {
        // Nada que hacer, avanza reloj inactivo
        reloj += 1;
        return false;
    }

    // Ejecuta 1 tick
    actual->ejecutar_un_tick();
    slice_usado += 1;
    reloj += 1;

    // Incrementa espera de los que están en cola
    for (auto *p : cola)
        p->tiempo_espera_acum += 1;

    // ¿Terminó?
    if (actual->terminado())
    {
        actual->estado = PCB::Estado::Terminado;
        actual->fin_tick = reloj;
        finalizados.push_back(ProcStats{
            actual->id, actual->name, actual->llegada_tick, actual->fin_tick,
            actual->turnaround(), actual->tiempo_espera_acum});
        delete actual;
        actual = nullptr;
        slice_usado = 0;
        return !cola.empty();
    }

    // ¿Se agotó el quantum en RR?
    if (pol == Policy::RR && slice_usado >= quantum)
    {
        actual->estado = PCB::Estado::Listo;
        cola.push_back(actual);
        actual = nullptr;
        slice_usado = 0;
    }

    return true;
}

void Scheduler::ejecutarHastaVacio()
{
    while (actual != nullptr || !cola.empty())
    {
        step();
    }
}

std::string Scheduler::ps() const
{
    std::ostringstream os;
    os << "=== ps ===\n";
    if (actual)
    {
        os << "[RUN] id=" << actual->id << " name=" << actual->name << "\n";
    }
    else
    {
        os << "[RUN] (idle)\n";
    }
    os << "Ready queue: ";
    if (cola.empty())
        os << "(vacía)";
    for (auto *p : cola)
    {
        os << "[id=" << p->id << " " << p->name << "] ";
    }
    os << "\n";
    return os.str();
}
