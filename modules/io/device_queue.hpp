#pragma once
#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <iostream>

namespace os
{
    namespace io
    {

        // Prioridad 0 = más alta
        struct IORequest
        {
            std::string rid;   // id de la solicitud (p.ej. "R1" o "P42:read")
            std::string owner; // dueño (p.ej. pid o nombre de proceso)
            int arrival = 0;   // tiempo de llegada
            int service = 1;   // tiempo total que requiere el dispositivo
            int priority = 1;  // 0..(levels-1)

            // campos de simulación
            int start = -1;    // cuándo comenzó a atenderse
            int finish = -1;   // cuándo terminó
            int wait = 0;      // tiempo total de espera en cola
            int remaining = 0; // servicio restante (inicialmente = service)
        };

        // Parámetros del planificador del dispositivo
        struct DeviceParams
        {
            int priority_levels = 3; // número de niveles (>=1)
            bool aging_enabled = true;
            int aging_period = 5; // ticks de espera para promover +1 nivel (hacia prioridad 0)

            // Validación mínima
            void clamp()
            {
                if (priority_levels < 1)
                    priority_levels = 1;
                if (aging_period < 1)
                    aging_period = 1;
            }
        };

        // Estadísticas agregadas del dispositivo
        struct DeviceStats
        {
            int total_requests = 0;
            long long total_wait = 0;
            double avg_wait = 0.0;

            long long busy_time = 0;  // ticks en servicio
            long long total_time = 0; // ticks simulados
            double utilization = 0.0; // busy_time / total_time

            // por prioridad
            std::vector<int> count_by_prio;
            std::vector<long long> wait_by_prio;
            std::vector<double> avg_wait_by_prio;

            // orden real de servicio
            std::vector<std::string> service_order;
        };

        // Dispositivo con colas multinivel y prioridad (no expropiativo).
        class Device
        {
        public:
            explicit Device(std::string name, DeviceParams p = {})
                : name_(std::move(name)), params_(p) { reset(); }

            // Reinicia el estado interno (mantiene parámetros)
            void reset();

            // Envía una solicitud (queda pendiente hasta que su arrival <= now)
            void submit(const IORequest &req);

            // Avanza 'ticks' unidades de tiempo, admitiendo, programando y atendiendo.
            void tick(int ticks = 1);

            // Lectura de resultados
            DeviceStats stats() const;
            int now() const { return now_; }

            // Impresiones de soporte
            void print_table(std::ostream &out = std::cout) const;    // tabla de solicitudes completadas
            void print_queues(std::ostream &out = std::cout) const;   // estado actual de colas
            void print_timeline(std::ostream &out = std::cout) const; // ruta de servicio (orden)

            // Exporta CSV con la línea de tiempo de servicio: step, rid, owner
            bool export_timeline_csv(const std::string &csv_path) const;

        private:
            struct Pending
            {
                IORequest io;
                int enqueued_at = -1; // cuando entró a colas (para aging)
            };

            // Helpers
            void admit_arrivals();   // pasa de 'incoming_' a colas por prioridad
            void promote_aging();    // promoción por envejecimiento
            void dispatch_if_idle(); // selecciona de la cola de mayor prioridad
            void run_one_tick();     // atiende 1 tick a la solicitud en curso
            void finish_current();   // finaliza la actual
            bool idle() const { return current_ < 0; }

            std::string name_;
            DeviceParams params_;

            int now_ = 0;

            // almacenamiento y colas
            std::vector<Pending> incoming_;       // aún no ingresaron a colas (arrival > now)
            std::vector<Pending> completed_;      // finalizadas
            std::vector<std::deque<int>> queues_; // por prioridad: índices a 'pool_'
            std::vector<Pending> pool_;           // almacena todas las pendientes en colas

            int current_ = -1;   // índice en pool_ de la solicitud activa
            int busy_ticks_ = 0; // acumulado de tiempo ocupado

            // timeline de servicio (para visualización y CSV)
            std::vector<std::string> service_order_;
        };

    } // namespace io
} // namespace os
