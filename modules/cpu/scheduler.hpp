#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

namespace os
{
    namespace cpu
    {

        // Representa un proceso
        struct Process
        {
            int id;      // identificador del proceso
            int arrival; // tiempo de llegada
            int burst;   // duración o tiempo de CPU requerido

            // Resultados
            int waiting = 0;    // tiempo en cola antes de ejecutarse
            int start = 0;      // instante en que corre por primera vez
            int finish = 0;     // tiempo en que finaliza
            int turnaround = 0; // tiempo total en el sistema (finish - arrival)
        };

        // Estadísticas de ejecución
        struct SchedulerStats
        {
            float avg_wait = 0.0f;
            float avg_turnaround = 0.0f;
        };

        // Planificador de CPU
        class CPUScheduler
        {
        public:
            // Shortest Job First (no expropiativo)
            static SchedulerStats sjf_non_preemptive(std::vector<Process> &processes);

            // Round Robin (preemptivo) con quantum fijo (en unidades de tiempo)
            static SchedulerStats round_robin(std::vector<Process> &processes, int quantum);

            // Imprime resultados en formato tabla (ordenada por inicio real)
            static void print_table(const std::vector<Process> &processes,
                                    const SchedulerStats &stats,
                                    std::ostream &out = std::cout);
        };

    }
}
