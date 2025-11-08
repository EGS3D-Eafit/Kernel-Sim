#include "scheduling.hpp"
#include <cmath>
#include <fstream>
#include <limits>

namespace os
{
    namespace disk
    {

        // ----------------------
        // Utilidades internas
        // ----------------------
        static long long movement_between(int a, int b)
        {
            return std::llabs((long long)a - (long long)b);
        }

        // Para promedios seguros con n>0
        static float safe_avg(long long sum, int n)
        {
            return (n > 0) ? static_cast<float>(sum) / static_cast<float>(n) : 0.0f;
        }

                // ----------------------
        // FCFS
        // ----------------------

        DiskStats DiskScheduler::fcfs(std::vector<DiskRequest> &reqs, int head_start)
        {
            DiskStats stats{};
            const int n = (int)reqs.size();
            if (n == 0)
                return stats;

            // Orden por llegada
            std::sort(reqs.begin(), reqs.end(),
                      [](const DiskRequest &a, const DiskRequest &b)
                      {
                          return a.arrival < b.arrival;
                      });

            // Tiempos y movimiento
            stats.head_path.clear();
            stats.head_path.push_back(head_start);

            long long sum_wait = 0, sum_tat = 0;
            long long movement = 0;

            int time = 0;
            int head = head_start;

            for (int i = 0; i < n; ++i)
            {
                // El inicio real es cuando el recurso queda libre y el proceso ha llegado
                int start_time = std::max(time, reqs[i].arrival);
                reqs[i].start_time = start_time;
                reqs[i].completion = start_time + reqs[i].service;
                reqs[i].turnaround = reqs[i].completion - reqs[i].arrival;
                reqs[i].waiting = reqs[i].turnaround - reqs[i].service;

                // Movimiento del cabezal hacia el cilindro solicitado
                movement += movement_between(head, reqs[i].cylinder);
                head = reqs[i].cylinder;
                stats.head_path.push_back(head);
                stats.service_order.push_back(reqs[i].id);

                sum_wait += reqs[i].waiting;
                sum_tat += reqs[i].turnaround;
                time = reqs[i].completion; // siguiente queda disponible luego
            }

            stats.avg_wait = safe_avg(sum_wait, n);
            stats.avg_turnaround = safe_avg(sum_tat, n);
            stats.total_requests = n;
            stats.total_movement = movement;

            return stats;
        }

        // ----------------------
        // SSTF
        // ----------------------

        // Selecciona índice de la petición (en 'pending') más cercana al cabezal.
        // Si hay empate por distancia, prioriza menor arrival; si empata, por id lexicográfico.

        static int pick_sstf_index(const std::vector<DiskRequest> &pending, int head, int current_time)
        {
            int best_idx = -1;
            long long best_dist = std::numeric_limits<long long>::max();

            for (int i = 0; i < (int)pending.size(); ++i)
            {
                if (pending[i].arrival > current_time)
                    continue; // aún no llegó
                long long d = movement_between(head, pending[i].cylinder);
                if (d < best_dist)
                {
                    best_dist = d;
                    best_idx = i;
                }
                else if (d == best_dist && best_idx != -1)
                {
                    if (pending[i].arrival < pending[best_idx].arrival)
                    {
                        best_idx = i;
                    }
                    else if (pending[i].arrival == pending[best_idx].arrival &&
                             pending[i].id < pending[best_idx].id)
                    {
                        best_idx = i;
                    }
                }
            }
            return best_idx;
        }

        DiskStats DiskScheduler::sstf(std::vector<DiskRequest> reqs, int head_start)
        {
            DiskStats stats{};
            const int n = (int)reqs.size();
            if (n == 0)
                return stats;

            // Ordenar por llegada para facilitar "siguiente arribo"
            std::sort(reqs.begin(), reqs.end(),
                      [](const DiskRequest &a, const DiskRequest &b)
                      {
                          if (a.arrival != b.arrival)
                              return a.arrival < b.arrival;
                          return a.id < b.id;
                      });

            // Pendientes sin iniciar
            std::vector<DiskRequest> pending = reqs;
            std::vector<DiskRequest> finished;
            finished.reserve(n);

            stats.head_path.clear();
            stats.head_path.push_back(head_start);

            long long sum_wait = 0, sum_tat = 0;
            long long movement = 0;

            int head = head_start;
            int time = 0;

            // Iniciar el tiempo en el primer arribo si no hay nada antes
            if (!pending.empty())
                time = std::max(time, pending.front().arrival);

            while (!pending.empty())
            {
                // Elegir la más cercana ya llegada
                int idx = pick_sstf_index(pending, head, time);

                if (idx == -1)
                {
                    // No hay solicitudes disponibles todavía: saltar al siguiente arribo
                    time = std::max(time, pending.front().arrival);
                    continue;
                }

                DiskRequest cur = pending[idx];
                pending.erase(pending.begin() + idx);

                cur.start_time = std::max(time, cur.arrival);
                cur.completion = cur.start_time + cur.service;
                cur.turnaround = cur.completion - cur.arrival;
                cur.waiting = cur.turnaround - cur.service;

                movement += movement_between(head, cur.cylinder);
                head = cur.cylinder;
                stats.head_path.push_back(head);
                stats.service_order.push_back(cur.id);

                sum_wait += cur.waiting;
                sum_tat += cur.turnaround;

                time = cur.completion;
                finished.push_back(cur);
            }

            // Copiamos resultados a 'reqs' ordenados por id para imprimir (opcional)
            // Aquí mantenemos finished (orden real de servicio) para print_table si se desea.
            // Para mantener coherencia con print_table, usamos 'finished'.
            stats.avg_wait = safe_avg(sum_wait, n);
            stats.avg_turnaround = safe_avg(sum_tat, n);
            stats.total_requests = n;
            stats.total_movement = movement;

            // Reemplazamos 'reqs' original por 'finished' para que el llamador pueda imprimir si quiere
            // (No es estrictamente necesario; print_table puede recibir 'finished' externamente.)
            // Dejamos esta responsabilidad al llamador.

            return stats;
        }

        // ----------------------
        // Impresiones y CSV
        // ----------------------
        void DiskScheduler::print_table(const std::vector<DiskRequest> &requests,
                                        const DiskStats &stats,
                                        std::ostream &out)
        {
            out << "Proceso\tLlegada\tServicio\tCilindro\tInicio\tFinal\tRetorno\tEspera\n";
            for (const auto &p : requests)
            {
                out << p.id << "\t"
                    << p.arrival << "\t"
                    << p.service << "\t\t"
                    << p.cylinder << "\t\t"
                    << p.start_time << "\t"
                    << p.completion << "\t"
                    << p.turnaround << "\t"
                    << p.waiting << "\n";
            }
            out << "\nTiempo promedio de espera: " << stats.avg_wait << " unidades\n";
            out << "Tiempo promedio de retorno: " << stats.avg_turnaround << " unidades\n";
            out << "Movimiento total del cabezal: " << stats.total_movement << " cilindros\n";
        }

        void DiskScheduler::print_head_path(const DiskStats &stats, std::ostream &out)
        {
            out << "Trayectoria del cabezal (cilindros): ";
            for (size_t i = 0; i < stats.head_path.size(); ++i)
            {
                if (i)
                    out << " -> ";
                out << stats.head_path[i];
            }
            out << "\nMovimiento total: " << stats.total_movement << " cilindros\n";

            if (!stats.service_order.empty())
            {
                out << "Orden de servicio: ";
                for (size_t i = 0; i < stats.service_order.size(); ++i)
                {
                    if (i)
                        out << ", ";
                    out << stats.service_order[i];
                }
                out << "\n";
            }
        }

        bool DiskScheduler::export_head_csv(const DiskStats &stats,
                                            const std::string &csv_path)
        {
            std::ofstream f(csv_path);
            if (!f.is_open())
                return false;

            // step,cylinder,req_id
            f << "step,cylinder,req_id\n";
            // El primer punto (posición inicial) no corresponde a una req_id
            if (!stats.head_path.empty())
            {
                f << 0 << "," << stats.head_path[0] << "," << "" << "\n";
            }
            // A partir del 1, asociamos en lo posible el id atendido
            for (size_t i = 1; i < stats.head_path.size(); ++i)
            {
                std::string id = (i - 1 < stats.service_order.size()) ? stats.service_order[i - 1] : "";
                f << i << "," << stats.head_path[i] << "," << id << "\n";
            }
            return true;
        }

    }
}
