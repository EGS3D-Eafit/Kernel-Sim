#pragma once
#include "pcb.h"
#include <deque>
#include <string>
#include <vector>
#include <optional>

struct ProcStats
{
    int id;
    std::string name;
    int llegada_tick;
    int fin_tick;
    int turnaround;
    int tiempo_espera;
};

class Scheduler
{
public:
    enum class Policy
    {
        RR,
        SJF
    };

    explicit Scheduler(int quantum = 3);

    void setPolicy(Policy p);
    Policy policy() const { return pol; }

    void setQuantum(int q);
    int getQuantum() const { return quantum; }

    void agregarProceso(PCB *proceso, int now_tick = 0); // encola (Listo)

    // Un tick de CPU (1 unidad de tiempo)
    bool step(); // devuelve false si no hay nada por ejecutar

    // Ejecuta hasta terminar todo (usa step)
    void ejecutarHastaVacio();

    // Estado (similar a ps)
    std::string ps() const;

    // Métricas acumuladas de finalizados
    const std::vector<ProcStats> &stats() const { return finalizados; }

private:
    // Helpers
    void schedule_next_if_needed();

    std::deque<PCB *> cola; // listos
    PCB *actual = nullptr;  // ejecutando
    int quantum = 3;
    int slice_usado = 0;
    int reloj = 0;

    Policy pol = Policy::RR;

    std::vector<ProcStats> finalizados;
};
