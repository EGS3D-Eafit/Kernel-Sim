#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

#include "../modules/disk/disk.h"
#include "../modules/cpu/pcb.h"
#include "../modules/cpu/cpu.h"

// demos de memoria y disco (lectura de trazas .txt con cabeceras)
#include "../modules/mem/mem.h"
#include "../modules/disk/disk.h"

// productor-consumidor CLI
#include "../modules/sync/pc_cli.h"

using namespace std;

// ------------------------------ util ------------------------------
vector<string> spltstring(const string &str, const string &delimiter)
{
    vector<string> tokens;
    size_t start = 0;
    size_t end;

    while ((end = str.find(delimiter, start)) != string::npos)
    {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(str.substr(start));
    return tokens;
}

static string trim(string s)
{
    while (!s.empty() && isspace((unsigned char)s.front()))
        s.erase(s.begin());
    while (!s.empty() && isspace((unsigned char)s.back()))
        s.pop_back();
    return s;
}

// ------------------------------ llegadas desde proc.run ------------------------------
struct ProcArrival
{
    string name;
    int at;
    int burst;
};
static vector<ProcArrival> g_proc_arrivals; // cargado por proc.run
static long long g_time = 0;                // tiempo simulado global

// ------------------------------ impresora (E/S) simple ------------------------------
struct PrintJob
{
    int pid;
    int pages;
    int prio;
};
static deque<PrintJob> PRINTER_Q;

// ------------------------------ main ------------------------------
int main()
{
    Disk disk("C");
    CPU cpu(5); // quantum por defecto; puedes cambiarlo editando CPU si quieres

    string input;
    while (true)
    {
        cout << disk.get_current_directory_path() << " > ";
        if (!getline(cin, input))
            break;
        if (input.empty())
            continue;

        if (input == "exit")
        {
            cout << "Saliendo del sistema...\n";
            break;
        }

        if (input == "help")
        {
            cout << "Comandos disponibles:\n";

            // --- Sistema de archivos / programas ---
            cout << "  new nombre.ext      - Crear archivo/programa [exe, txt]\n";
            cout << "  new nombre          - Crear directorio\n";
            cout << "  ls                  - Listar contenido\n";
            cout << "  edit nombre.ext     - Editar archivo/programa\n";
            cout << "  run nombre          - Ejecutar proceso (si existe programa .exe con ese nombre)\n";
            cout << "  kill nombre.ext     - Eliminar archivo\n";
            cout << "  kill nombre         - Eliminar directorio\n";
            cout << "  cd nombre           - Cambiar a subdirectorio\n";
            cout << "  cd ..               - Subir un nivel\n";
            cout << "  cd /                - Ir a raiz\n";
            cout << "  pwd                 - Mostrar ruta actual\n";

            // --- CPU / Planificador ---
            cout << "\nCPU / Planificador:\n";
            cout << "  proc.run <archivo>  - Cargar llegadas CSV: name,arrival,cpu_burst\n";
            cout << "  tick n              - Avanzar n ticks (inyecta llegadas por tiempo)\n";
            cout << "  ps                  - Mostrar procesos, estados y cola READY\n";
            cout << "  sched rr|sjf        - Seleccionar algoritmo (Round Robin o SJF no apropiativo)\n";
            cout << "  suspend <pid>       - Suspender proceso (SUSPENDED)\n";
            cout << "  resume <pid>        - Reanudar proceso (READY)\n";
            cout << "  terminate <pid>     - Terminar proceso (TERMINATED)\n";
            cout << "  irq timer           - Simular interrupcion de temporizador (1 tick)\n";
            cout << "  irq io <pid>        - Desbloquear proceso <pid> por E/S (a READY)\n";

            // --- Memoria ---
            cout << "\nMemoria:\n";
            cout << "  mem.run <ruta>      - Ejecutar traza (headers + lineas 'pid page')\n";
            cout << "  mem.stat            - Ver estadisticas (hits/faults) y resumen\n";

            // --- Disco ---
            cout << "\nDisco:\n";
            cout << "  disk.run <ruta>     - Ejecutar traza (headers + lista de cilindros)\n";
            cout << "  disk.stat           - Ver recorrido total y estado del cabezal\n";

            // --- Sincronizacion (Productor-Consumidor) ---
            cout << "\nSincronizacion:\n";
            cout << "  pc init <cap>       - Inicializar buffer acotado de capacidad <cap>\n";
            cout << "  pc produce [v]      - Producir (opcionalmente valor v)\n";
            cout << "  pc consume          - Consumir\n";
            cout << "  pc stat             - Estado del buffer\n";

            // --- Dispositivo de E/S simulado: Impresora ---
            cout << "\nImpresora (E/S):\n";
            cout << "  printer submit <pid> <pages> [prio] - Encolar trabajo y bloquear <pid>\n";
            cout << "  printer tick n       - Procesar n paginas de la cola\n";
            cout << "  printer stat         - Ver cola de impresion\n";

            // --- Sistema ---
            cout << "\nSistema:\n";
            cout << "  help                - Mostrar esta ayuda\n";
            cout << "  exit                - Salir\n";
            continue;
        }

        if (input == "pwd")
        {
            cout << "Ruta actual: " << disk.get_current_directory_path() << "\n";
            continue;
        }

        if (input.rfind("cd ", 0) == 0)
        {
            string path = input.substr(3);
            cout << disk.go_to_path(path) << "\n";
            continue;
        }

        if (input == "ps")
        {
            cpu.listarProcesos();
            continue;
        }

        // ==========================
        // CPU / PLANIFICADOR EXTRA
        // ==========================

        if (input.rfind("sched ", 0) == 0)
        {
            auto args = spltstring(input, " ");
            if (args.size() >= 2)
            {
                string which = args[1];
                if (which == "rr")
                {
                    cpu.set_algo_rr();
                    cout << "Algoritmo: RR\n";
                }
                else if (which == "sjf")
                {
                    cpu.set_algo_sjf();
                    cout << "Algoritmo: SJF (no apropiativo)\n";
                }
                else
                {
                    cout << "Uso: sched rr|sjf\n";
                }
            }
            else
                cout << "Uso: sched rr|sjf\n";
            continue;
        }

        if (input.rfind("suspend ", 0) == 0)
        {
            int pid = stoi(input.substr(8));
            cout << (cpu.suspend_pid(pid) ? "OK\n" : "PID no valido\n");
            continue;
        }

        if (input.rfind("resume ", 0) == 0)
        {
            int pid = stoi(input.substr(7));
            cout << (cpu.resume_pid(pid) ? "OK\n" : "PID no valido o no suspendido\n");
            continue;
        }

        if (input.rfind("terminate ", 0) == 0)
        {
            int pid = stoi(input.substr(10));
            cout << (cpu.terminate_pid(pid) ? "OK\n" : "PID no valido\n");
            continue;
        }

        if (input == "irq timer")
        {
            cpu.irq_timer();
            g_time++; // avanza el tiempo simulado también
            continue;
        }

        if (input.rfind("irq io ", 0) == 0)
        {
            int pid = stoi(input.substr(7));
            cout << (cpu.irq_io_unblock(pid) ? "Desbloqueado\n" : "PID no valido o no bloqueado\n");
            continue;
        }

        // ==========================
        // CARGA DE TRAZAS
        // ==========================

        // proc.run lee CSV: name,arrival,cpu_burst
        if (input.rfind("proc.run ", 0) == 0)
        {
            string path = trim(input.substr(9));
            g_proc_arrivals.clear();
            ifstream f(path);
            if (!f.good())
            {
                cout << "No existe " << path << "\n";
                continue;
            }
            string line;
            while (getline(f, line))
            {
                if (line.empty() || line[0] == '#')
                    continue;
                istringstream iss(line);
                string name, arr_s, burst_s;
                if (getline(iss, name, ',') && getline(iss, arr_s, ',') && getline(iss, burst_s, ','))
                {
                    name = trim(name);
                    arr_s = trim(arr_s);
                    burst_s = trim(burst_s);
                    int at = max(0, stoi(arr_s));
                    int bt = max(1, stoi(burst_s));
                    g_proc_arrivals.push_back({name, at, bt});
                }
            }
            sort(g_proc_arrivals.begin(), g_proc_arrivals.end(),
                 [](const auto &a, const auto &b)
                 { return a.at < b.at; });
            cout << "Cargados " << g_proc_arrivals.size() << " procesos desde " << path << "\n";
            continue;
        }

        // memoria
        if (input.rfind("mem.run ", 0) == 0)
        {
            string path = trim(input.substr(8));
            memdemo::run_trace_file(path);
            continue;
        }
        if (input == "mem.stat")
        {
            memdemo::stat();
            continue;
        }

        // disco
        if (input.rfind("disk.run ", 0) == 0)
        {
            string path = trim(input.substr(9));
            diskdemo::run_trace_file(path);
            continue;
        }
        if (input == "disk.stat")
        {
            diskdemo::stat();
            continue;
        }

        // ==========================
        // FS / EXISTENTE
        // ==========================

        Directory *current = disk.get_current_directory();

        if (input.rfind("edit ", 0) == 0)
        {
            string arg = input.substr(5);
            auto split = spltstring(arg, ".");

            if (split.size() != 2)
            {
                cout << "Formato invalido. Usa: edit nombre.ext\n";
                continue;
            }

            Directory *current = disk.get_current_directory();
            auto cur_cont = current->get_content();
            auto vec = get_if<vector<Information *>>(&cur_cont);
            if (!vec)
            {
                cout << "El directorio actual no tiene contenido valido.\n";
                continue;
            }

            bool found = false;
            for (Information *info : *vec)
            {
                File *file = dynamic_cast<File *>(info);
                if (file && file->get_name() == split[0] && file->get_extension() == split[1])
                {
                    found = true;
                    if (file->get_extension() == "txt")
                    {
                        cout << "Ingresa el nuevo contenido del archivo:\n";
                        string nuevoContenido;
                        getline(cin, nuevoContenido);
                        file->edit_content(nuevoContenido);
                        cout << "Archivo editado correctamente.\n";
                    }
                    else if (file->get_extension() == "exe")
                    {
                        cout << "Ingresa el nuevo tiempo de ejecucion del proceso:\n";
                        int tiempo;
                        cin >> tiempo;
                        cin.ignore();
                        PCB *nuevoPCB = new PCB(file->get_name(), tiempo);
                        file->edit_content(nuevoPCB);
                        cout << "Proceso editado correctamente.\n";
                    }
                    else
                    {
                        cout << "Tipo de archivo no editable.\n";
                    }
                    break;
                }
            }

            if (!found)
            {
                cout << "Archivo no encontrado.\n";
            }

            continue;
        }

        if (input.rfind("new ", 0) == 0)
        {
            string arg = input.substr(4);
            cout << current->command("new " + arg, {}) << "\n";
            continue;
        }

        if (input == "ls")
        {
            cout << current->command("ls", {}) << "\n";
            continue;
        }

        if (input.rfind("run ", 0) == 0)
        {
            string arg = input.substr(4);
            cout << current->command("run " + arg, cpu) << "\n";
            continue;
        }

        if (input.rfind("kill ", 0) == 0)
        {
            string arg = input.substr(5);
            cout << current->command("kill " + arg, {}) << "\n";
            continue;
        }

        // ==========================
        // tick n (inyecta llegadas)
        // ==========================
        if (input.rfind("tick ", 0) == 0)
        {
            string arg = input.substr(5);
            int n = max(0, stoi(arg));

            for (int i = 0; i < n; i++)
            {
                // inyectar llegadas con arrival == g_time
                while (!g_proc_arrivals.empty() && g_proc_arrivals.front().at == g_time)
                {
                    auto pa = g_proc_arrivals.front();
                    g_proc_arrivals.erase(g_proc_arrivals.begin());
                    auto *p = new PCB(pa.name, pa.burst);
                    cpu.add_process(*p);
                    cout << "[t=" << g_time << "] llegada PID " << p->pid << " (" << pa.name << ", burst=" << pa.burst << ")\n";
                }

                // ejecutar 1 tick
                cpu.ejecutar(1);
                g_time++;
            }
            continue;
        }

        // ==========================
        // Productor-Consumidor
        // ==========================
        if (input.rfind("pc ", 0) == 0)
        {
            auto args = spltstring(input, " ");
            if (args.size() >= 2)
            {
                if (args[1] == "init")
                {
                    if (args.size() >= 3)
                    {
                        size_t cap = std::stoul(args[2]);
                        pc_cli::init(cap);
                        std::cout << "OK (cap=" << cap << ")\n";
                    }
                    else
                    {
                        pc_cli::init(); // por defecto 5
                        std::cout << "OK (cap=5)\n";
                    }
                }
                else if (args[1] == "produce")
                {
                    if (args.size() >= 3)
                    {
                        int v = std::stoi(args[2]);
                        std::cout << pc_cli::produce(v) << "\n";
                    }
                    else
                    {
                        std::cout << pc_cli::produce(std::nullopt) << "\n";
                    }
                }
                else if (args[1] == "consume")
                {
                    std::cout << pc_cli::consume() << "\n";
                }
                else if (args[1] == "stat")
                {
                    std::cout << pc_cli::stat() << "\n";
                }
                else
                {
                    std::cout << "Uso: pc init <cap> | pc produce [v] | pc consume | pc stat\n";
                }
            }
            else
            {
                std::cout << "Uso: pc init <cap> | pc produce [v] | pc consume | pc stat\n";
            }
            continue;
        }

        // ==========================
        // Impresora (E/S) simple
        // ==========================
        if (input.rfind("printer ", 0) == 0)
        {
            auto args = spltstring(input, " ");
            if (args.size() >= 2)
            {
                if (args[1] == "submit" && args.size() >= 4)
                {
                    int pid = stoi(args[2]);
                    int pages = stoi(args[3]);
                    int pr = (args.size() >= 5 ? stoi(args[4]) : 0);
                    PRINTER_Q.push_back({pid, pages, pr});
                    cpu.suspend_pid(pid); // lo bloqueamos/suspendemos durante IO
                    cout << "En cola impresora pid=" << pid << " pages=" << pages << "\n";
                }
                else if (args[1] == "tick" && args.size() >= 3)
                {
                    int n = stoi(args[2]);
                    while (n-- > 0 && !PRINTER_Q.empty())
                    {
                        auto &front = PRINTER_Q.front();
                        front.pages -= 1;
                        if (front.pages <= 0)
                        {
                            int pid = front.pid;
                            PRINTER_Q.pop_front();
                            cpu.resume_pid(pid); // listo IO -> READY
                            cout << "Impreso pid=" << pid << " -> READY\n";
                        }
                    }
                }
                else if (args[1] == "stat")
                {
                    cout << "PRINTER_Q:";
                    for (auto &j : PRINTER_Q)
                        cout << " [pid=" << j.pid << ",pages=" << j.pages << "]";
                    cout << "\n";
                }
                else
                {
                    cout << "Uso: printer submit <pid> <pages> [prio] | printer tick n | printer stat\n";
                }
            }
            else
            {
                cout << "Uso: printer submit <pid> <pages> [prio] | printer tick n | printer stat\n";
            }
            continue;
        }

        cout << "Comando no reconocido. Escribe 'help' para ver opciones.\n";
    }

    return 0;
}
