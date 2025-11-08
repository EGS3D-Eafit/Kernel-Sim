#pragma once
#include <vector>
#include <string>
#include <optional>
#include <iostream>

#include "paging.hpp" // reutiliza Policy y PagingStats, PagerSimulator::run

namespace os
{
    namespace mem
    {

        // Proceso con su secuencia de referencias
        struct ProcRefs
        {
            int pid = 0;
            std::vector<int> refs;
        };

        enum class FrameAllocPolicy
        {
            EQUAL,       // reparto uniforme
            PROPORTIONAL // proporcional a páginas únicas referenciadas
        };

        struct AllocationPlan
        {
            int total_frames = 0;
            std::vector<int> frames_per_proc; // frames asignados a cada proceso (en el mismo orden de entrada)
        };

        // Resultados de la simulación de asignación
        struct AllocationSimStats
        {
            FrameAllocPolicy alloc_policy;
            Policy repl_policy; // FIFO o LRU (reemplazo local por proceso)
            int total_frames = 0;

            // por proceso (en el mismo orden que el input)
            std::vector<AllocationPlan> debug_plans; // opcional (no usado: por si generas variantes)
            std::vector<int> frames_assigned;
            std::vector<PagingStats> per_process;

            // agregados
            int total_faults = 0;
            int total_hits = 0;
            double hit_rate = 0.0;
        };

        // Motor de asignación + simulación
        class FrameAllocator
        {
        public:
            // Construye un plan de asignación dado N procesos y total de frames
            static AllocationPlan plan_equal(size_t n_procs, int total_frames);
            static AllocationPlan plan_proportional(const std::vector<ProcRefs> &procs, int total_frames);

            // Simula con la política de asignación elegida y política de reemplazo local (FIFO/LRU)
            static AllocationSimStats simulate(const std::vector<ProcRefs> &procs,
                                               int total_frames,
                                               FrameAllocPolicy alloc_policy,
                                               Policy repl_policy);

            // Impresiones de soporte
            static void print_report(const std::vector<ProcRefs> &procs,
                                     const AllocationSimStats &stats,
                                     std::ostream &out = std::cout);

            // Exporta CSV resumen: pid,frames,faults,hits,hit_rate
            static bool export_summary_csv(const std::vector<ProcRefs> &procs,
                                           const AllocationSimStats &stats,
                                           const std::string &csv_path);
        };

    }
}
