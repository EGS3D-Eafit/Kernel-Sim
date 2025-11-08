#pragma once
#include <vector>
#include <queue>
#include <list>
#include <unordered_map>
#include <optional>
#include <string>
#include <iostream>
#include <fstream>

namespace os
{
    namespace mem
    {

        enum class Policy
        {
            FIFO,
            LRU,
            PFF
        };

        struct PagingStats
        {
            int frames = 0;
            int steps = 0;
            int faults = 0;
            int hits = 0;

            // frame_table[t][f] = página en el frame f después de procesar la página t (0..steps-1)
            std::vector<std::vector<std::optional<int>>> frame_table;

            // true si el acceso en t fue fallo de página
            std::vector<bool> fault_flags;

            // secuencia de referencia original
            std::vector<int> ref_string;

            // Solo PFF: cantidad de marcos usados en cada paso y máximo observado.
            std::vector<int> frames_per_step; // len = steps
            int max_frames_observed = 0;
        };

        struct PFFParams
        {
            int frames_init = 3;  // marcos iniciales
            int frames_max = 6;   // cota superior
            int frames_min = 1;   // cota inferior (opcional)
            int window_size = 5;  // VENTANA
            int fault_thresh = 3; // UMBRAL_FALLOS
        };

        // Simulador de paginación con políticas FIFO, LRU y PFF.
        // Recolecta trazas por paso y métricas para visualización.
        class PagerSimulator
        {
        public:
            // Ejecuta la simulación y devuelve estadísticas y traza completa.
            static PagingStats run(const std::vector<int> &pages, int frames, Policy policy);

            // Ejecutor PFF con parámetros explícitos
            static PagingStats run_pff(const std::vector<int> &pages, const PFFParams &p);

            // Visualización ASCII de la tabla de marcos por paso.
            static void print_ascii(const PagingStats &s, std::ostream &out = std::cout);

            // Exporta CSV para graficar (por ejemplo, en notebook):
            // - "timeline.csv": por columna, el contenido de cada frame en cada paso.
            // - "events.csv": secuencia, pagina, hit, fault.
            static bool export_csv(const PagingStats &s,
                                   const std::string &timeline_csv_path,
                                   const std::string &events_csv_path);
        };

    }
}
