#include "scheduler.h"
#include <iostream>
#include <algorithm>

using std::cout;
using std::endl;

Scheduler::Scheduler() : curQuantum(0), current(nullptr), algo(SchedAlgo::RR) {}

void Scheduler::set_algo(SchedAlgo a) { algo = a; }
SchedAlgo Scheduler::get_algo() const { return algo; }

std::optional<PCB *> Scheduler::find_by_pid(int pid)
{
    for (auto *p : pool)
        if (p && p->pid == pid && p->state != ProcState::TERMINATED)
            return p;
    return std::nullopt;
}

void Scheduler::agregarProceso(PCB *p)
{
    p->arrived_order = ++arrival_counter;
    if (p->state == ProcState::NEW)
        p->state = ProcState::READY;
    pool.push_back(p);
    // Para RR siempre va a cola; para SJF lo dejamos en READY (sin cola explícita)
    cola_ready.push_back(p);
}

void Scheduler::cleanup_terminated()
{
    // limpia current si terminó
    if (current && current->terminado())
    {
        current = nullptr;
    }
    // limpia de cola_ready cualquier terminado/suspendido/bloqueado
    std::deque<PCB *> tmp;
    while (!cola_ready.empty())
    {
        PCB *p = cola_ready.front();
        cola_ready.pop_front();
        if (!p->terminado() && p->state == ProcState::READY)
            tmp.push_back(p);
    }
    cola_ready.swap(tmp);

    // limpia pool (no borra memoria, solo ignora terminados)
}

PCB *Scheduler::pick_sjf()
{
    PCB *best = nullptr;
    for (auto *p : pool)
    {
        if (!p || p->terminado())
            continue;
        if (p->state != ProcState::READY)
            continue;
        if (!best || p->tiempoEjecucion < best->tiempoEjecucion ||
            (p->tiempoEjecucion == best->tiempoEjecucion && p->arrived_order < best->arrived_order))
        {
            best = p;
        }
    }
    return best;
}

void Scheduler::ensure_current()
{
    if (current && (current->terminado() || current->state != ProcState::RUNNING))
    {
        current = nullptr;
    }
    if (!current)
    {
        // elegir siguiente según algoritmo
        if (algo == SchedAlgo::RR)
        {
            while (!cola_ready.empty())
            {
                PCB *p = cola_ready.front();
                cola_ready.pop_front();
                if (!p->terminado() && p->state == ProcState::READY)
                {
                    current = p;
                    current->state = ProcState::RUNNING;
                    curQuantum = 0;
                    break;
                }
            }
        }
        else
        { // SJF no apropiativo
            PCB *p = pick_sjf();
            if (p)
            {
                current = p;
                current->state = ProcState::RUNNING;
            }
        }
    }
}

void Scheduler::ejecutar(int quantum, int tick)
{
    int restante = tick;
    while (restante > 0)
    {
        cleanup_terminated();
        ensure_current();
        if (!current)
        {
            cout << "(CPU idle)" << endl;
            break;
        }
        int slice = 1; // avanzamos de a 1 para respetar quantum y “ligereza”
        int ran = current->ejecutar(slice);
        restante -= ran;

        if (current->terminado())
        {
            current->terminate(); // estado TERMINATED
            current = nullptr;
            continue;
        }

        if (algo == SchedAlgo::RR)
        {
            curQuantum += ran;
            if (curQuantum >= quantum)
            {
                // desalojo por quantum (RR)
                current->state = ProcState::READY;
                cola_ready.push_back(current);
                current = nullptr;
                curQuantum = 0;
            }
        }
        else
        {
            // SJF no apropiativo: sigue hasta que termine o bloquee (sin desalojo por quantum)
        }
        if (ran == 0)
            break; // nada ejecutado (p. ej. suspended/blocked), salimos
    }
}

void Scheduler::listarProcesos() const
{
    auto state_str = [](ProcState s)
    {
        switch (s)
        {
        case ProcState::NEW:
            return "NEW";
        case ProcState::READY:
            return "READY";
        case ProcState::RUNNING:
            return "RUNNING";
        case ProcState::BLOCKED:
            return "BLOCKED";
        case ProcState::SUSPENDED:
            return "SUSPENDED";
        case ProcState::TERMINATED:
            return "TERMINATED";
        }
        return "?";
    };
    cout << "=== Procesos ===" << endl;
    cout << "Algoritmo: " << ((algo == SchedAlgo::RR) ? "RR" : "SJF") << endl;
    for (auto *p : pool)
    {
        if (!p)
            continue;
        cout << "PID " << p->pid << " | " << p->name
             << " | restante=" << p->tiempoEjecucion
             << " | estado=" << state_str(p->state) << endl;
    }
    cout << "Cola READY (RR):";
    for (auto *p : cola_ready)
        cout << " " << p->pid;
    cout << endl;
}

bool Scheduler::suspend_pid(int pid)
{
    auto opt = find_by_pid(pid);
    if (!opt)
        return false;
    PCB *p = *opt;
    p->suspend();
    return true;
}

bool Scheduler::resume_pid(int pid)
{
    auto opt = find_by_pid(pid);
    if (!opt)
        return false;
    PCB *p = *opt;
    p->resume();
    if (algo == SchedAlgo::RR && p->state == ProcState::READY)
        cola_ready.push_back(p);
    return true;
}

bool Scheduler::terminate_pid(int pid)
{
    auto opt = find_by_pid(pid);
    if (!opt)
        return false;
    PCB *p = *opt;
    p->terminate();
    if (current == p)
        current = nullptr;
    return true;
}

void Scheduler::irq_timer(int quantum)
{
    // ejecuta un “tick” de CPU
    ejecutar(quantum, 1);
}

bool Scheduler::irq_io_unblock(int pid)
{
    auto opt = find_by_pid(pid);
    if (!opt)
        return false;
    PCB *p = *opt;
    if (p->state != ProcState::BLOCKED)
        return false;
    p->unblock();
    if (algo == SchedAlgo::RR)
        cola_ready.push_back(p);
    return true;
}
