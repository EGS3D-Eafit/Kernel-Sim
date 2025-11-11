#include <bits/stdc++.h>
#include <span>
#ifdef _WIN32
#include <windows.h>
#include <clocale>
#endif

using namespace std;

// ====== módulos del proyecto ======
#include "../modules/cpu/process.hpp"
#include "../modules/cpu/scheduler.hpp"
#include "../modules/mem/paging.hpp"
#include "../modules/mem/allocation.hpp"
#include "../modules/disk/scheduling.hpp"
#include "../modules/io/printer.hpp"
#include "../modules/sync/producer_consumer.hpp"
#include "../modules/sync/philosophers.hpp"

// ====== utilidades CLI (ANSI colores y helpers) ======
namespace ansi
{
    const string reset = "\033[0m";
    const string bold = "\033[1m";
    const string dim = "\033[2m";
    const string red = "\033[31m";
    const string green = "\033[32m";
    const string yellow = "\033[33m";
    const string blue = "\033[34m";
    const string magenta = "\033[35m";
    const string cyan = "\033[36m";
    const string gray = "\033[90m";
}

static vector<string> split(const string &line)
{
    vector<string> out;
    string tok;
    istringstream iss(line);
    while (iss >> tok)
        out.push_back(tok);
    return out;
}
static vector<int> parse_ints(const vector<string> &v)
{
    vector<int> out;
    out.reserve(v.size());
    for (auto &s : v)
        out.push_back(stoi(s));
    return out;
}
static void print_ok(const string &s) { std::cout << ansi::green << "✓ " << s << ansi::reset << "\n"; }
static void print_err(const string &s) { std::cerr << ansi::red << "✗ " << s << ansi::reset << "\n"; }

// ====== vistas especializadas ======
static void mem_view_colored(const os::mem::PagingStats &s)
{
    using namespace os::mem;
    int F = (s.max_frames_observed > 0 ? s.max_frames_observed : s.frames);

    cout << "Referencia: ";
    for (int p : s.ref_string)
        cout << p << " ";
    cout << "\n";
    cout << "Frames(init)=" << s.frames
         << "  Pasos=" << s.steps
         << "  Hits=" << s.hits
         << "  Faults=" << s.faults
         << "  HitRate=" << fixed << setprecision(2)
         << (s.steps ? (100.0 * s.hits / s.steps) : 0.0) << "%\n";

    // Tabla con colores: verde si hit en ese paso; rojo si fallo.
    for (int f = 0; f < F; ++f)
    {
        cout << "F" << f << " | ";
        for (int t = 0; t < s.steps; ++t)
        {
            bool fault = s.fault_flags[t];
            auto v = (t < (int)s.frame_table.size() && f < (int)s.frame_table[t].size())
                         ? s.frame_table[t][f]
                         : std::optional<int>{};
            string color = fault ? ansi::red : ansi::green;
            if (v.has_value())
                cout << color << setw(3) << v.value() << ansi::reset;
            else
                cout << ansi::gray << setw(3) << '.' << ansi::reset;
            cout << ' ';
        }
        cout << '\n';
    }
    cout << "FL | ";
    for (int t = 0; t < s.steps; ++t)
    {
        cout << (s.fault_flags[t] ? ansi::red : ansi::green)
             << " " << (s.fault_flags[t] ? 'X' : '.') << "  " << ansi::reset;
    }
    cout << "\n";
    if (!s.frames_per_step.empty())
    {
        cout << "FC | ";
        for (int t = 0; t < s.steps; ++t)
            cout << setw(3) << s.frames_per_step[t] << ' ';
        cout << "(frames por paso)\n";
    }
}

static void disk_view_headline(const os::disk::DiskStats &st, int min_cyl, int max_cyl)
{
    using namespace ansi;
    if (st.head_path.empty())
    {
        cout << "(sin trayectoria)\n";
        return;
    }

    int last = st.head_path.back();
    int L = min_cyl, R = max_cyl;
    if (L > R)
        swap(L, R);
    L = min(L, last);
    R = max(R, last);

    // línea de cilindros
    cout << "Cilindros: " << L << " … " << R << "\n";
    // ASCII: marcar head final
    int width = 60;
    auto mapx = [&](int c)
    {
        if (R == L)
            return 0;
        double u = double(c - L) / double(R - L);
        int x = int(round(u * (width - 1)));
        return max(0, min(width - 1, x));
    };
    string line(width, '-');
    line[mapx(last)] = '|';
    cout << gray << line << reset << "  (" << blue << "head@" << last << reset << ")\n";

    // trayectoria resumida
    cout << "Trayectoria: ";
    for (size_t i = 0; i < st.head_path.size(); ++i)
    {
        if (i)
            cout << " -> ";
        cout << (i + 1 == st.head_path.size() ? (blue + to_string(st.head_path[i]) + ansi::reset) : to_string(st.head_path[i]));
    }
    cout << "\nMovimiento total: " << ansi::magenta << st.total_movement << ansi::reset << " cilindros\n";
}

// ====== ayuda ======
static void print_help()
{
    using namespace ansi;
    cout << bold << "Comandos disponibles" << reset << R"(
# Procesos (ProcessManager, interrupciones)
  proc create <name> <burst> <arrival>
  proc suspend <pid>
  proc resume  <pid>
  proc block   <pid> <ticks>
  proc term    <pid>
  proc run     <ticks>
  proc table
  proc queues

# Planificador (SJF/RR)
  sched sjf  <id,arrival,burst>...         # ej: sched sjf 1,0,8 2,1,4 3,2,9 4,3,5
  sched rr   <quantum> <id,arrival,burst>  # ej: sched rr 2 1,0,5 2,1,3 3,2,8 4,3,6

# Memoria (FIFO/LRU/PFF)
  mem fifo <frames> <seq...>
  mem lru  <frames> <seq...>
  mem pff  <frames_init> <frames_max> <win> <thresh> <seq...>
  mem csv  <policy:fifo|lru|pff> <outprefix> <args... como arriba>

# Asignación de marcos (por proceso, reemplazo local)
  alloc equal <total_frames> <pid:[refs...]>...
  alloc prop  <total_frames> <pid:[refs...]>...

# Disco (FCFS/SSTF)
  disk fcfs <head_start> <id,arrival,service,cylinder>...
  disk sstf <head_start> <id,arrival,service,cylinder>...
  disk csv  <algo:fcfs|sstf> <out.csv> <head_start> <...igual arriba>

# Sincronización (Productor–Consumidor)
  sync pc start <prod> <cons> <cap> [prod_ms] [cons_ms] [--quiet]
  sync pc stat
  sync pc stop

# Sincronización (Filósofos comensales)
  sync ph start <n_filosofos> [think_ms] [eat_ms] [--quiet]
  sync ph stat
  sync ph stop


# Otros
  help
  exit/quit
)" << "\n";
}

// ====== CLI principal ======
int main()
{
#ifdef _WIN32
    // Fuerza la consola a UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setlocale(LC_ALL, ".UTF-8");
#endif
    using namespace os;

    // Instancia de productor-consumidor para el CLI
    os::sync::ProducerConsumer pc;

    // Instancia de filósofos de Dining para el CLI
    os::sync::DiningPhilosophers ph;

    // Estado “vivo” del kernel simulado
    cpu::ProcessManager pm;

    cout << ansi::bold << "Kernel-Sim CLI" << ansi::reset
         << "\nescribe 'help' para ayuda\n";

    string line;
    cout << "os> " << flush;
    while (getline(cin, line))
    {
        auto t = split(line);
        if (t.empty())
        {
            cout << "os> ";
            continue;
        }

        try
        {
            // ==== salir/ayuda ====
            if (t[0] == "exit" || t[0] == "quit")
            {
                ph.stop();
                pc.stop();
                break;
            }

            if (t[0] == "help")
            {
                print_help();
                cout << "os> ";
                continue;
            }

            // ==== Impresora  ====
            if (t[0] == "printer")
            {
                if (t.size() < 2)
                {
                    print_err("uso: printer demo [N]");
                    cout << "os> ";
                    continue;
                }
                if (t[1] == "demo")
                {
                    int n = (t.size() >= 3 ? stoi(t[2]) : 5);
                    os::io::demo_printer(n);
                }
                else
                {
                    print_err("subcomando printer desconocido");
                }
                cout << "os> ";
                continue;
            }

            // ==== Procesos ====
            if (t[0] == "proc")
            {
                if (t.size() < 2)
                {
                    print_err("falta subcomando");
                    cout << "os> ";
                    continue;
                }
                string sub = t[1];

                if (sub == "create")
                {
                    if (t.size() != 5)
                    {
                        print_err("uso: proc create <name> <burst> <arrival>");
                    }
                    else
                    {
                        string name = t[2];
                        int burst = stoi(t[3]);
                        int arr = stoi(t[4]);
                        int pid = pm.create_process(name, burst, arr);
                        print_ok("pid=" + to_string(pid) + " creado");
                    }
                }
                else if (sub == "suspend")
                {
                    if (t.size() != 3)
                        print_err("uso: proc suspend <pid>");
                    else
                        print_ok(pm.suspend(stoi(t[2])) ? "suspendido" : "no se pudo");
                }
                else if (sub == "resume")
                {
                    if (t.size() != 3)
                        print_err("uso: proc resume <pid>");
                    else
                        print_ok(pm.resume(stoi(t[2])) ? "reanudado" : "no se pudo");
                }
                else if (sub == "term")
                {
                    if (t.size() != 3)
                        print_err("uso: proc term <pid>");
                    else
                        print_ok(pm.terminate(stoi(t[2])) ? "terminado" : "no se pudo");
                }
                else if (sub == "block")
                {
                    if (t.size() != 4)
                        print_err("uso: proc block <pid> <ticks>");
                    else
                        print_ok(pm.block_for(stoi(t[2]), stoi(t[3])) ? "bloqueado" : "no se pudo");
                }
                else if (sub == "run")
                {
                    if (t.size() != 3)
                        print_err("uso: proc run <ticks>");
                    else
                    {
                        pm.run(stoi(t[2]));
                        print_ok("now=" + to_string(pm.now()));
                    }
                }
                else if (sub == "table")
                {
                    pm.print_table();
                }
                else if (sub == "queues")
                {
                    pm.print_queues();
                }
                else
                {
                    print_err("subcomando proc desconocido");
                }
                cout << "os> ";
                continue;
            }

            // ==== Schedulers independientes (entrada rápida) ====
            if (t[0] == "sched")
            {
                if (t.size() < 3)
                {
                    print_err("uso: sched <sjf|rr> ...");
                    cout << "os> ";
                    continue;
                }
                string kind = t[1];

                if (kind == "sjf")
                {
                    std::vector<cpu::Process> v;

                    for (size_t i = 2; i < t.size(); ++i)
                    {
                        // formato id,arr,burst
                        auto s = t[i];
                        std::replace(s.begin(), s.end(), ',', ' ');

                        std::istringstream iss(s);
                        int id, arrival, burst;
                        if (!(iss >> id >> arrival >> burst))
                        {
                            print_err("Entrada inválida: usa id,arrival,burst (ej: 1,0,8)");
                            v.clear();
                            break;
                        }

                        // Validación de dominio
                        if (id <= 0 || arrival < 0 || burst <= 0)
                        {
                            std::ostringstream msg;
                            msg << "Datos inválidos para proceso '" << s
                                << "'. Reglas: id>0, llegada>=0, duracion>0.";
                            print_err(msg.str());
                            v.clear();
                            break;
                        }

                        v.push_back({id, arrival, burst});
                    }

                    if (v.empty())
                    {
                        print_err("No se ejecutó SJF por entradas inválidas.");
                    }
                    else
                    {
                        auto stats = cpu::CPUScheduler::sjf_non_preemptive(v);
                        cpu::CPUScheduler::print_table(v, stats);
                        std::cout << "Fairness (Jain): " << std::fixed << std::setprecision(4)
                                  << stats.jain_fairness << "\n";
                    }
                }

                else if (kind == "rr")
                {
                    if (t.size() < 4)
                    {
                        print_err("uso: sched rr <quantum> <id,arr,burst>...");
                        std::cout << "os> ";
                        continue;
                    }

                    int quantum = 0;
                    try
                    {
                        quantum = std::stoi(t[2]);
                    }
                    catch (...)
                    {
                        quantum = 0;
                    }

                    if (quantum <= 0)
                    {
                        print_err("Quantum inválido: debe ser > 0.");
                        std::cout << "os> ";
                        continue;
                    }

                    std::vector<cpu::Process> v;
                    for (size_t i = 3; i < t.size(); ++i)
                    {
                        auto s = t[i];
                        std::replace(s.begin(), s.end(), ',', ' ');
                        std::istringstream iss(s);
                        int id, arrival, burst;
                        if (!(iss >> id >> arrival >> burst))
                        {
                            print_err("Entrada inválida: usa id,arr,burst (ej: 2,0,3)");
                            v.clear();
                            break;
                        }
                        if (id <= 0 || arrival < 0 || burst <= 0)
                        {
                            std::ostringstream msg;
                            msg << "Datos inválidos para proceso '" << s
                                << "'. Reglas: id>0, llegada>=0, duracion>0.";
                            print_err(msg.str());
                            v.clear();
                            break;
                        }
                        v.push_back({id, arrival, burst});
                    }

                    if (v.empty())
                    {
                        print_err("No se ejecutó RR por entradas inválidas.");
                    }
                    else
                    {
                        auto stats = cpu::CPUScheduler::round_robin(v, quantum);
                        cpu::CPUScheduler::print_table(v, stats);
                        std::cout << "Fairness (Jain): " << std::fixed << std::setprecision(4)
                                  << stats.jain_fairness << "\n";
                    }
                }

                else
                {
                    print_err("planificador desconocido");
                }
                cout << "os> ";
                continue;
            }

            // ==== Memoria: FIFO / LRU / PFF ====
            if (t[0] == "mem")
            {
                if (t.size() < 3)
                {
                    print_err("uso: mem <fifo|lru|pff|csv> ...");
                    cout << "os> ";
                    continue;
                }
                string sub = t[1];

                if (sub == "fifo" || sub == "lru")
                {
                    int frames = stoi(t[2]);
                    vector<int> seq;
                    for (size_t i = 3; i < t.size(); ++i)
                        seq.push_back(stoi(t[i]));
                    auto policy = (sub == "fifo" ? mem::Policy::FIFO : mem::Policy::LRU);
                    auto st = mem::PagerSimulator::run(seq, frames, policy);
                    mem_view_colored(st);
                }
                else if (sub == "pff")
                {
                    if (t.size() < 7)
                    {
                        print_err("uso: mem pff <frames_init> <frames_max> <win> <thresh> <seq...>");
                        cout << "os> ";
                        continue;
                    }
                    mem::PFFParams p;
                    p.frames_init = stoi(t[2]);
                    p.frames_max = stoi(t[3]);
                    p.window_size = stoi(t[4]);
                    p.fault_thresh = stoi(t[5]);
                    vector<int> seq;
                    for (size_t i = 6; i < t.size(); ++i)
                        seq.push_back(stoi(t[i]));
                    auto st = mem::PagerSimulator::run_pff(seq, p);
                    mem_view_colored(st);
                }
                else if (sub == "csv")
                {
                    if (t.size() < 5)
                    {
                        print_err("uso: mem csv <fifo|lru|pff> <outprefix> <args...>");
                        cout << "os> ";
                        continue;
                    }
                    string pol = t[2], outp = t[3];
                    if (pol == "fifo" || pol == "lru")
                    {
                        if (t.size() < 6)
                        {
                            print_err("uso: mem csv fifo|lru <outprefix> <frames> <seq...>");
                            cout << "os> ";
                            continue;
                        }
                        int frames = stoi(t[4]);
                        vector<int> seq;
                        for (size_t i = 5; i < t.size(); ++i)
                            seq.push_back(stoi(t[i]));
                        auto policy = (pol == "fifo" ? mem::Policy::FIFO : mem::Policy::LRU);
                        auto st = mem::PagerSimulator::run(seq, frames, policy);
                        bool ok = mem::PagerSimulator::export_csv(st, outp + "_timeline.csv", outp + "_events.csv");
                        ok ? print_ok("CSV exportado") : print_err("no se pudo exportar CSV");
                    }
                    else if (pol == "pff")
                    {
                        if (t.size() < 8)
                        {
                            print_err("uso: mem csv pff <outprefix> <frames_init> <frames_max> <win> <thresh> <seq...>");
                            cout << "os> ";
                            continue;
                        }
                        mem::PFFParams p;
                        p.frames_init = stoi(t[4]);
                        p.frames_max = stoi(t[5]);
                        p.window_size = stoi(t[6]);
                        p.fault_thresh = stoi(t[7]);
                        vector<int> seq;
                        for (size_t i = 8; i < t.size(); ++i)
                            seq.push_back(stoi(t[i]));
                        auto st = mem::PagerSimulator::run_pff(seq, p);
                        bool ok = mem::PagerSimulator::export_csv(st, outp + "_timeline.csv", outp + "_events.csv");
                        ok ? print_ok("CSV exportado") : print_err("no se pudo exportar CSV");
                    }
                    else
                    {
                        print_err("política desconocida");
                    }
                }
                else
                {
                    print_err("subcomando mem desconocido");
                }
                cout << "os> ";
                continue;
            }

            // ==== Asignación de marcos (igual/proporcional) ====
            if (t[0] == "alloc")
            {
                if (t.size() < 4)
                {
                    print_err("uso: alloc <equal|prop> <total_frames> <pid:[refs...]>...");
                    cout << "os> ";
                    continue;
                }
                string kind = t[1];
                int total_frames = stoi(t[2]);
                vector<mem::ProcRefs> procs;
                for (size_t i = 3; i < t.size(); ++i)
                {
                    // formato pid:[a,b,c]
                    string s = t[i];
                    auto pos = s.find(':');
                    if (pos == string::npos)
                    {
                        print_err("formato pid:[refs]");
                        continue;
                    }
                    int pid = stoi(s.substr(0, pos));
                    string rs = s.substr(pos + 1);
                    // eliminar [ ]
                    rs.erase(remove(rs.begin(), rs.end(), '['), rs.end());
                    rs.erase(remove(rs.begin(), rs.end(), ']'), rs.end());
                    replace(rs.begin(), rs.end(), ',', ' ');
                    istringstream iss(rs);
                    int x;
                    vector<int> refs;
                    while (iss >> x)
                        refs.push_back(x);
                    procs.push_back({pid, refs});
                }
                auto policy = (kind == "equal" ? mem::FrameAllocPolicy::EQUAL : mem::FrameAllocPolicy::PROPORTIONAL);
                auto stats = mem::FrameAllocator::simulate(procs, total_frames, policy, mem::Policy::LRU);
                mem::FrameAllocator::print_report(procs, stats);
                cout << "os> ";
                continue;
            }

            // ==== Disco ====
            if (t[0] == "disk")
            {
                if (t.size() < 3)
                {
                    print_err("uso: disk <fcfs|sstf|csv> ...");
                    cout << "os> ";
                    continue;
                }
                string sub = t[1];

                auto parse_reqs = [&](span<const string> sv)
                {
                    vector<disk::DiskRequest> r;
                    for (auto &tok : sv)
                    {
                        // id,arr,svc,cyl
                        string s = tok;
                        replace(s.begin(), s.end(), ',', ' ');
                        istringstream iss(s);
                        disk::DiskRequest d{};
                        iss >> d.id >> d.arrival >> d.service >> d.cylinder;
                        r.push_back(d);
                    }
                    return r;
                };

                if (sub == "fcfs" || sub == "sstf")
                {
                    if (t.size() < 4)
                    {
                        print_err("uso: disk <fcfs|sstf> <head_start> <id,arr,svc,cyl>...");
                        cout << "os> ";
                        continue;
                    }
                    int h0 = stoi(t[2]);
                    auto reqs = parse_reqs({t.begin() + 3, t.end()});
                    os::disk::DiskStats stats;
                    vector<disk::DiskRequest> reqs_for_table;

                    if (sub == "fcfs")
                    {
                        reqs_for_table = reqs;
                        stats = disk::DiskScheduler::fcfs(reqs_for_table, h0);
                        disk::DiskScheduler::print_table(reqs_for_table, stats);
                    }
                    else
                    {
                        stats = disk::DiskScheduler::sstf(reqs, h0);
                        // reconstruir tabla en orden de servicio
                        for (auto &id : stats.service_order)
                        {
                            auto it = find_if(reqs.begin(), reqs.end(), [&](auto &x)
                                              { return x.id == id; });
                            if (it != reqs.end())
                                reqs_for_table.push_back(*it);
                        }
                        // calcular tiempos para imprimir
                        int time = 0;
                        for (auto &r : reqs_for_table)
                        {
                            r.start_time = max(time, r.arrival);
                            r.completion = r.start_time + r.service;
                            r.turnaround = r.completion - r.arrival;
                            r.waiting = r.turnaround - r.service;
                            time = r.completion;
                        }
                        disk::DiskScheduler::print_table(reqs_for_table, stats);
                    }
                    // Línea de cilindros + head
                    int minc = h0, maxc = h0;
                    for (auto &r : reqs_for_table)
                    {
                        minc = min(minc, r.cylinder);
                        maxc = max(maxc, r.cylinder);
                    }
                    disk_view_headline(stats, minc, maxc);
                }
                else if (sub == "csv")
                {
                    if (t.size() < 6)
                    {
                        print_err("uso: disk csv <fcfs|sstf> <outfile.csv> <head_start> <reqs...>");
                        cout << "os> ";
                        continue;
                    }
                    string algo = t[2];
                    string outcsv = t[3];
                    int h0 = stoi(t[4]);
                    auto reqs = parse_reqs({t.begin() + 5, t.end()});
                    os::disk::DiskStats stats;
                    vector<disk::DiskRequest> dummy = reqs;
                    if (algo == "fcfs")
                        stats = disk::DiskScheduler::fcfs(dummy, h0);
                    else
                        stats = disk::DiskScheduler::sstf(reqs, h0);
                    bool ok = disk::DiskScheduler::export_head_csv(stats, outcsv);
                    ok ? print_ok("CSV exportado") : print_err("no se pudo exportar");
                }
                else
                {
                    print_err("subcomando disk desconocido");
                }
                cout << "os> ";
                continue;
            }

            // ==== Sincronización: productor–consumidor ====
            if (t[0] == "sync")
            {
                if (t.size() < 2)
                {
                    print_err("uso: sync pc <start|stat|stop> ...");
                    std::cout << "os> ";
                    continue;
                }
                if (t[1] != "pc")
                {
                    print_err("solo 'pc' está soportado por ahora");
                    std::cout << "os> ";
                    continue;
                }
                // subcomandos: start / stat / stop
                if (t.size() >= 3 && t[2] == "start")
                {
                    if (t.size() < 6)
                    {
                        print_err("uso: sync pc start <prod> <cons> <cap> [prod_ms] [cons_ms] [--quiet]");
                        std::cout << "os> ";
                        continue;
                    }
                    std::size_t prod = std::stoul(t[3]);
                    std::size_t cons = std::stoul(t[4]);
                    std::size_t cap = std::stoul(t[5]);

                    // opcionales
                    int prod_ms = (t.size() >= 7 ? std::stoi(t[6]) : 100);
                    int cons_ms = (t.size() >= 8 ? std::stoi(t[7]) : 150);
                    bool verbose = true;
                    for (size_t i = 6; i < t.size(); ++i)
                    {
                        if (t[i] == "--quiet")
                        {
                            verbose = false;
                            break;
                        }
                    }

                    pc.start(prod, cons, cap, os::sync::ms(prod_ms), os::sync::ms(cons_ms), verbose);
                    print_ok("productor–consumidor iniciado");
                    std::cout << "os> ";
                    continue;
                }
                if (t.size() >= 3 && t[2] == "stat")
                {
                    auto s = pc.stats();
                    std::cout << "produced=" << s.produced
                              << "  consumed=" << s.consumed
                              << "  max_buffer=" << s.max_buffer
                              << "  last_item=" << s.last_item << "\n";
                    std::cout << "running=" << (pc.running() ? "yes" : "no") << "\n";
                    std::cout << "os> ";
                    continue;
                }
                if (t.size() >= 3 && t[2] == "stop")
                {
                    pc.stop();
                    print_ok("productor–consumidor detenido");
                    std::cout << "os> ";
                    continue;
                }
                print_err("uso: sync pc <start|stat|stop>");
                std::cout << "os> ";
                continue;
            }

            // ==== Sincronización: filósofos comensales ====
            if (t[0] == "sync" && t.size() >= 2 && t[1] == "ph")
            {
                if (t.size() >= 3 && t[2] == "start")
                {
                    if (t.size() < 4)
                    {
                        print_err("uso: sync ph start <n_filosofos> [think_ms] [eat_ms] [--quiet]");
                        std::cout << "os> ";
                        continue;
                    }
                    std::size_t n = std::stoul(t[3]);
                    int think_ms = (t.size() >= 5 ? std::stoi(t[4]) : 500);
                    int eat_ms = (t.size() >= 6 ? std::stoi(t[5]) : 1000);
                    bool verbose = true;
                    for (size_t i = 4; i < t.size(); ++i)
                        if (t[i] == "--quiet")
                        {
                            verbose = false;
                            break;
                        }

                    ph.start(n, os::sync::ms(think_ms), os::sync::ms(eat_ms), verbose);
                    print_ok("filósofos iniciados");
                    std::cout << "os> ";
                    continue;
                }
                if (t.size() >= 3 && t[2] == "stat")
                {
                    std::cout << "running=" << (ph.running() ? "yes" : "no")
                              << "  n=" << ph.size() << "\n";
                    std::cout << "os> ";
                    continue;
                }
                if (t.size() >= 3 && t[2] == "stop")
                {
                    ph.stop();
                    print_ok("filósofos detenidos");
                    std::cout << "os> ";
                    continue;
                }
                print_err("uso: sync ph <start|stat|stop>");
                std::cout << "os> ";
                continue;
            }

            // desconocido
            print_err("comando desconocido. Usa 'help'.");
            cout << "os> ";
        }
        catch (const exception &e)
        {
            print_err(string("error: ") + e.what());
            cout << "os> ";
        }
    }

    cout << "Saliendo del programa...\n";
    return 0;
}
