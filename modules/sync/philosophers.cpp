#include "philosophers.hpp"

namespace os
{
    namespace sync
    {

        void DiningPhilosophers::start(std::size_t n, ms think, ms eat, bool verbose)
        {
            stop(); // reiniciar si estaba corriendo

            forks_.assign(n, std::mutex{});
            running_ = true;

            threads_.reserve(n);
            for (std::size_t i = 0; i < n; ++i)
            {
                threads_.emplace_back(&DiningPhilosophers::philosopher_loop, this, i, think, eat, verbose);
            }
        }

        void DiningPhilosophers::stop()
        {
            if (!running_)
                return;
            running_ = false;
            // No hay CV aquí; la salida ocurre tras el siguiente sleep/iteración
            for (auto &t : threads_)
                if (t.joinable())
                    t.join();
            threads_.clear();
            forks_.clear();
        }

        void DiningPhilosophers::philosopher_loop(std::size_t id, ms think, ms eat, bool verbose)
        {
            auto left = id;
            auto right = (id + 1) % forks_.size();

            while (running_)
            {
                if (verbose)
                    std::cout << "[Φ" << id << "] pensando...\n";
                std::this_thread::sleep_for(think);

                if (id % 2 == 0)
                {
                    forks_[left].lock();
                    forks_[right].lock();
                }
                else
                {
                    forks_[right].lock();
                    forks_[left].lock();
                }

                if (!running_)
                { // por si stop() llegó justo aquí
                    forks_[left].unlock();
                    forks_[right].unlock();
                    break;
                }

                if (verbose)
                    std::cout << "[Φ" << id << "] comiendo\n";
                std::this_thread::sleep_for(eat);

                forks_[left].unlock();
                forks_[right].unlock();
            }
        }

    }
}
