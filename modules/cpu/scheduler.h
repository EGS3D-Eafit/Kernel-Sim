#pragma once
#include "pcb.h"
#include <deque>
#include <vector>
#include <optional>

enum class SchedAlgo
{
    RR,
    SJF
};

class Scheduler
{
public:
    Scheduler();

    void set_algo(SchedAlgo a);
    SchedAlgo get_algo() const;

    void agregarProceso(PCB *proceso); // entra a READY

    // Ejecuta planificación durante 'tick' unidades usando el quantum si RR
    void ejecutar(int quantum, int tick);

    void listarProcesos() const;

    // Control de estados
    bool suspend_pid(int pid);
    bool resume_pid(int pid);
    bool terminate_pid(int pid);

    // Interrupciones
    void irq_timer(int quantum);  // avanza un “tick” de CPU
    bool irq_io_unblock(int pid); // desbloquea pid

    // Utilidad
    std::optional<PCB *> find_by_pid(int pid);

private:
    std::deque<PCB *> cola_ready; // RR
    std::vector<PCB *> pool;      // todos los procesos vivos/no terminados
    int curQuantum = 0;
    PCB *current = nullptr;
    SchedAlgo algo = SchedAlgo::RR;
    int arrival_counter = 0;

    // SJF helpers
    PCB *pick_sjf(); // el de menor tiempoEjecucion (no apropiativo)
    void ensure_current();
    void cleanup_terminated();
};
