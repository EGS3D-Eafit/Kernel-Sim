#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <optional>

namespace os
{
    namespace disk
    {

        struct DiskRequest
        {
            std::string id; // identificador de la petición
            int arrival;    // tiempo de llegada
            int service;    // tiempo de servicio (latencia/tiempo de I/O)
            int cylinder;   // cilindro solicitado (posición física)

            // Resultados
            int completion = 0;
            int turnaround = 0;
            int waiting = 0;
            int start_time = 0; // cuando realmente comienza a ser atendida
        };

        // Estadísticas y trazas
        struct DiskStats
        {
            float avg_wait = 0.0f;
            float avg_turnaround = 0.0f;
            long long total_movement = 0; // Σ|Δcabezal|
            int total_requests = 0;

            // Trayectoria del cabezal (incluye posición inicial y cada salto)
            std::vector<int> head_path; // p.ej., [head0, r1.cyl, r2.cyl, ...]
            // Orden real de servicio (índices en el vector resultante o ids)
            std::vector<std::string> service_order;
        };

        class DiskScheduler
        {
        public:
            // FCFS con movimiento de cabezal. Modifica 'requests' para fijar tiempos finales.
            static DiskStats fcfs(std::vector<DiskRequest> &requests, int head_start);

            // SSTF: siempre atiende la petición disponible más cercana al cabezal.
            // Nota: 'requests' se pasa por valor para poder reordenar internamente sin afectar al llamador.
            static DiskStats sstf(std::vector<DiskRequest> requests, int head_start);

            // Tabla de tiempos (llegada, servicio, finalización, retorno, espera)
            static void print_table(const std::vector<DiskRequest> &requests,
                                    const DiskStats &stats,
                                    std::ostream &out = std::cout);

            // Imprime la corrida del cabezal y el movimiento total
            static void print_head_path(const DiskStats &stats, std::ostream &out = std::cout);

            // Exporta CSV de la trayectoria del cabezal (step,cylinder,req_id)
            // Retorna true si se escribió correctamente.
            static bool export_head_csv(const DiskStats &stats,
                                        const std::string &csv_path);
        };

    }
}
