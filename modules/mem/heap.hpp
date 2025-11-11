#pragma once
#include <cstddef>
#include <vector>
#include <list>
#include <unordered_map>
#include <string>
#include <ostream>

namespace os
{
    namespace mem
    {

        struct BuddyStats
        {
            std::size_t total = 0;               // tamaño total del pool
            std::size_t min_block = 0;           // tamaño mínimo de bloque
            std::size_t used_bytes = 0;          // bytes ocupados (suma de bloques asignados)
            std::size_t free_bytes = 0;          // bytes libres (= total - used_bytes)
            std::size_t largest_free_block = 0;  // tamaño del bloque libre más grande
            std::size_t internal_frag_bytes = 0; // desperdicio interno acumulado (asignaciones)
            std::size_t peak_used_bytes = 0;     // pico de uso
            std::size_t alloc_count = 0;
            std::size_t free_count = 0;
            std::size_t fail_count = 0; // fallos de alloc o free inválido
            std::size_t splits = 0;     // particiones realizadas
            std::size_t merges = 0;     // fusiones realizadas
        };

        struct BuddyEvent
        {
            int step = 0;                // contador de evento
            std::string op;              // "alloc" | "free" | "alloc_fail" | "free_fail"
            std::size_t req_bytes = 0;   // bytes solicitados (solo alloc)
            std::size_t block_bytes = 0; // tamaño real del bloque afectado
            int offset = -1;             // desplazamiento del bloque
            bool ok = true;              // éxito/fracaso
            std::size_t used_bytes = 0;  // snapshot tras el evento
            std::size_t free_bytes = 0;  // snapshot tras el evento
            std::size_t largest_free_block = 0;
        };

        class BuddyAllocator
        {
        public:
            BuddyAllocator(std::size_t total_bytes, std::size_t min_block);

            // Reserva al menos nbytes; devuelve offset o -1 si no hay espacio
            int allocate(std::size_t nbytes);

            // Libera un bloque previamente asignado por su offset
            void free_block(int offset);

            // Estado/métricas
            BuddyStats stats() const;

            // Imprime listas libres por orden
            void print_state(std::ostream &out) const;

            // Exporta CSV: eventos (outprefix_events.csv) y snapshot de freelists (outprefix_freelist.csv)
            // Devuelve true si ambos archivos se escribieron bien.
            bool export_csv(const std::string &events_csv_path,
                            const std::string &freelist_csv_path) const;

        private:
            // Helpers de potencia de 2 / órdenes
            static bool is_pow2(std::size_t x);
            static std::size_t next_pow2(std::size_t x);
            static std::size_t round_up(std::size_t x, std::size_t a);

            int order_for_size(std::size_t size) const;  // size = MIN_ << order
            std::size_t size_for_order(int order) const; // MIN_ << order
            int max_order() const;                       // order_for_size(TOTAL_)
            int find_nonempty_from(int start_order) const;

            // Recalcula largest_free_block para los snapshots
            std::size_t compute_largest_free_block() const;

        private:
            // Parámetros
            const std::size_t TOTAL_;
            const std::size_t MIN_;

            // Listas libres por orden: free_lists_[o] contiene offsets de bloques de tamaño size_for_order(o)
            std::vector<std::list<int>> free_lists_;

            // Bloques asignados: offset -> order
            std::unordered_map<int, int> allocated_;

            // Métricas y trazas
            BuddyStats stats_;
            mutable std::vector<BuddyEvent> events_;
            int step_ = 0;
        };

    } // namespace mem
} // namespace os
