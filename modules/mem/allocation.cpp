#include "allocation.hpp"
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <iomanip>

namespace os
{
    namespace mem
    {

        // -------------------------------
        // Helpers
        // -------------------------------
        static int clamp_min(int v, int lo) { return v < lo ? lo : v; }

        static int unique_pages(const std::vector<int> &v)
        {
            std::unordered_set<int> s(v.begin(), v.end());
            return (int)s.size();
        }

        // Reparte 'total' con mínimos (min_each) y proporciones reales 'weights'.
        // Usa floor y distribuye el resto por mayor parte fraccional.
        // Garantiza sum == total y cada bucket >= min_each.
        static std::vector<int> proportional_split(const std::vector<double> &weights,
                                                   int total, int min_each)
        {
            const size_t n = weights.size();
            std::vector<int> out(n, 0);
            if (n == 0)
                return out;
            if (total < (int)(n * min_each))
            {
                // No alcanza para dar min_each a todos: darlos a los primeros hasta agotar.
                for (int i = 0; i < (int)n && total > 0; ++i)
                {
                    out[i] = 1;
                    --total;
                }
                return out;
            }

            // Normalizar pesos
            double sumw = 0.0;
            for (auto w : weights)
                sumw += (w <= 0 ? 1.0 : w); // evitar ceros duros
            std::vector<double> norm(n);
            for (size_t i = 0; i < n; ++i)
                norm[i] = (weights[i] <= 0 ? 1.0 : weights[i]) / (sumw > 0 ? sumw : 1.0);

            // Asignación base = min_each
            for (size_t i = 0; i < n; ++i)
                out[i] = min_each;
            int remaining = total - (int)(n * min_each);
            if (remaining <= 0)
                return out;

            // Asignación proporcional del remanente
            std::vector<double> targets(n, 0.0);
            for (size_t i = 0; i < n; ++i)
                targets[i] = norm[i] * remaining;

            std::vector<int> base(n, 0);
            std::vector<double> frac(n, 0.0);
            int base_sum = 0;
            for (size_t i = 0; i < n; ++i)
            {
                base[i] = (int)std::floor(targets[i]);
                frac[i] = targets[i] - base[i];
                base_sum += base[i];
            }

            for (size_t i = 0; i < n; ++i)
                out[i] += base[i];

            int leftover = remaining - base_sum;
            // repartir el resto por fracciones más grandes
            std::vector<size_t> idx(n);
            std::iota(idx.begin(), idx.end(), 0);
            std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b)
                      {
        if (frac[a] == frac[b]) return a < b; // desempate estable
        return frac[a] > frac[b]; });
            for (int k = 0; k < leftover; ++k)
                out[idx[k % n]] += 1;

            return out;
        }

        // -------------------------------
        // Planes de asignación
        // -------------------------------
        AllocationPlan FrameAllocator::plan_equal(size_t n_procs, int total_frames)
        {
            AllocationPlan plan{};
            plan.total_frames = std::max(0, total_frames);
            plan.frames_per_proc.assign(n_procs, 0);
            if (n_procs == 0 || total_frames <= 0)
                return plan;

            int base = total_frames / (int)n_procs;
            int rem = total_frames % (int)n_procs;
            for (size_t i = 0; i < n_procs; ++i)
                plan.frames_per_proc[i] = base;
            for (int i = 0; i < rem; ++i)
                plan.frames_per_proc[i] += 1; // repartir sobrante a los primeros
            // garantizar al menos 1 frame si es posible
            for (size_t i = 0; i < n_procs; ++i)
            {
                if (plan.frames_per_proc[i] == 0)
                {
                    // toma de otros si hay
                    for (size_t j = 0; j < n_procs; j++)
                    {
                        if (plan.frames_per_proc[j] > 1)
                        {
                            plan.frames_per_proc[j]--;
                            plan.frames_per_proc[i]++;
                            break;
                        }
                    }
                }
            }
            return plan;
        }

        AllocationPlan FrameAllocator::plan_proportional(const std::vector<ProcRefs> &procs, int total_frames)
        {
            AllocationPlan plan{};
            plan.total_frames = std::max(0, total_frames);
            const size_t n = procs.size();
            plan.frames_per_proc.assign(n, 0);
            if (n == 0 || total_frames <= 0)
                return plan;

            std::vector<double> weights(n, 1.0);
            for (size_t i = 0; i < n; ++i)
            {
                int uniq = unique_pages(procs[i].refs);
                weights[i] = (double)std::max(1, uniq);
            }
            plan.frames_per_proc = proportional_split(weights, total_frames, /*min_each=*/1);
            return plan;
        }

        // -------------------------------
        // Simulación
        // -------------------------------
        AllocationSimStats FrameAllocator::simulate(const std::vector<ProcRefs> &procs,
                                                    int total_frames,
                                                    FrameAllocPolicy alloc_policy,
                                                    Policy repl_policy)
        {
            AllocationSimStats out{};
            out.alloc_policy = alloc_policy;
            out.repl_policy = repl_policy;
            out.total_frames = std::max(0, total_frames);

            const size_t n = procs.size();
            out.frames_assigned.assign(n, 0);
            out.per_process.clear();
            out.per_process.reserve(n);

            // plan
            AllocationPlan plan;
            if (alloc_policy == FrameAllocPolicy::EQUAL)
            {
                plan = plan_equal(n, total_frames);
            }
            else
            {
                plan = plan_proportional(procs, total_frames);
            }
            out.frames_assigned = plan.frames_per_proc;

            // Simular por proceso con reemplazo local
            int total_faults = 0;
            int total_hits = 0;

            for (size_t i = 0; i < n; ++i)
            {
                int frames_i = clamp_min(plan.frames_per_proc[i], 1); // al menos 1 frame
                auto stats_i = PagerSimulator::run(procs[i].refs, frames_i, repl_policy);
                out.per_process.push_back(std::move(stats_i));
                total_faults += out.per_process.back().faults;
                total_hits += out.per_process.back().hits;
            }

            out.total_faults = total_faults;
            out.total_hits = total_hits;

            int total_accesses = 0;
            for (const auto &p : procs)
                total_accesses += (int)p.refs.size();
            out.hit_rate = (total_accesses ? (double)out.total_hits / (double)total_accesses : 0.0);

            return out;
        }

        // -------------------------------
        // Impresiones y CSV
        // -------------------------------
        void FrameAllocator::print_report(const std::vector<ProcRefs> &procs,
                                          const AllocationSimStats &stats,
                                          std::ostream &out)
        {
            auto alloc_str = (stats.alloc_policy == FrameAllocPolicy::EQUAL ? "EQUAL" : "PROPORTIONAL");
            auto repl_str = (stats.repl_policy == Policy::FIFO ? "FIFO" : (stats.repl_policy == Policy::LRU ? "LRU" : "OTHER"));

            out << "Asignacion de marcos (" << alloc_str << "), Reemplazo local (" << repl_str << ")\n";
            out << "Total frames=" << stats.total_frames
                << "  Total procesos=" << procs.size()
                << "  Total fallos=" << stats.total_faults
                << "  HitRate=" << std::fixed << std::setprecision(2) << (stats.hit_rate * 100.0) << "%\n\n";

            out << "PID\tFrames\tRefs\tFaults\tHits\tHitRate\n";
            for (size_t i = 0; i < procs.size(); ++i)
            {
                const auto &p = procs[i];
                const auto &ps = stats.per_process[i];
                int refs = (int)p.refs.size();
                double hr = (refs ? (double)ps.hits / (double)refs : 0.0);
                out << p.pid << "\t"
                    << stats.frames_assigned[i] << "\t"
                    << refs << "\t"
                    << ps.faults << "\t"
                    << ps.hits << "\t"
                    << std::fixed << std::setprecision(2) << (hr * 100.0) << "%\n";
            }
        }

        bool FrameAllocator::export_summary_csv(const std::vector<ProcRefs> &procs,
                                                const AllocationSimStats &stats,
                                                const std::string &csv_path)
        {
            std::ofstream f(csv_path);
            if (!f.is_open())
                return false;
            f << "pid,frames,refs,faults,hits,hitrate\n";
            for (size_t i = 0; i < procs.size(); ++i)
            {
                const auto &p = procs[i];
                const auto &ps = stats.per_process[i];
                int refs = (int)p.refs.size();
                double hr = (refs ? (double)ps.hits / (double)refs : 0.0);
                f << p.pid << ","
                  << stats.frames_assigned[i] << ","
                  << refs << ","
                  << ps.faults << ","
                  << ps.hits << ","
                  << hr << "\n";
            }
            return true;
        }

    }
}
