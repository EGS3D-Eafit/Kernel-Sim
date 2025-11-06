#include "philosophers.h"

void Philosophers::init(int n)
{
    if (n < 2)
        n = 2;
    std::lock_guard<std::mutex> lk(m);
    N = n;
    state.assign(N, Thinking);
    eaten.assign(N, 0);
    eating_count = 0;
}

void Philosophers::tryEat(int i)
{
    // Con camarero: no permitir que todos N estén hambrientos a la vez
    if (eating_count >= N - 1)
        return; // evita deadlock
    // Orden total de palillos: siempre tomar primero el menor índice
    // Simulado: si vecinos no están comiendo, puede comer
    int left = (i + N - 1) % N, right = (i + 1) % N;
    if (state[left] != Eating && state[right] != Eating)
    {
        state[i] = Eating;
        eating_count += 1;
    }
}

void Philosophers::stopEat(int i)
{
    state[i] = Thinking;
    eating_count -= 1;
    eaten[i] += 1;
}

void Philosophers::run(int n_ticks)
{
    std::lock_guard<std::mutex> lk(m);
    if (N == 0)
        return;
    static std::mt19937 rng{12345};
    std::uniform_int_distribution<int> coin(0, 1);

    for (int t = 0; t < n_ticks; ++t)
    {
        for (int i = 0; i < N; ++i)
        {
            if (state[i] == Thinking)
            {
                if (coin(rng) == 1)
                {
                    state[i] = Hungry;
                }
            }
            else if (state[i] == Hungry)
            {
                tryEat(i);
            }
            else if (state[i] == Eating)
            {
                if (coin(rng) == 1)
                {
                    stopEat(i);
                }
            }
        }
    }
}

std::string Philosophers::show() const
{
    std::lock_guard<std::mutex> lk(m);
    std::ostringstream os;
    os << "=== PHILOSOPHERS ===\n";
    for (int i = 0; i < N; ++i)
    {
        char c = (state[i] == Thinking ? 'T' : state[i] == Hungry ? 'H'
                                                                  : 'E');
        os << i << ":" << c << " ";
    }
    os << "\n";
    return os.str();
}

std::string Philosophers::stats() const
{
    std::lock_guard<std::mutex> lk(m);
    std::ostringstream os;
    os << "=== PHILOSOPHERS STATS ===\n";
    for (int i = 0; i < N; ++i)
        os << "P" << i << " comió " << eaten[i] << " veces\n";
    return os.str();
}
