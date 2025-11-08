#include "paging.hpp"
#include <algorithm>
#include <iomanip>
#include <unordered_set>

namespace os
{
    namespace mem
    {

        // Helper para snapshot fijo
        static std::vector<std::optional<int>> snapshot_frames(const std::vector<std::optional<int>> &frames_vec)
        {
            return frames_vec;
        }

        // Helper para snapshot con tamaño dinámico (usado por PFF)
        static std::vector<std::optional<int>>
        resize_snapshot(const std::vector<std::optional<int>> &src, int target)
        {
            std::vector<std::optional<int>> out = src;
            if ((int)out.size() < target)
                out.resize(target, std::nullopt);
            return out;
        }

        // ==========================
        // FIFO y LRU (version fija)
        // ==========================
        PagingStats PagerSimulator::run(const std::vector<int> &pages, int frames, Policy policy)
        {
            PagingStats st;
            st.frames = frames;
            st.steps = static_cast<int>(pages.size());
            st.ref_string = pages;
            st.frame_table.reserve(st.steps);
            st.fault_flags.reserve(st.steps);

            std::vector<std::optional<int>> frames_vec(frames, std::nullopt);

            std::queue<int> fifo_q;
            std::list<int> lru_list;
            std::unordered_map<int, std::list<int>::iterator> pos;

            auto in_frames = [&](int page)
            {
                return std::find_if(frames_vec.begin(), frames_vec.end(),
                                    [&](const std::optional<int> &x)
                                    { return x.has_value() && x.value() == page; }) != frames_vec.end();
            };

            auto free_slot = [&]() -> int
            {
                for (int i = 0; i < frames; i++)
                    if (!frames_vec[i].has_value())
                        return i;
                return -1;
            };

            for (int t = 0; t < st.steps; ++t)
            {
                int page = pages[t];
                bool hit = in_frames(page);

                if (policy == Policy::LRU)
                {
                    if (hit)
                    {
                        auto it = pos[page];
                        lru_list.erase(it);
                        lru_list.push_front(page);
                        pos[page] = lru_list.begin();
                    }
                    else
                    {
                        st.faults++;
                        int idx = free_slot();
                        if (idx >= 0)
                        {
                            frames_vec[idx] = page;
                        }
                        else
                        {
                            int lruPage = lru_list.back();
                            lru_list.pop_back();
                            pos.erase(lruPage);
                            for (int i = 0; i < frames; i++)
                            {
                                if (frames_vec[i].has_value() && frames_vec[i].value() == lruPage)
                                {
                                    frames_vec[i] = page;
                                    break;
                                }
                            }
                        }
                        lru_list.push_front(page);
                        pos[page] = lru_list.begin();
                    }
                }
                else if (policy == Policy::FIFO)
                {
                    if (!hit)
                    {
                        st.faults++;
                        int idx = free_slot();
                        if (idx >= 0)
                        {
                            frames_vec[idx] = page;
                            fifo_q.push(page);
                        }
                        else
                        {
                            int victim = fifo_q.front();
                            fifo_q.pop();
                            for (int i = 0; i < frames; i++)
                            {
                                if (frames_vec[i].has_value() && frames_vec[i].value() == victim)
                                {
                                    frames_vec[i] = page;
                                    break;
                                }
                            }
                            fifo_q.push(page);
                        }
                    }
                }

                if (hit)
                    st.hits++;
                st.fault_flags.push_back(!hit);
                st.frame_table.push_back(snapshot_frames(frames_vec));
            }

            return st;
        }

        // ==========================
        // PFF (Page Fault Frequency)
        // ==========================
        PagingStats PagerSimulator::run_pff(const std::vector<int> &pages, const PFFParams &p)
        {
            PagingStats st;
            st.steps = (int)pages.size();
            st.ref_string = pages;
            st.frames = p.frames_init;
            st.max_frames_observed = p.frames_init;
            st.frame_table.reserve(st.steps);
            st.fault_flags.reserve(st.steps);
            st.frames_per_step.reserve(st.steps);

            int marcos_asignados = p.frames_init;
            std::vector<std::optional<int>> frames_vec(marcos_asignados, std::nullopt);
            std::unordered_set<int> in_mem;
            std::deque<int> fifo_order;

            auto has_free_slot = [&]() -> int
            {
                for (int i = 0; i < marcos_asignados; i++)
                    if (!frames_vec[i].has_value())
                        return i;
                return -1;
            };

            std::deque<int> ventana_idx;
            auto recalc_recent_faults = [&](int t) -> int
            {
                std::unordered_set<int> vistos;
                int fallos = 0;
                for (int k = (int)ventana_idx.size() - 1; k >= 0; --k)
                {
                    int j = ventana_idx[k];
                    int pag = pages[j];
                    if (vistos.insert(pag).second)
                    {
                        if (!in_mem.count(pag))
                            ++fallos;
                    }
                }
                return fallos;
            };

            for (int t = 0; t < st.steps; ++t)
            {
                int page = pages[t];
                bool hit = in_mem.count(page) > 0;

                if (!hit)
                {
                    st.faults++;

                    int free_i = has_free_slot();
                    if (free_i >= 0)
                    {
                        frames_vec[free_i] = page;
                        in_mem.insert(page);
                        fifo_order.push_back(page);
                    }
                    else
                    {
                        int victim = fifo_order.front();
                        fifo_order.pop_front();
                        for (int i = 0; i < marcos_asignados; i++)
                        {
                            if (frames_vec[i].has_value() && frames_vec[i].value() == victim)
                            {
                                frames_vec[i] = page;
                                break;
                            }
                        }
                        in_mem.erase(victim);
                        in_mem.insert(page);
                        fifo_order.push_back(page);
                    }
                }
                else
                {
                    st.hits++;
                }

                ventana_idx.push_back(t);
                while ((int)ventana_idx.size() > p.window_size)
                    ventana_idx.pop_front();

                int fallos_recientes = recalc_recent_faults(t);

                // Ajuste de marcos
                if (fallos_recientes >= p.fault_thresh && marcos_asignados < p.frames_max)
                {
                    marcos_asignados += 1;
                    frames_vec.resize(marcos_asignados, std::nullopt);
                }
                else if (fallos_recientes == 0 && marcos_asignados > p.frames_min)
                {
                    bool top_empty = frames_vec.back().has_value() == false;
                    if (!top_empty)
                    {
                        int victim = fifo_order.front();
                        fifo_order.pop_front();
                        for (int i = 0; i < marcos_asignados; i++)
                        {
                            if (frames_vec[i].has_value() && frames_vec[i].value() == victim)
                            {
                                frames_vec[i] = std::nullopt;
                                break;
                            }
                        }
                        in_mem.erase(victim);
                    }
                    marcos_asignados -= 1;
                    frames_vec.resize(marcos_asignados);
                }

                st.max_frames_observed = std::max(st.max_frames_observed, marcos_asignados);
                st.frames_per_step.push_back(marcos_asignados);

                st.frame_table.push_back(resize_snapshot(frames_vec, st.max_frames_observed));
                st.fault_flags.push_back(!hit);
            }

            for (auto &col : st.frame_table)
            {
                if ((int)col.size() < st.max_frames_observed)
                {
                    col.resize(st.max_frames_observed, std::nullopt);
                }
            }
            return st;
        }

        // ===========================
        // Visualización / Exportación
        // ===========================
        void PagerSimulator::print_ascii(const PagingStats &s, std::ostream &out)
        {
            out << "Referencia: ";
            for (int p : s.ref_string)
                out << p << " ";
            out << "\n";

            out << "Frames(init)=" << s.frames
                << "  Pasos=" << s.steps
                << "  Hits=" << s.hits
                << "  Faults=" << s.faults
                << "  HitRate=" << std::fixed << std::setprecision(2)
                << (s.steps ? (100.0 * s.hits / s.steps) : 0.0) << "%\n";

            int F = (s.max_frames_observed > 0 ? s.max_frames_observed : s.frames);

            for (int f = 0; f < F; ++f)
            {
                out << "F" << f << " | ";
                for (int t = 0; t < s.steps; ++t)
                {
                    auto v = (t < (int)s.frame_table.size() && f < (int)s.frame_table[t].size())
                                 ? s.frame_table[t][f]
                                 : std::optional<int>{};
                    if (v.has_value())
                        out << std::setw(3) << v.value();
                    else
                        out << std::setw(3) << '.';
                    out << ' ';
                }
                out << '\n';
            }

            out << "FL | ";
            for (int t = 0; t < s.steps; ++t)
                out << " " << (s.fault_flags[t] ? 'X' : '.') << "  ";
            out << "\n";

            if (!s.frames_per_step.empty())
            {
                out << "FC | ";
                for (int t = 0; t < s.steps; ++t)
                    out << std::setw(3) << s.frames_per_step[t] << ' ';
                out << "(frames por paso)\n";
            }
        }

        bool PagerSimulator::export_csv(const PagingStats &s,
                                        const std::string &timeline_csv_path,
                                        const std::string &events_csv_path)
        {
            int F = (s.max_frames_observed > 0 ? s.max_frames_observed : s.frames);

            std::ofstream tl(timeline_csv_path);
            if (!tl.is_open())
                return false;
            tl << "step";
            for (int f = 0; f < F; ++f)
                tl << ",frame_" << f;
            if (!s.frames_per_step.empty())
                tl << ",frames_used";
            tl << "\n";
            for (int t = 0; t < s.steps; ++t)
            {
                tl << t;
                for (int f = 0; f < F; ++f)
                {
                    auto v = (t < (int)s.frame_table.size() && f < (int)s.frame_table[t].size())
                                 ? s.frame_table[t][f]
                                 : std::optional<int>{};
                    if (v.has_value())
                        tl << "," << v.value();
                    else
                        tl << ",";
                }
                if (!s.frames_per_step.empty())
                    tl << "," << s.frames_per_step[t];
                tl << "\n";
            }
            tl.close();

            std::ofstream ev(events_csv_path);
            if (!ev.is_open())
                return false;
            ev << "step,page,hit,fault";
            if (!s.frames_per_step.empty())
                ev << ",frames_used";
            ev << "\n";
            for (int t = 0; t < s.steps; ++t)
            {
                int page = s.ref_string[t];
                bool fault = s.fault_flags[t];
                bool hit = !fault;
                ev << t << "," << page << "," << (hit ? 1 : 0) << "," << (fault ? 1 : 0);
                if (!s.frames_per_step.empty())
                    ev << "," << s.frames_per_step[t];
                ev << "\n";
            }
            ev.close();
            return true;
        }

    } // namespace mem
} // namespace os
