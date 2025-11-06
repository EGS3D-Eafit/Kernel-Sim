#pragma once
#include <vector>
#include <queue>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <fstream>

struct DiskOp
{
    int cyl;
    uint64_t t_submit;
};

struct DiskLog
{
    int from;
    int to;
    int distance;
    uint64_t t;
};

class DiskScheduler
{
public:
    enum class Policy
    {
        FCFS,
        SSTF,
        SCAN
    };

    DiskScheduler();

    // Config
    void init(int cylinders, int start_head);
    void setPolicy(Policy p);
    Policy getPolicy() const { return policy; }

    // Requests
    void submit(int cyl, uint64_t now = 0);
    // Run until cola vacía. Devuelve movimiento total
    int run();

    // Estado
    std::string show() const;
    std::string stats() const;
    bool dumpCSV(const std::string &path) const; // t,from,to,distance

private:
    int cylinders = 200;
    int head = 0;
    Policy policy = Policy::FCFS;
    std::vector<DiskOp> queue; // cola de solicitudes
    std::vector<DiskLog> log;

    // helpers
    int chooseSSTF() const;       // índice en queue
    std::vector<int> buildSCAN(); // orden de servicio para SCAN
};
