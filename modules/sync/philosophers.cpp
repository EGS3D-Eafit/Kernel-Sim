#include "philosophers.hpp"

namespace os
{
    namespace sync
    {

        void DiningPhilosophers::start(std::size_t n, ms think, ms eat, bool verbose)
        {
            stop(); // reiniciar si estaba corriendo

            if (n == 0)
            {
                // nada que hacer
                return;
            }

            {
                std::vector<std::mutex> fresh(n);
                forks_.swap(fresh);
            }

            running_ = true;

            threads_.clear();
            threads_.reserve(n);
            for (std::size_t i = 0; i < n; ++i)
            {
                threads_.emplace_back(&DiningPhilosophers::philosopher_loop,
                                      this, i, think, eat, verbose);
            }
        }

        void DiningPhilosophers::stop()
        {
            if (!running_)
            {
                // Aun así, aseguremos limpieza si quedaran hilos
                for (auto &t : threads_)
                    if (t.joinable())
                        t.join();
                threads_.clear();
                forks_.clear();
                return;
            }

            running_ = false;

            for (auto &t : threads_)
            {
                if (t.joinable())
                    t.join();
            }
            threads_.clear();

            // Liberar todos los mutex reconstruyendo el contenedor
            std::vector<std::mutex> empty;
            forks_.swap(empty);
        }

        void DiningPhilosophers::philosopher_loop(std::size_t id, ms think, ms eat, bool verbose)
        {
            const auto N = forks_.size();
            const auto left = id % N;
            const auto right = (id + 1) % N;

            while (running_)
            {
                if (verbose)
                    std::cout << "[Φ" << id << "] pensando...\n";
                std::this_thread::sleep_for(think);

                // Orden alternado para reducir riesgo de interbloqueo:
                if ((id & 1u) == 0u)
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
