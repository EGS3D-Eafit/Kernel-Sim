#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <random>
#include <string>
#include <sstream>

class Philosophers
{
public:
    void init(int n);
    // corre n "ticks": cada tick, cada filósofo intenta pensar/comer con reglas seguras
    void run(int n_ticks);
    std::string show() const;  // estados
    std::string stats() const; // cuántas veces comió cada uno

private:
    enum State
    {
        Thinking,
        Hungry,
        Eating
    };
    int N = 0;
    mutable std::mutex m;
    std::vector<State> state;
    std::vector<int> eaten;
    // estrategia anti-deadlock: camarero (permitir hasta N-1 comiendo a la vez) + orden total en palillos
    int eating_count = 0;

    void tryEat(int i);
    void stopEat(int i);
};
