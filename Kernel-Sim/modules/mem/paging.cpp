#include "paging.h"
#include <algorithm>
#include <iomanip>

static const char* ANSI_RESET="\033[0m";
static const char* ANSI_DIM="\033[2m";
static const char* ANSI_GREEN="\033[32m";
static const char* ANSI_RED="\033[31m";
static const char* ANSI_CYAN="\033[36m";


Pager::Pager() { setFrames(10); } // default 10 marcos

void Pager::setFrames(int n)
{
    if (n <= 0)
        n = 1;
    frames.assign(n, FrameEntry{});
    // limpiar todo lo demás
    pageTables.clear();
    while (!fifoQueue.empty())
        fifoQueue.pop();
    clock = hits = misses = 0;
    log.clear();
}

void Pager::setPolicy(Policy p)
{
    policy = p;
}

void Pager::setTau(uint64_t t)
{
    if (t == 0)
        t = 1;
    tau = t;
}

void Pager::reset()
{
    for (auto &f : frames)
        f = FrameEntry{};
    pageTables.clear();
    while (!fifoQueue.empty())
        fifoQueue.pop();
    clock = hits = misses = 0;
    log.clear();
}

int Pager::findFreeFrame() const
{
    for (int i = 0; i < (int)frames.size(); ++i)
    {
        if (frames[i].free)
            return i;
    }
    return -1;
}

int Pager::chooseVictimFIFO()
{
    // Si hay libre, úsalo
    int ff = findFreeFrame();
    if (ff >= 0)
        return ff;

    // FIFO clásico: víctima = frente de la cola
    // Depurar cualquier frame libre en la cola (no debería ocurrir)
    while (!fifoQueue.empty())
    {
        int idx = fifoQueue.front();
        if (!frames[idx].free)
        {
            fifoQueue.pop(); // consumimos la víctima
            return idx;
        }
        else
        {
            fifoQueue.pop();
        }
    }
    // fallback: primer frame
    return 0;
}

int Pager::chooseVictimLRU()
{
    int ff = findFreeFrame();
    if (ff >= 0)
        return ff;

    int best = 0;
    uint64_t bestLU = UINT64_MAX;
    for (int i = 0; i < (int)frames.size(); ++i)
    {
        if (!frames[i].free && frames[i].lastUse < bestLU)
        {
            bestLU = frames[i].lastUse;
            best = i;
        }
    }
    return best;
}

int Pager::chooseVictimWS()
{
    int ff = findFreeFrame();
    if (ff >= 0)
        return ff;

    // Primero, preferir páginas FUERA de la ventana (clock - lastUse > tau).
    int candidate = -1;
    uint64_t oldest = 0;
    for (int i = 0; i < (int)frames.size(); ++i)
    {
        if (frames[i].free)
            continue;
        uint64_t age = clock - frames[i].lastUse;
        if (age > tau)
        {
            if (candidate < 0 || frames[i].lastUse < oldest)
            {
                candidate = i;
                oldest = frames[i].lastUse;
            }
        }
    }
    if (candidate >= 0)
        return candidate;

    // Si todos están activos dentro de la ventana, caer a LRU
    return chooseVictimLRU();
}

int Pager::chooseVictim()
{
    switch (policy)
    {
    case Policy::FIFO:
        return chooseVictimFIFO();
    case Policy::LRU:
        return chooseVictimLRU();
    case Policy::WS:
        return chooseVictimWS();
    }
    return chooseVictimLRU();
}

void Pager::unmapFrame(int frameIdx)
{
    FrameEntry &f = frames[frameIdx];
    if (f.free)
        return;
    auto &pt = pageTables[f.pid];
    auto it = pt.find(f.page);
    if (it != pt.end())
    {
        it->second.valid = false;
        it->second.frame = -1;
    }
    f = FrameEntry{}; // queda libre
}

void Pager::mapPageToFrame(int pid, int page, int frameIdx)
{
    FrameEntry &f = frames[frameIdx];
    f.pid = pid;
    f.page = page;
    f.free = false;
    f.loadTime = clock;
    f.lastUse = clock;

    if (policy == Policy::FIFO)
    {
        fifoQueue.push(frameIdx);
    }

    PageTableEntry &pte = pageTables[pid][page];
    pte.valid = true;
    pte.frame = frameIdx;
    pte.loadTime = clock;
    pte.lastUse = clock;
}

bool Pager::access(int pid, int page)
{
    clock += 1;

    PageTableEntry &pte = pageTables[pid][page]; // crea si no existe
    bool hit = false;
    int fidx_after = -1;

    if (pte.valid)
    {
        // hit
        hits += 1;
        hit = true;
        pte.lastUse = clock;
        int fidx = pte.frame;
        frames[fidx].lastUse = clock;
        fidx_after = fidx;
    }
    else
    {
        // miss → cargar/reemplazar
        misses += 1;
        int victim = chooseVictim();
        if (!frames[victim].free)
        {
            unmapFrame(victim);
        }
        mapPageToFrame(pid, page, victim);
        fidx_after = victim;
    }

    // log para CSV
    log.push_back(AccessRecord{clock, pid, page, hit, fidx_after});
    return hit;
}

std::string Pager::show() const
{
    std::ostringstream os;
    os << "=== MEMORIA ===\n";
    os << "Frames: " << frames.size() << " | Policy: ";
    switch (policy)
    {
    case Policy::FIFO:
        os << "FIFO";
        break;
    case Policy::LRU:
        os << "LRU";
        break;
    case Policy::WS:
        os << "WS(tau=" << tau << ")";
        break;
    }
    os << "\n";
    os << "Idx | PID | Page | load | last\n";
    for (int i = 0; i < (int)frames.size(); ++i)
    {
        const auto &f = frames[i];
        if (f.free)
        {
            os << std::setw(3) << i << " |  -  |  -   |  -   |  -\n";
        }
        else
        {
            os << std::setw(3) << i << " | "
               << std::setw(3) << f.pid << " | "
               << std::setw(4) << f.page << " | "
               << std::setw(4) << f.loadTime << " | "
               << std::setw(4) << f.lastUse << "\n";
        }
    }

    // pequeño resumen de algunas tablas activas (hasta 5 pids)
    os << "\n=== TABLAS DE PÁGINAS (resumen) ===\n";
    int count = 0;
    for (const auto &kv : pageTables)
    {
        if (count++ >= 5)
        {
            os << "... (más procesos ocultos)\n";
            break;
        }
        int pid = kv.first;
        os << "PID " << pid << ": ";
        const auto &pt = kv.second;
        int shown = 0;
        for (const auto &kv2 : pt)
        {
            const auto &pte = kv2.second;
            if (!pte.valid)
                continue;
            os << "[p" << kv2.first << "->f" << pte.frame << "] ";
            if (++shown >= 10)
            {
                os << "... ";
                break;
            }
        }
        if (shown == 0)
            os << "(sin páginas válidas)";
        os << "\n";
    }

    return os.str();
}

std::string Pager::stats() const
{
    std::ostringstream os;
    uint64_t total = hits + misses;
    double miss_ratio = (total == 0) ? 0.0 : (double)misses / (double)total;
    double pff = (clock == 0) ? 0.0 : (double)misses / (double)clock; // Page Fault Frequency simple
    os << "=== STATS ===\n";
    os << "Accesos: " << total << " | Hits: " << hits << " | Misses: " << misses
       << " | Miss Ratio: " << std::fixed << std::setprecision(3) << miss_ratio
       << " | PFF: " << std::fixed << std::setprecision(3) << pff << "\n";
    return os.str();
}

bool Pager::dumpCSV(const std::string &path) const
{
    std::ofstream out(path);
    if (!out.is_open())
        return false;
    out << "t,pid,page,hit,frame\n";
    for (const auto &r : log)
    {
        out << r.t << "," << r.pid << "," << r.page << ","
            << (r.hit ? 1 : 0) << "," << r.frame << "\n";
    }
    out.close();
    return true;
}
