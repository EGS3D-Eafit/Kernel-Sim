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
#include "../modules/mem/heap.hpp"

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
static vector<int> parse_ints(std::span<string> &v)
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
    cout << "Marcos=" << s.frames
         << "  Pasos=" << s.steps
         << "  Aciertos=" << s.hits
         << "  Fallos=" << s.faults
         << "  TasaAciertos=" << fixed << setprecision(2)
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
        cout << "(marcos por paso)\n";
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
    cout << gray << line << reset << "  (" << blue << "cabezal@" << last << reset << ")\n";

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
# ===================== AYUDA (ES) =====================

# Procesos (Administrador de procesos e interrupciones)
  proc crear     <nombre> <rafaga> [llegada=0]
  proc suspender <pid>
  proc reanudar  <pid>
  proc bloquear  <pid> <ticks>
  proc terminar  <pid>
  proc run       <ticks>
  proc tabla
  proc colas

# Planificador (SJF / Round Robin)
  sched sjf  <id,llegada,rafaga> ...
    # ej: sched sjf  1,0,8  2,1,4  3,2,9  4,3,5

  sched rr   <quantum> <id,llegada,rafaga> ...
    # ej: sched rr  2  1,0,5  2,1,3  3,2,8  4,3,6

# Memoria (FIFO / LRU / PFF)
  mem fifo <marcos> <secuencia_de_paginas ...>
  mem lru  <marcos> <secuencia_de_paginas ...>
  mem pff  <marcos_iniciales> <marcos_maximos> <ventana> <umbral> <secuencia_de_paginas ...>
  mem csv  <politica:fifo|lru|pff> <nombre_salida> <args... como arriba>

# Asignación de marcos (por proceso; reemplazo local)
  alloc igual <marcos_totales> <pid:[refs...]> ...
  alloc prop  <marcos_totales> <pid:[refs...]> ...

# Disco (FCFS / SSTF)
  disk fcfs <cabezal_inicial> <id,llegada,servicio,cilindro> ...
  disk sstf <cabezal_inicial> <id,llegada,servicio,cilindro> ...
  disk csv  <fcfs|sstf> <salida.csv> <cabezal_inicial> <...igual arriba>

# Sincronización (Productor–Consumidor)
  sync pc start <productores> <consumidores> <capacidad> [prod_ms] [cons_ms] [--silencio|--quiet]
  sync pc stat
  sync pc stop

# Sincronización (Filósofos comensales)
  sync ph start <n_filosofos> [pensar_ms] [comer_ms] [--silencio]
  sync ph stat
  sync ph stop

# Heap (Asignador Buddy)
  heap buddy init  <tam_total_bytes> <bloque_min_bytes>
  heap buddy alloc <nbytes>
  heap buddy free  <offset>
  heap buddy stat
  heap buddy view
  heap buddy csv   <prefijo_salida>  # genera <prefijo>_events.csv y <prefijo>_freelist.csv


# Otros
  printer demo [N]  
  
  help
  exit | quit

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

    os::mem::BuddyAllocator *buddy = nullptr; // puntero para permitir re-init

    cout << ansi::bold << "Consola de Kernel-Sim" << ansi::reset
         << "\nEscribe 'help' para ver ayuda.\n";

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

                if (sub == "crear")
                {
                    if (t.size() != 5)
                    {
                        print_err("uso: proc crear <nombre> <rafaga> <llegada>");
                    }
                    else
                    {
                        string name = t[2];
                        int burst = stoi(t[3]);
                        int arr = stoi(t[4]);
                        int pid = pm.create_process(name, burst, arr);
                        print_ok("proceso creado con pid=" + to_string(pid));
                    }
                }
                else if (sub == "suspender")
                {
                    if (t.size() != 3)
                        print_err("uso: proc suspender <pid>");
                    else
                        print_ok(pm.suspend(stoi(t[2])) ? "proceso suspendido" : "no se pudo suspender el proceso");
                }
                else if (sub == "reanudar")
                {
                    if (t.size() != 3)
                        print_err("uso: proc reanudar <pid>");
                    else
                        print_ok(pm.resume(stoi(t[2])) ? "proceso reanudado" : "no se pudo reanudar el proceso");
                }
                else if (sub == "terminar")
                {
                    if (t.size() != 3)
                        print_err("uso: proc terminar <pid>");
                    else
                        print_ok(pm.terminate(stoi(t[2])) ? "proceso terminado" : "no se pudo terminar el proceso");
                }
                else if (sub == "bloquear")
                {
                    if (t.size() != 4)
                        print_err("uso: proc bloquear <pid> <ticks>");
                    else
                        print_ok(pm.block_for(stoi(t[2]), stoi(t[3])) ? "proceso bloqueado" : "no se pudo bloquear el proceso");
                }
                else if (sub == "run")
                {
                    if (t.size() != 3)
                        print_err("uso: proc run <ticks>");
                    else
                    {
                        pm.run(stoi(t[2]));
                        print_ok("tiempo_actual=" + to_string(pm.now()));
                    }
                }
                else if (sub == "tabla")
                {
                    pm.print_table();
                }
                else if (sub == "colas")
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
                            print_err("Entrada inválida: usa id,llegada,rafaga (ej: 1,0,8)");
                            v.clear();
                            break;
                        }

                        // Validación de dominio
                        if (id <= 0 || arrival < 0 || burst <= 0)
                        {
                            std::ostringstream msg;
                            msg << "Datos inválidos para proceso '" << s
                                << "'. Reglas: id>0, llegada>=0, rafaga>0.";
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
                        std::cout << "Índice de fairness (Jain): " << std::fixed << std::setprecision(4)
                                  << stats.jain_fairness << "\n";
                    }
                }

                else if (kind == "rr")
                {
                    if (t.size() < 4)
                    {
                        print_err("uso: sched rr <quantum> <id,llegada,rafaga>...");
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
                            print_err("Entrada inválida: usa id,llegada,rafaga (ej: 2,0,3)");
                            v.clear();
                            break;
                        }
                        if (id <= 0 || arrival < 0 || burst <= 0)
                        {
                            std::ostringstream msg;
                            msg << "Datos inválidos para proceso '" << s
                                << "'. Reglas: id>0, llegada>=0, rafaga>0.";
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
                        std::cout << "Índice de justicia (Jain): " << std::fixed << std::setprecision(4)
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
                        print_err("uso: mem pff <marcos_init> <marcos_max> <ventana> <umbral> <seq...>");
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
                        print_err("uso: mem csv <fifo|lru|pff> <prefijo_salida> <args...>");
                        cout << "os> ";
                        continue;
                    }
                    string pol = t[2], outp = t[3];
                    if (pol == "fifo" || pol == "lru")
                    {
                        if (t.size() < 6)
                        {
                            print_err("uso: mem csv fifo|lru <prefijo_salida> <marcos> <seq...>");
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
                        ok ? print_ok("CSV exportado correctamente") : print_err("no se pudo exportar CSV");
                    }
                    else if (pol == "pff")
                    {
                        if (t.size() < 8)
                        {
                            print_err("uso: mem csv pff <prefijo_salida> <marcos_init> <marcos_max> <ventana> <umbral> <seq...>");
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
                        ok ? print_ok("CSV exportado correctamente") : print_err("no se pudo exportar CSV");
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
                    print_err("uso: alloc <equal|prop> <marcos_totales> <pid:[refs...]>...");
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
                        print_err("formato esperado: pid:[refs]");
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

            if (t[0] == "heap")
            {
                if (t.size() < 2)
                {
                    print_err("uso: heap buddy <...>");
                    cout << "os> ";
                    continue;
                }
                if (t[1] != "buddy")
                {
                    print_err("solo se soporta 'buddy'");
                    cout << "os> ";
                    continue;
                }

                if (t.size() >= 3 && t[2] == "init")
                {
                    if (t.size() != 5)
                    {
                        print_err("uso: heap buddy init <total> <min>");
                        cout << "os> ";
                        continue;
                    }
                    size_t total = stoull(t[3]), minb = stoull(t[4]);
                    delete buddy;
                    buddy = nullptr;
                    try
                    {
                        buddy = new os::mem::BuddyAllocator(total, minb);
                        print_ok("Asignador Buddy inicializado");
                    }
                    catch (const std::exception &e)
                    {
                        print_err(string("init falló: ") + e.what());
                    }
                    cout << "os> ";
                    continue;
                }

                if (!buddy)
                {
                    print_err("primero inicializa: heap buddy init <total> <min>");
                    cout << "os> ";
                    continue;
                }

                if (t.size() >= 3 && t[2] == "alloc")
                {
                    if (t.size() != 4)
                    {
                        print_err("uso: heap buddy alloc <nbytes>");
                        cout << "os> ";
                        continue;
                    }
                    int off = buddy->allocate(stoi(t[3]));
                    if (off < 0)
                        print_err("alloc falló");
                    else
                        print_ok("asignado en offset=" + to_string(off));
                    cout << "os> ";
                    continue;
                }

                if (t.size() >= 3 && t[2] == "free")
                {
                    if (t.size() != 4)
                    {
                        print_err("uso: heap buddy free <offset>");
                        cout << "os> ";
                        continue;
                    }
                    buddy->free_block(stoi(t[3]));
                    print_ok("bloque liberado");
                    cout << "os> ";
                    continue;
                }

                if (t.size() >= 3 && t[2] == "stat")
                {
                    auto s = buddy->stats();
                    cout << "total=" << s.total << " min=" << s.min_block
                         << " usado=" << s.used_bytes << " libre=" << s.free_bytes
                         << " mayor_libre=" << s.largest_free_block
                         << " frag_interna=" << s.internal_frag_bytes
                         << " allocs=" << s.alloc_count << " frees=" << s.free_count
                         << " fallas=" << s.fail_count << " splits=" << s.splits << " merges=" << s.merges << "\n";
                    cout << "os> ";
                    continue;
                }

                if (t.size() >= 3 && t[2] == "view")
                {
                    buddy->print_state(std::cout);
                    cout << "os> ";
                    continue;
                }

                if (t.size() >= 3 && t[2] == "csv")
                {
                    if (t.size() != 4)
                    {
                        print_err("uso: heap buddy csv <prefijo_salida>");
                        cout << "os> ";
                        continue;
                    }
                    string pfx = t[3];
                    bool ok = buddy->export_csv(pfx + "_events.csv", pfx + "_freelist.csv");
                    ok ? print_ok("CSV exportado correctamente") : print_err("no se pudo exportar CSV");
                    cout << "os> ";
                    continue;
                }

                print_err("subcomando de heap buddy desconocido");
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
                        print_err("uso: disk <fcfs|sstf> <cabezal_inicial> <id,llegada,servicio,cilindro>...");
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
                        print_err("uso: disk csv <fcfs|sstf> <salida.csv> <cabezal_inicial> <reqs...>");
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
                    ok ? print_ok("CSV exportado correctamente") : print_err("no se pudo exportar CSV");
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
                    print_err("uso: sync pc <pc|ph> <start|stat|stop> ...");
                    std::cout << "os> ";
                    continue;
                }
                if (t[1] == "pc")
                {
                    // subcomandos: start / stat / stop
                    if (t.size() >= 3 && t[2] == "start")
                    {
                        if (t.size() < 6)
                        {
                            print_err("uso: sync <pc|ph> start <prod> <cons> <cap> [prod_ms] [cons_ms] [--silencio]");
                            std::cout << "os> ";
                            continue;
                        }
                        std::size_t prod = std::stoul(t[3]);
                        std::size_t cons = std::stoul(t[4]);
                        std::size_t cap = std::stoul(t[5]);

                        int prod_ms = (t.size() >= 7 ? std::stoi(t[6]) : 100);
                        int cons_ms = (t.size() >= 8 ? std::stoi(t[7]) : 150);
                        bool verbose = true;
                        for (size_t i = 6; i < t.size(); ++i)
                        {
                            if (t[i] == "--silencio")
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
                        std::cout << "producido=" << s.produced
                                  << "  consumido=" << s.consumed
                                  << "  max_buffer=" << s.max_buffer
                                  << "  ultimo_item=" << s.last_item << "\n";
                        std::cout << "ejecutando=" << (pc.running() ? "sí" : "no") << "\n";
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

                if (t[1] == "ph")
                {
                    if (t.size() >= 3 && t[2] == "start")
                    {
                        if (t.size() < 4)
                        {
                            print_err("uso: sync ph start <n_filosofos> [pensar_ms] [comer_ms] [--silencio]");
                            std::cout << "os> ";
                            continue;
                        }
                        std::size_t n = std::stoul(t[3]);
                        int think_ms = (t.size() >= 5 ? std::stoi(t[4]) : 500);
                        int eat_ms = (t.size() >= 6 ? std::stoi(t[5]) : 1000);
                        bool verbose = true;
                        for (size_t i = 4; i < t.size(); ++i)
                            if (t[i] == "--silencio")
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
                        std::cout << "ejecutando=" << (ph.running() ? "sí" : "no")
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

                // subcomando desconocido
                print_err("subcomando sync desconocido: usa 'pc' o 'ph'");
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
