#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <queue>
#include <optional>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <fstream>

struct PageTableEntry
{
    int frame = -1;
    bool valid = false;
    uint64_t lastUse = 0;  // para LRU / WS
    uint64_t loadTime = 0; // para FIFO
};

struct FrameEntry
{
    int pid = -1;
    int page = -1;
    bool free = true;
    uint64_t lastUse = 0;
    uint64_t loadTime = 0;
};

struct AccessRecord
{
    uint64_t t;
    int pid;
    int page;
    bool hit;
    int frame; // frame tras el acceso (si miss, el frame que recibió la página)
};

class Pager
{
public:
    enum class Policy
    {
        FIFO,
        LRU,
        WS
    }; // WS = Working Set (ventana tau)

    Pager();

    // Configuración
    void setFrames(int n);
    int getFrames() const { return (int)frames.size(); }
    void setPolicy(Policy p);
    Policy getPolicy() const { return policy; }

    // Working Set
    void setTau(uint64_t t);
    uint64_t getTau() const { return tau; }

    // Operación principal: acceso a una página de un proceso
    // Devuelve true si hit, false si fallo.
    bool access(int pid, int page);

    // Estado / métricas
    std::string show() const;  // tablero ASCII de frames y tablas
    std::string stats() const; // fallos, hits, tasas

    // CSV de accesos (t,pid,page,hit,frame)
    bool dumpCSV(const std::string &path) const;

    // Reset total (mantiene policy, frames y tau)
    void reset();

private:
    // Estructuras
    std::vector<FrameEntry> frames;
    std::unordered_map<int, std::unordered_map<int, PageTableEntry>> pageTables;

    // Métricas
    uint64_t clock = 0;
    uint64_t hits = 0;
    uint64_t misses = 0;

    // Reemplazo para FIFO
    std::queue<int> fifoQueue; // índices de frames en orden de carga

    // Working Set
    uint64_t tau = 10; // ventana por defecto (ticks)

    // Log de accesos para CSV
    std::vector<AccessRecord> log;

    Policy policy = Policy::FIFO;

private:
    // Helpers
    int findFreeFrame() const;
    int chooseVictim(); // según policy
    int chooseVictimFIFO();
    int chooseVictimLRU();
    int chooseVictimWS(); // preferir páginas fuera de ventana; si no, LRU
    void mapPageToFrame(int pid, int page, int frameIdx);
    void unmapFrame(int frameIdx);
};
