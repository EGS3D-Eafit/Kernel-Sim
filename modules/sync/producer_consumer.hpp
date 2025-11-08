#pragma once
#include "common.hpp"

namespace os
{
    namespace sync
    {

        // Estadísticas simples para reportar a la CLI
        struct PCStats
        {
            std::size_t produced = 0;
            std::size_t consumed = 0;
            std::size_t max_buffer = 0;
            std::size_t last_item = 0;
        };

        // Productor–Consumidor con buffer acotado (semáforos vía mutex+CV)
        class ProducerConsumer
        {
        public:
            ProducerConsumer() = default;
            ~ProducerConsumer() { stop(); }

            // Inicia N productores y M consumidores con un buffer de tamaño B
            void start(std::size_t producers, std::size_t consumers, std::size_t buffer_cap,
                       ms prod_delay = ms(100), ms cons_delay = ms(150), bool verbose = true);

            // Detiene limpios todos los hilos
            void stop();

            // Estadísticas actuales
            PCStats stats() const;

            // ¿Está corriendo?
            bool running() const { return running_; }

        private:
            void producer_loop(std::size_t id, ms delay, bool verbose);
            void consumer_loop(std::size_t id, ms delay, bool verbose);

            // Estado compartido
            mutable std::mutex mtx_;
            std::condition_variable cv_not_full_;
            std::condition_variable cv_not_empty_;
            std::queue<std::size_t> buffer_;
            std::size_t buffer_cap_ = 0;

            // Control de vida
            std::atomic<bool> running_{false};
            std::vector<std::thread> threads_;

            // Métricas
            std::atomic<std::size_t> produced_{0};
            std::atomic<std::size_t> consumed_{0};
            std::atomic<std::size_t> item_counter_{0};
            std::atomic<std::size_t> max_buffer_seen_{0};
        };

    }
}
