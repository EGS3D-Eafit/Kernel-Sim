#include "heap.hpp"
#include <deque>
#include <optional>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <sstream>

namespace os
{
    namespace mem
    {

        // -------------------- Helpers potencias de 2 --------------------
        bool BuddyAllocator::is_pow2(std::size_t x)
        {
            return x > 0 && (x & (x - 1)) == 0;
        }

        std::size_t BuddyAllocator::next_pow2(std::size_t x)
        {
            if (x <= 1)
                return 1;
            if (is_pow2(x))
                return x;
            std::size_t p = 1;
            while (p < x)
                p <<= 1;
            return p;
        }

        std::size_t BuddyAllocator::round_up(std::size_t x, std::size_t a)
        {
            return ((x + a - 1) / a) * a;
        }

        int BuddyAllocator::order_for_size(std::size_t size) const
        {
            // size = MIN_ << order  => order = log2(size / MIN_)
            std::size_t q = size / MIN_;
            int order = 0;
            while ((static_cast<std::size_t>(1) << order) < q)
                order++;
            return order;
        }

        std::size_t BuddyAllocator::size_for_order(int order) const
        {
            return MIN_ << order;
        }

        int BuddyAllocator::max_order() const
        {
            return order_for_size(TOTAL_);
        }

        int BuddyAllocator::find_nonempty_from(int start_order) const
        {
            for (int o = start_order; o <= max_order(); ++o)
            {
                if (!free_lists_[o].empty())
                    return o;
            }
            return -1;
        }

        std::size_t BuddyAllocator::compute_largest_free_block() const
        {
            for (int o = max_order(); o >= 0; --o)
            {
                if (!free_lists_[o].empty())
                    return size_for_order(o);
            }
            return 0;
        }

        // -------------------- Constructor --------------------
        BuddyAllocator::BuddyAllocator(std::size_t total_bytes, std::size_t min_block)
            : TOTAL_(total_bytes), MIN_(min_block)
        {
            if (!is_pow2(TOTAL_) || !is_pow2(MIN_) || MIN_ > TOTAL_)
            {
                throw std::runtime_error("TOTAL y MIN deben ser potencias de 2 y MIN <= TOTAL");
            }
            int levels = order_for_size(TOTAL_) + 1; // órdenes [0..max]
            free_lists_.resize(levels);
            // Inicial: todo libre en el orden máximo (bloque [0, TOTAL_))
            free_lists_[max_order()].push_back(0);

            stats_.total = TOTAL_;
            stats_.min_block = MIN_;
            stats_.used_bytes = 0;
            stats_.free_bytes = TOTAL_;
            stats_.largest_free_block = TOTAL_;
            stats_.internal_frag_bytes = 0;
            stats_.peak_used_bytes = 0;
        }

        // -------------------- Allocate --------------------
        int BuddyAllocator::allocate(std::size_t nbytes)
        {
            // Evento de fallo por tamaño inválido
            if (nbytes == 0 || nbytes > TOTAL_)
            {
                stats_.fail_count++;
                BuddyEvent ev{++step_, "alloc_fail", nbytes, 0, -1, false,
                              stats_.used_bytes, stats_.total - stats_.used_bytes,
                              compute_largest_free_block()};
                events_.push_back(ev);
                return -1;
            }

            const std::size_t need = round_up(nbytes, MIN_);
            std::size_t blockSize = next_pow2(need);
            if (blockSize < MIN_)
                blockSize = MIN_;
            if (blockSize > TOTAL_)
            {
                stats_.fail_count++;
                BuddyEvent ev{++step_, "alloc_fail", nbytes, 0, -1, false,
                              stats_.used_bytes, stats_.total - stats_.used_bytes,
                              compute_largest_free_block()};
                events_.push_back(ev);
                return -1;
            }

            int targetOrder = order_for_size(blockSize);
            int idx = find_nonempty_from(targetOrder);
            if (idx == -1)
            {
                // No hay bloques suficientes
                stats_.fail_count++;
                BuddyEvent ev{++step_, "alloc_fail", nbytes, 0, -1, false,
                              stats_.used_bytes, stats_.total - stats_.used_bytes,
                              compute_largest_free_block()};
                events_.push_back(ev);
                return -1;
            }

            // Tomamos un bloque grande y partimos hasta el objetivo
            int offset = free_lists_[idx].front();
            free_lists_[idx].pop_front();

            int local_splits = 0;
            while (idx > targetOrder)
            {
                idx--;
                std::size_t halfSize = size_for_order(idx);
                int buddyOff = offset + static_cast<int>(halfSize);
                // dejamos la mitad derecha libre en el nivel actual
                free_lists_[idx].push_front(buddyOff);
                local_splits++;
            }

            // Registrar asignación
            allocated_[offset] = idx;

            // Métricas
            const std::size_t blk = size_for_order(idx);
            stats_.alloc_count++;
            stats_.used_bytes += blk;
            if (stats_.used_bytes > stats_.peak_used_bytes)
                stats_.peak_used_bytes = stats_.used_bytes;
            stats_.free_bytes = stats_.total - stats_.used_bytes;
            stats_.internal_frag_bytes += (blk > need ? blk - need : 0);
            stats_.splits += local_splits;
            stats_.largest_free_block = compute_largest_free_block();

            // Evento
            BuddyEvent ev{++step_, "alloc", nbytes, blk, offset, true,
                          stats_.used_bytes, stats_.free_bytes, stats_.largest_free_block};
            events_.push_back(ev);

            return offset;
        }

        // -------------------- Free --------------------
        void BuddyAllocator::free_block(int offset)
        {
            auto it = allocated_.find(offset);
            if (it == allocated_.end())
            {
                // free inválido (doble free o offset inexistente)
                stats_.fail_count++;
                BuddyEvent ev{++step_, "free_fail", 0, 0, offset, false,
                              stats_.used_bytes, stats_.total - stats_.used_bytes,
                              compute_largest_free_block()};
                events_.push_back(ev);
                return;
            }

            int order = it->second;
            allocated_.erase(it);

            // Reducir used_bytes por el bloque originalmente asignado
            const std::size_t blk_bytes = size_for_order(order);
            if (stats_.used_bytes >= blk_bytes)
                stats_.used_bytes -= blk_bytes;
            stats_.free_bytes = stats_.total - stats_.used_bytes;

            // Intentar fusionar repetidamente con su buddy
            int final_order = order;
            int off = offset;

            while (final_order < max_order())
            {
                std::size_t blockSize = size_for_order(final_order);
                int buddy = off ^ static_cast<int>(blockSize); // XOR da el compañero

                auto &lst = free_lists_[final_order];
                auto pos = std::find(lst.begin(), lst.end(), buddy);
                if (pos == lst.end())
                    break; // el buddy no está libre: no hay más fusión

                // quitar buddy y fusionar
                lst.erase(pos);
                off = std::min(off, buddy);
                final_order++;
                stats_.merges++;
            }

            // insertar bloque final ya fusionado
            free_lists_[final_order].push_front(off);

            stats_.free_count++;
            stats_.largest_free_block = compute_largest_free_block();

            // Evento
            BuddyEvent ev{++step_, "free", 0, size_for_order(final_order), off, true,
                          stats_.used_bytes, stats_.total - stats_.used_bytes, stats_.largest_free_block};
            events_.push_back(ev);
        }

        // -------------------- Stats / Print --------------------
        BuddyStats BuddyAllocator::stats() const
        {
            BuddyStats s = stats_;
            s.free_bytes = (s.total >= s.used_bytes ? s.total - s.used_bytes : 0);
            s.largest_free_block = compute_largest_free_block();
            return s;
        }

        void BuddyAllocator::print_state(std::ostream &out) const
        {
            out << "=== Free lists (order -> offsets) ===\n";
            for (int o = max_order(); o >= 0; --o)
            {
                out << "order " << std::setw(2) << o
                    << " (block=" << std::setw(6) << size_for_order(o) << "): ";
                for (int off : free_lists_[o])
                    out << off << ' ';
                out << '\n';
            }
            out << "alloc (offset->order): ";
            if (allocated_.empty())
                out << "(empty)";
            for (auto &kv : allocated_)
                out << "[" << kv.first << "->" << kv.second << "] ";
            out << "\n\n";
        }

        // -------------------- Export CSV --------------------
        bool BuddyAllocator::export_csv(const std::string &events_csv_path,
                                        const std::string &freelist_csv_path) const
        {
            // Events CSV
            {
                std::ofstream ev(events_csv_path);
                if (!ev.is_open())
                    return false;

                ev << "step,op,req_bytes,block_bytes,offset,ok,used_bytes,free_bytes,largest_free_block\n";
                for (const auto &e : events_)
                {
                    ev << e.step << ','
                       << e.op << ','
                       << e.req_bytes << ','
                       << e.block_bytes << ','
                       << e.offset << ','
                       << (e.ok ? 1 : 0) << ','
                       << e.used_bytes << ','
                       << e.free_bytes << ','
                       << e.largest_free_block << '\n';
                }
                ev.close();
            }

            // Snapshot de las free lists (un volcado del estado actual)
            {
                std::ofstream fl(freelist_csv_path);
                if (!fl.is_open())
                    return false;

                fl << "order,block_size,offsets\n";
                for (int o = max_order(); o >= 0; --o)
                {
                    fl << o << ',' << size_for_order(o) << ',';
                    bool first = true;
                    for (int off : free_lists_[o])
                    {
                        if (!first)
                            fl << ' ';
                        fl << off;
                        first = false;
                    }
                    fl << '\n';
                }
                fl.close();
            }

            return true;
        }

    } // namespace mem
} // namespace os
