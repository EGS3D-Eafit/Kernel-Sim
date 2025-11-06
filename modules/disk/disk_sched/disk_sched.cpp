#include "disk_sched.h"
#include <iomanip>
static const char* ANSI_RESET="\033[0m";
static const char* ANSI_YELLOW="\033[33m";
static const char* ANSI_BLUE="\033[34m";


DiskScheduler::DiskScheduler() { init(200, 0); }

void DiskScheduler::init(int cylinders_, int start_head)
{
    cylinders = std::max(1, cylinders_);
    head = std::clamp(start_head, 0, cylinders - 1);
    queue.clear();
    log.clear();
}

void DiskScheduler::setPolicy(Policy p) { policy = p; }

void DiskScheduler::submit(int cyl, uint64_t now)
{
    cyl = std::clamp(cyl, 0, cylinders - 1);
    queue.push_back(DiskOp{cyl, now});
}

int DiskScheduler::chooseSSTF() const
{
    if (queue.empty())
        return -1;
    int best = 0;
    int bestd = std::abs(queue[0].cyl - head);
    for (int i = 1; i < (int)queue.size(); ++i)
    {
        int d = std::abs(queue[i].cyl - head);
        if (d < bestd)
        {
            bestd = d;
            best = i;
        }
    }
    return best;
}

std::vector<int> DiskScheduler::buildSCAN()
{
    // separar a izquierda/derecha de head
    std::vector<int> left, right;
    for (int i = 0; i < (int)queue.size(); ++i)
    {
        if (queue[i].cyl < head)
            left.push_back(i);
        else
            right.push_back(i);
    }
    std::sort(left.begin(), left.end(), [&](int a, int b)
              { return queue[a].cyl > queue[b].cyl; }); // descendente hacia 0
    std::sort(right.begin(), right.end(), [&](int a, int b)
              { return queue[a].cyl < queue[b].cyl; }); // ascendente hacia max
    // estrategia: subir primero (→ cilindros mayores), luego bajar
    std::vector<int> order;
    order.insert(order.end(), right.begin(), right.end());
    order.insert(order.end(), left.begin(), left.end());
    return order;
}

int DiskScheduler::run()
{
    int total_move = 0;
    uint64_t t = log.empty() ? 0 : log.back().t + 1;

    if (queue.empty())
        return 0;

    if (policy == Policy::FCFS)
    {
        for (auto &op : queue)
        {
            int from = head;
            int to = op.cyl;
            int d = std::abs(to - from);
            total_move += d;
            head = to;
            log.push_back(DiskLog{from, to, d, t++});
        }
        queue.clear();
        return total_move;
    }

    if (policy == Policy::SSTF)
    {
        while (!queue.empty())
        {
            int idx = chooseSSTF();
            auto op = queue[idx];
            int from = head, to = op.cyl, d = std::abs(to - from);
            total_move += d;
            head = to;
            log.push_back(DiskLog{from, to, d, t++});
            queue.erase(queue.begin() + idx);
        }
        return total_move;
    }

    // SCAN
    auto order = buildSCAN();
    // para borrar sin invalidar índices, copiamos ops en ese orden
    std::vector<DiskOp> newQ;
    newQ.reserve(order.size());
    for (int i : order)
        newQ.push_back(queue[i]);

    for (auto &op : newQ)
    {
        int from = head, to = op.cyl, d = std::abs(to - from);
        total_move += d;
        head = to;
        log.push_back(DiskLog{from, to, d, t++});
    }
    queue.clear();
    return total_move;
}

std::string DiskScheduler::show() const
{
    std::ostringstream os;
    os << "=== DISK ===\n";
    os << "Cylinders: " << cylinders << " | Head@" << head << " | Policy: ";
    os << (policy == Policy::FCFS ? "FCFS" : policy == Policy::SSTF ? "SSTF"
                                                                    : "SCAN")
       << "\n";
    os << "Queue (" << queue.size() << "): ";
    for (auto &op : queue)
        os << op.cyl << " ";
    if (queue.empty())
        os << "(vacía)";
    os << "\n";
    
    // Visual simple de cilindros (muestra hasta 80 posiciones, remuestreado)
    int width = 80;
    if (cylinders > 0) {
        os << "Visual: [";
        for (int i=0;i<width;i++){
            int cyl = (int)std::lround((double)i*(cylinders-1)/(width-1));
            if (cyl == head) os << ANSI_YELLOW << "|" << ANSI_RESET;
            else os << "-";
        }
        os << "] head=" << head << "\n";
    }
    return os.str();
}

std::string DiskScheduler::stats() const
{
    int total = 0;
    for (auto &r : log)
        total += r.distance;
    std::ostringstream os;
    os << "=== DISK STATS ===\n";
    os << "Requests atendidas: " << log.size() << " | Movimiento total: " << total << "\n";
    if (!log.empty())
    {
        os << "Últimos movimientos:\n";
        int show = std::min<int>(10, (int)log.size());
        for (int i = (int)log.size() - show; i < (int)log.size(); ++i)
        {
            const auto &e = log[i];
            os << "t=" << e.t << " " << e.from << " -> " << e.to << " (+" << e.distance << ")\n";
        }
    }
    
    // Visual simple de cilindros (muestra hasta 80 posiciones, remuestreado)
    int width = 80;
    if (cylinders > 0) {
        os << "Visual: [";
        for (int i=0;i<width;i++){
            int cyl = (int)std::lround((double)i*(cylinders-1)/(width-1));
            if (cyl == head) os << ANSI_YELLOW << "|" << ANSI_RESET;
            else os << "-";
        }
        os << "] head=" << head << "\n";
    }
    return os.str();
}

bool DiskScheduler::dumpCSV(const std::string &path) const
{
    std::ofstream out(path);
    if (!out.is_open())
        return false;
    out << "t,from,to,distance\n";
    for (auto &e : log)
        out << e.t << "," << e.from << "," << e.to << "," << e.distance << "\n";
    return true;
}
