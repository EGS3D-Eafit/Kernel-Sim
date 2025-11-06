#pragma once
#include <queue>
#include <string>
#include <sstream>
#include <cstdint>
#include <vector>

struct IOJob
{
    int pid;
    int pages; // duración en ticks
    int prio;  // 0 = alta, 1 = normal (más alto => menor prioridad real)
    uint64_t submit_t;
};

class Printer
{
public:
    void init()
    {
        while (!high.empty())
            high.pop();
        while (!norm.empty())
            norm.pop();
        current = {};
        busy = false;
        t = 0;
        done = 0;
    }
    void tick(); // avanza 1 tick (si ocupado, consume)
    void submit(int pid, int pages, int prio = 1);
    void run(int n);           // corre n ticks
    std::string show() const;  // estado y colas
    std::string stats() const; // total impresiones
private:
    std::queue<IOJob> high, norm;
    IOJob current{};
    bool busy = false;
    uint64_t t = 0;
    int done = 0;
};
