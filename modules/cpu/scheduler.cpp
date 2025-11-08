#include "scheduler.hpp"
#include <queue>
#include <numeric>

namespace os
{
    namespace cpu
    {

        // -----------------------------
        // SJF (no expropiativo)
        // -----------------------------
        SchedulerStats CPUScheduler::sjf_non_preemptive(std::vector<Process> &processes)
        {
            SchedulerStats stats{};
            const int n = (int)processes.size();
            if (n == 0)
                return stats;

            // Ordenar por tiempo de llegada
            std::sort(processes.begin(), processes.end(),
                      [](const Process &a, const Process &b)
                      {
                          return a.arrival < b.arrival;
                      });

            int current_time = 0;
            int completed = 0;
            std::vector<bool> done(n, false);

            long long total_wait = 0, total_turnaround = 0;

            while (completed < n)
            {
                // Encontrar proceso elegible con menor burst
                int idx = -1;
                int min_burst = 1e9;

                for (int i = 0; i < n; ++i)
                {
                    if (!done[i] && processes[i].arrival <= current_time)
                    {
                        if (processes[i].burst < min_burst)
                        {
                            min_burst = processes[i].burst;
                            idx = i;
                        }
                    }
                }

                if (idx == -1)
                {
                    // No hay procesos listos, avanzar tiempo
                    current_time++;
                    continue;
                }

                Process &p = processes[idx];
                p.waiting = current_time - p.arrival;
                p.start = current_time; // <-- registrar inicio real
                p.finish = current_time + p.burst;
                p.turnaround = p.finish - p.arrival;
                current_time = p.finish;

                done[idx] = true;
                completed++;

                total_wait += p.waiting;
                total_turnaround += p.turnaround;
            }

            stats.avg_wait = static_cast<float>(total_wait) / n;
            stats.avg_turnaround = static_cast<float>(total_turnaround) / n;

            return stats;
        }

        // -----------------------------
        // Round Robin (preemptivo)
        // -----------------------------
        SchedulerStats CPUScheduler::round_robin(std::vector<Process> &processes, int quantum)
        {
            SchedulerStats stats{};
            const int n = (int)processes.size();
            if (n == 0 || quantum <= 0)
                return stats;

            // Tiempo restante de cada proceso
            std::vector<int> rem(n);
            for (int i = 0; i < n; ++i)
                rem[i] = processes[i].burst;

            // Índices ordenados por llegada para encolar
            std::vector<int> idx(n);
            std::iota(idx.begin(), idx.end(), 0);
            std::sort(idx.begin(), idx.end(),
                      [&](int a, int b)
                      {
                          if (processes[a].arrival != processes[b].arrival)
                              return processes[a].arrival < processes[b].arrival;
                          return processes[a].id < processes[b].id;
                      });

            std::queue<int> q; // cola de listos (índices de procesos)
            int next_arrival_ix = 0;
            int time = 0;
            int finished = 0;

            // Registrar primera vez que toman CPU
            std::vector<int> first_start(n, -1); // <-- nuevo

            // Si no hay nadie al tiempo 0, saltar al primer arribo
            if (next_arrival_ix < n && processes[idx[next_arrival_ix]].arrival > time)
                time = processes[idx[next_arrival_ix]].arrival;

            // Encolar todos los que hayan llegado hasta 'time'
            while (next_arrival_ix < n && processes[idx[next_arrival_ix]].arrival <= time)
            {
                q.push(idx[next_arrival_ix++]);
            }

            while (finished < n)
            {
                if (q.empty())
                {
                    // Saltar al siguiente arribo y encolar
                    time = std::max(time, processes[idx[next_arrival_ix]].arrival);
                    q.push(idx[next_arrival_ix++]);
                    while (next_arrival_ix < n && processes[idx[next_arrival_ix]].arrival <= time)
                    {
                        q.push(idx[next_arrival_ix++]);
                    }
                }

                int u = q.front();
                q.pop();

                // Asegurar que el CPU no “corra” antes de la llegada real
                time = std::max(time, processes[u].arrival);

                // Registrar primer inicio
                if (first_start[u] == -1)
                    first_start[u] = time;

                int slice = std::min(quantum, rem[u]);
                rem[u] -= slice;
                time += slice;

                // Encolar nuevos arribos que llegaron durante este slice
                while (next_arrival_ix < n && processes[idx[next_arrival_ix]].arrival <= time)
                {
                    q.push(idx[next_arrival_ix++]);
                }

                if (rem[u] > 0)
                {
                    // Aún no terminó: vuelve al final de la cola
                    q.push(u);
                }
                else
                {
                    // Terminó: calcular finish, turnaround y waiting
                    processes[u].finish = time;
                    processes[u].turnaround = processes[u].finish - processes[u].arrival;
                    processes[u].waiting = processes[u].turnaround - processes[u].burst;
                    processes[u].start = first_start[u]; // <-- guardar inicio
                    finished++;
                }
            }

            long long sum_wait = 0, sum_turn = 0;
            for (const auto &p : processes)
            {
                sum_wait += p.waiting;
                sum_turn += p.turnaround;
            }
            stats.avg_wait = (n ? (float)sum_wait / n : 0.0f);
            stats.avg_turnaround = (n ? (float)sum_turn / n : 0.0f);
            return stats;
        }

        // -----------------------------
        // Impresión (orden por inicio real)
        // -----------------------------
        void CPUScheduler::print_table(const std::vector<Process> &processes,
                                       const SchedulerStats &stats,
                                       std::ostream &out)
        {
            // Orden: por start, luego arrival, luego id
            std::vector<Process> v = processes; // copia
            std::stable_sort(v.begin(), v.end(), [](const Process &a, const Process &b)
                             {
        if (a.start   != b.start)   return a.start   < b.start;
        if (a.arrival != b.arrival) return a.arrival < b.arrival;
        return a.id < b.id; });

            out << "ID\tLlegada\tDuración\tInicio\tEspera\tFinal\tRetorno\n";
            for (const auto &p : v)
            {
                out << p.id << "\t"
                    << p.arrival << "\t"
                    << p.burst << "\t\t"
                    << p.start << "\t"
                    << p.waiting << "\t"
                    << p.finish << "\t"
                    << p.turnaround << "\n";
            }
            out << "\nPromedio de espera: " << stats.avg_wait << " unidades\n";
            out << "Promedio de retorno: " << stats.avg_turnaround << " unidades\n";
        }

    }
}
