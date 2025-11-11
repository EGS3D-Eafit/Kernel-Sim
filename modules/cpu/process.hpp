#pragma once
#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <unordered_map>
#include <iostream>
#include <iomanip>

namespace os
{
    namespace cpu
    {

        enum class ProcState
        {
            NEW,
            READY,
            RUNNING,
            BLOCKED,   // bloqueado por “I/O” (simulado) o un sleep
            SUSPENDED, // suspendido administrativamente
            TERMINATED
        };

        struct PCB
        {
            int pid = -1;
            std::string name;
            ProcState state = ProcState::NEW;

            // Tiempos (unidades de tick)
            int arrival_time = 0;  // instante en que “entra” al sistema
            int cpu_total = 0;     // ráfaga total requerida (cupo total)
            int cpu_remaining = 0; // lo que falta
            int start_time = -1;   // primera vez que corre
            int finish_time = -1;  // cuando termina

            // Métricas acumuladas
            int waiting_time = 0;   // tiempo en READY
            int running_time = 0;   // tiempo efectivamente en CPU
            int blocked_until = -1; // si está bloqueado por tiempo hasta este tick

            // Utilidad
            bool operator==(int other_pid) const { return pid == other_pid; }
        };

        struct TableSnapshot
        {
            int now = 0;
            std::vector<PCB> pcbs;
            std::optional<int> running_pid;
        };

        // Gestor de procesos (simulación monohilo, FCFS simple para despacho).
        // Expone API para crear, suspender, reanudar y terminar procesos.
        class ProcessManager
        {
        public:
            ProcessManager() = default;

            // Crea proceso en estado NEW. Se activará (READY) cuando now >= arrival_time (en tick()).
            // Retorna el pid asignado.
            int create_process(const std::string &name, int cpu_burst, int arrival_time = 0);

            // Suspende un proceso (pasa a SUSPENDED). Si está RUNNING, se desaloja.
            bool suspend(int pid);

            // Reanuda un proceso SUSPENDED → READY.
            bool resume(int pid);

            // Termina un proceso (TERMINATED). Si es RUNNING, libera CPU.
            bool terminate(int pid);

            // Bloquea un proceso por “ticks” (simulate I/O). Si es RUNNING pasa a BLOCKED y se despierta en now+ticks.
            bool block_for(int pid, int ticks);

            // Avanza el reloj ‘ticks’ unidades. Despierta procesos por llegada/bloqueo y ejecuta FCFS.
            void run(int ticks = 1);

            // Consulta
            std::optional<PCB> get(int pid) const;
            TableSnapshot snapshot() const;

            // Utilidades de depuración
            void print_table(std::ostream &out = std::cout) const;
            void print_queues(std::ostream &out = std::cout) const;

            // Tiempo actual
            int now() const { return now_; }

        private:
            // Internos
            PCB *find_mut(int pid);
            const PCB *find(int pid) const;

            void admit_new_arrivals();          // NEW -> READY cuando now_ >= arrival_time
            void wake_blocked();                // BLOCKED -> READY cuando now_ >= blocked_until
            void dispatch_if_idle();            // elige RUNNING si CPU libre (FCFS)
            void run_one_tick();                // ejecuta 1 tick al RUNNING
            void deschedule_running_to_ready(); // mueve RUNNING -> READY (por suspensión/bloqueo externo)

            int now_ = 0;
            int next_pid_ = 1;

            std::unordered_map<int, PCB> table_; // pid -> PCB
            std::deque<int> ready_q_;            // cola FCFS de pids READY
            std::optional<int> running_;         // pid RUNNING si existe
        };

    }
}
