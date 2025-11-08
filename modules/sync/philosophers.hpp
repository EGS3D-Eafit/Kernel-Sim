#pragma once
#include "common.hpp"

namespace os
{
    namespace sync
    {

        // Cena de Filósofos con prevención de deadlock alternando el orden de toma
        class DiningPhilosophers
        {
        public:
            DiningPhilosophers() = default;
            ~DiningPhilosophers() { stop(); }

            void start(std::size_t n, ms think = ms(500), ms eat = ms(1000), bool verbose = true);
            void stop();
            bool running() const { return running_; }
            std::size_t size() const { return forks_.size(); }

        private:
            void philosopher_loop(std::size_t id, ms think, ms eat, bool verbose);

            std::vector<std::mutex> forks_;
            std::vector<std::thread> threads_;
            std::atomic<bool> running_{false};
        };

    }
}
