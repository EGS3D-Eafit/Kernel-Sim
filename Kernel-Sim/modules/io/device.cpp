#include "device.h"

void Printer::submit(int pid, int pages, int prio)
{
    IOJob j{pid, pages, prio, t};
    if (prio == 0)
        high.push(j);
    else
        norm.push(j);
}

void Printer::tick()
{
    t += 1;
    if (!busy)
    {
        if (!high.empty())
        {
            current = high.front();
            high.pop();
            busy = true;
        }
        else if (!norm.empty())
        {
            current = norm.front();
            norm.pop();
            busy = true;
        }
    }
    if (busy)
    {
        current.pages -= 1;
        if (current.pages <= 0)
        {
            busy = false;
            done += 1;
        }
    }
}

void Printer::run(int n)
{
    for (int i = 0; i < n; i++)
        tick();
}

std::string Printer::show() const
{
    std::ostringstream os;
    os << "=== PRINTER ===\n";
    os << "Time: " << t << " | Busy: " << (busy ? "yes" : "no") << "\n";
    if (busy)
        os << "Current: pid=" << current.pid << " pages_remaining=" << current.pages << " prio=" << current.prio << "\n";
    os << "High queue: " << high.size() << " | Normal queue: " << norm.size() << "\n";
    return os.str();
}

std::string Printer::stats() const
{
    std::ostringstream os;
    os << "=== PRINTER STATS ===\n";
    os << "Trabajos completados: " << done << " | Tiempo: " << t << "\n";
    return os.str();
}
