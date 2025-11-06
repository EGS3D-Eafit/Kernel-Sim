#include <iostream>
#include <bits/stdc++.h>
#include "../modules/disk/disk.h"
#include "../modules/cpu/pcb.h"
#include "../modules/cpu/cpu.h"
#include "../modules/mem/paging.h"
#include "../modules/disk_sched/disk_sched.h"
#include "../modules/io/device.h"
#include "../modules/sync/philosophers.h"
#include "../modules/sync/pc_cli.h"
#include "../modules/heap/buddy.h"

using namespace std;

vector<string> spltstring(const string &str, const string &delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0, end;

    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(str.substr(start));
    return tokens;
}
static bool is_number(const std::string &s) { return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit); }

int main()
{
    Disk disk("C");
    CPU cpu(5);
    Pager pager;
    DiskScheduler ds;
    Buddy heap;
    Printer printer;
    Philosophers philos;

    string input;
    while (true)
    {
        cout << disk.get_current_directory_path() << " > ";
        getline(cin, input);

        if (input == "exit")
        {
            cout << "Saliendo del sistema...\n";
            break;
        }

        if (input == "help")
        {
            cout << "Comandos disponibles:\n";
            cout << "  # FS/CPU ---------------------------------------------\n";
            cout << "  new nombre.ext          - Crear archivo/programa\n";
            cout << "  new nombre              - Crear directorio\n";
            cout << "  ls                      - Listar contenido\n";
            cout << "  edit nombre.ext         - Editar archivo/programa\n";
            cout << "  run nombre              - Ejecutar proceso (desde disco)\n";
            cout << "  run n                   - Ejecuta n ticks de CPU (simulación)\n";
            cout << "  kill nombre.ext         - Eliminar archivo\n";
            cout << "  kill nombre             - Eliminar directorio\n";
            cout << "  cd nombre | cd .. | /   - Navegar directorios\n";
            cout << "  pwd                     - Mostrar ruta actual\n";
            cout << "  ps                      - Listar procesos y estados (CPU)\n";
            cout << "  tick                    - Avanza 1 tick de CPU\n";
            cout << "  sched rr|sjf            - Política del planificador\n";
            cout << "  quantum N               - Fijar quantum de RR\n";
            cout << "  suspend PID             - Suspende/bloquea un proceso\n";
            cout << "  resume PID              - Reanuda/desbloquea un proceso\n";

            cout << "  # MEMORIA --------------------------------------------\n";
            cout << "  mem frames N              - Fija cantidad de marcos\n";
            cout << "  mem policy fifo|lru|ws    - Cambia algoritmo de reemplazo\n";
            cout << "  mem ws tau N              - Ventana Working Set (tau)\n";
            cout << "  mem access pid page       - Simula acceso a página\n";
            cout << "  mem show                  - Muestra estado de memoria\n";
            cout << "  mem stats                 - Muestra métricas de memoria\n";
            cout << "  mem dump archivo.csv      - Exporta accesos a CSV\n";
            cout << "  mem reset                 - Limpia tablas y métricas\n";

            cout << "  # DISK -----------------------------------------------\n";
            cout << "  disk init C H               - Inicializa (cilindros, cabezal)\n";
            cout << "  disk policy fcfs|sstf|scan  - Selecciona algoritmo\n";
            cout << "  disk req CYL                - Encola petición al cilindro CYL\n";
            cout << "  disk run                    - Atiende toda la cola\n";
            cout << "  disk show                   - Estado del planificador de disco\n";
            cout << "  disk stats                  - Métricas y movimiento total\n";
            cout << "  disk dump archivo.csv       - Exporta recorrido a CSV\n";
            cout << "  heap init NBYTES            - Inicializa heap buddy con N bytes (potencia de 2)\n";
            cout << "  heap alloc N                - Reserva N bytes y devuelve id\n";
            cout << "  heap free ID                - Libera bloque por id\n";
            cout << "  heap stats                  - Muestra estado del heap\n";

            cout << "  # IO/PRINTER -----------------------------------------\n";
            cout << "  io init                       - Reinicia la impresora\n";
            cout << "  io submit PID PAGES [prio0|1] - Encola trabajo\n";
            cout << "  io tick                       - Avanza 1 tick de impresión\n";
            cout << "  io run N                      - Ejecuta N ticks\n";
            cout << "  io show                       - Muestra estado de la impresora\n";
            cout << "  io stats                      - Métricas de impresión\n";

            cout << "  # PHILOSOPHERS --------------------------------------\n";
            cout << "  phil init N            - Inicializa N filósofos\n";
            cout << "  phil run N             - Simula N ticks\n";
            cout << "  phil show              - Estados actuales\n";
            cout << "  phil stats             - Veces que comió cada filósofo\n";

            cout << "  # PRODUCER/CONSUMER -------------------------------\n";
            cout << "  pc init N                - Inicializa buffer con capacidad N (def 5)\n";
            cout << "  pc produce [X]           - Produce un valor (auto si no se da X)\n";
            cout << "  pc consume               - Consume un valor si hay\n";
            cout << "  pc stat                  - Estado del buffer (size/cap/contadores)\n";
            cout << "  produce|consume|stat     - Atajos equivalentes\n";

            cout << "  exit                   - Salir\n";
            continue;
        }

        // ===== FS / navegación =====
        if (input == "pwd")
        {
            cout << "Ruta actual: " << disk.get_current_directory_path() << "\n";
            continue;
        }
        if (input.rfind("cd ", 0) == 0)
        {
            string param = input.substr(3);
            cout << disk.cd_command(param) << "\n";
            continue;
        }
        if (input.rfind("kill ", 0) == 0)
        {
            auto split = spltstring(input.substr(5), ".");
            if (split.size() == 2)
            {
                cout << disk.get_current_directory()->command("kill " + split[0], split[1]) << "\n";
            }
            else if (split.size() == 1)
            {
                cout << disk.get_current_directory()->command("kill " + split[0], {}) << "\n";
            }
            else
                cout << "Formato inválido.\n";
            continue;
        }
        if (input.rfind("new ", 0) == 0)
        {
            auto split = spltstring(input.substr(4), ".");
            if (split.size() == 2)
            {
                cout << disk.get_current_directory()->command("new " + split[0], split[1]) << "\n";
            }
            else if (split.size() == 1)
            {
                cout << disk.get_current_directory()->command("new " + split[0], {}) << "\n";
            }
            else
                cout << "Formato inválido.\n";
            continue;
        }
        if (input == "ls")
        {
            cout << disk.get_current_directory()->command("ls", {}) << "\n";
            continue;
        }
        if (input.rfind("edit ", 0) == 0)
        {
            string nombre = input.substr(5);
            auto *current = disk.get_current_directory();
            auto &vec = get<vector<Information *>>(current->get_content());
            bool found = false;
            for (auto *info : vec)
            {
                File *file = dynamic_cast<File *>(info);
                if (!file)
                    continue;
                auto sp = spltstring(nombre, ".");
                if (sp.size() == 2 && file->get_name() == sp[0] && file->get_extension() == sp[1])
                {
                    found = true;
                    if (file->get_extension() == "txt")
                    {
                        cout << "Nuevo contenido:\n";
                        string content;
                        getline(cin, content);
                        file->change_content(content);
                        cout << "Contenido actualizado.\n";
                    }
                    else if (file->get_extension() == "ext")
                    {
                        cout << "Nueva rafaga:\n";
                        string content;
                        getline(cin, content);
                        file->change_content(content);
                        cout << "Contenido actualizado.\n";
                    }
                    else
                    {
                        cout << "No es un archivo editable.\n";
                    }
                    break;
                }
            }
            if (!found)
                cout << "Archivo no encontrado.\n";
            continue;
        }
        if (input.rfind("run ", 0) == 0)
        {
            string param = input.substr(4);
            if (is_number(param))
            {
                int ticks = stoi(param);
                while (ticks--)
                    cpu.execute();
                cout << "[cpu] run ok\n";
                continue;
            }
            else
            {
                auto *current = disk.get_current_directory();
                auto &vec = get<vector<Information *>>(current->get_content());
                bool found = false;
                for (auto *info : vec)
                {
                    File *file = dynamic_cast<File *>(info);
                    if (!file)
                        continue;
                    auto sp = spltstring(param, ".");
                    if (sp.size() == 1 && file->get_name() == sp[0] && file->get_extension() == "ext")
                    {
                        found = true;
                        cpu.load_process_from_string(file->get_name(), file->get_content());
                        cout << "Proceso cargado: " << file->get_name() << "\n";
                        break;
                    }
                }
                if (!found)
                    cout << "Programa no encontrado o extensión inválida.\n";
                continue;
            }
        }
        if (input == "ps")
        {
            cout << cpu.processes_str();
            continue;
        }
        if (input == "tick")
        {
            cpu.execute();
            cout << "[cpu] tick\n";
            continue;
        }
        if (input.rfind("sched ", 0) == 0)
        {
            string pol = input.substr(6);
            transform(pol.begin(), pol.end(), pol.begin(), ::tolower);
            if (pol == "rr")
            {
                cpu.set_scheduler(CPU::SchedulerPolicy::ROUND_ROBIN);
                cout << "[cpu] scheduler=RR\n";
            }
            else if (pol == "sjf")
            {
                cpu.set_scheduler(CPU::SchedulerPolicy::SJF);
                cout << "[cpu] scheduler=SJF\n";
            }
            else
                cout << "Uso: sched rr|sjf\n";
            continue;
        }
        if (input.rfind("quantum ", 0) == 0)
        {
            string nstr = input.substr(8);
            if (is_number(nstr))
            {
                cpu.set_quantum(stoi(nstr));
                cout << "[cpu] quantum=" << nstr << "\n";
            }
            else
                cout << "Uso: quantum N\n";
            continue;
        }

        // === NUEVO: Suspender/Reanudar procesos ===
        if (input.rfind("suspend ", 0) == 0)
        {
            string nstr = input.substr(8);
            if (is_number(nstr))
            {
                int pid = stoi(nstr);
                // Ajusta estos nombres si en tu CPU se llaman block/unblock, etc.
                bool ok = cpu.suspend(pid);
                cout << (ok ? "[cpu] suspend pid=" + to_string(pid) + "\n" : "[cpu] no se pudo suspender pid\n");
            }
            else
            {
                cout << "Uso: suspend PID\n";
            }
            continue;
        }
        if (input.rfind("resume ", 0) == 0)
        {
            string nstr = input.substr(7);
            if (is_number(nstr))
            {
                int pid = stoi(nstr);
                bool ok = cpu.resume(pid);
                cout << (ok ? "[cpu] resume pid=" + to_string(pid) + "\n" : "[cpu] no se pudo reanudar pid\n");
            }
            else
            {
                cout << "Uso: resume PID\n";
            }
            continue;
        }

        // ===== MEMORIA =====
        if (input.rfind("mem ", 0) == 0)
        {
            if (input.rfind("mem frames ", 0) == 0)
            {
                string nstr = input.substr(11);
                if (is_number(nstr))
                {
                    int n = stoi(nstr);
                    pager.setFrames(n);
                    cout << "[mem] frames=" << n << "\n";
                }
                else
                    cout << "Uso: mem frames N\n";
                continue;
            }
            if (input.rfind("mem policy ", 0) == 0)
            {
                string p = input.substr(11);
                transform(p.begin(), p.end(), p.begin(), ::tolower);
                if (p == "fifo")
                {
                    pager.setPolicy(Pager::Policy::FIFO);
                    cout << "[mem] policy=FIFO\n";
                }
                else if (p == "lru")
                {
                    pager.setPolicy(Pager::Policy::LRU);
                    cout << "[mem] policy=LRU\n";
                }
                else if (p == "ws")
                {
                    pager.setPolicy(Pager::Policy::WS);
                    cout << "[mem] policy=WS\n";
                }
                else
                    cout << "Uso: mem policy fifo|lru|ws\n";
                continue;
            }
            if (input.rfind("mem ws tau ", 0) == 0)
            {
                string nstr = input.substr(11);
                if (is_number(nstr))
                {
                    pager.setWSParameter(stoi(nstr));
                    cout << "[mem] ws tau=" << nstr << "\n";
                }
                else
                    cout << "Uso: mem ws tau N\n";
                continue;
            }
            if (input.rfind("mem access ", 0) == 0)
            {
                auto parts = spltstring(input.substr(11), " ");
                if (parts.size() == 2 && is_number(parts[0]) && is_number(parts[1]))
                {
                    int pid = stoi(parts[0]);
                    int page = stoi(parts[1]);
                    cout << pager.access(pid, page) << "\n";
                }
                else
                    cout << "Uso: mem access pid page\n";
                continue;
            }
            if (input == "mem show")
            {
                cout << pager.show();
                continue;
            }
            if (input == "mem stats")
            {
                cout << pager.stats();
                continue;
            }
            if (input.rfind("mem dump ", 0) == 0)
            {
                string p = input.substr(9);
                cout << (pager.dumpCSV(p) ? "[mem] CSV -> " + p + "\n" : "Error CSV\n");
                continue;
            }
            if (input == "mem reset")
            {
                pager.reset();
                cout << "[mem] reset\n";
                continue;
            }
            cout << "Comando mem desconocido.\n";
            continue;
        }

        // ===== DISK SCHED =====
        if (input.rfind("disk ", 0) == 0)
        {
            if (input.rfind("disk init ", 0) == 0)
            {
                auto parts = spltstring(input.substr(10), " ");
                if (parts.size() == 2 && is_number(parts[0]) && is_number(parts[1]))
                {
                    ds.init(stoi(parts[0]), stoi(parts[1]));
                    cout << "[disk] init ok\n";
                }
                else
                    cout << "Uso: disk init C H\n";
                continue;
            }
            if (input.rfind("disk policy ", 0) == 0)
            {
                string p = input.substr(12);
                transform(p.begin(), p.end(), p.begin(), ::tolower);
                if (p == "fcfs")
                {
                    ds.setPolicy(DiskScheduler::Policy::FCFS);
                    cout << "[disk] policy=FCFS\n";
                }
                else if (p == "sstf")
                {
                    ds.setPolicy(DiskScheduler::Policy::SSTF);
                    cout << "[disk] policy=SSTF\n";
                }
                else if (p == "scan")
                {
                    ds.setPolicy(DiskScheduler::Policy::SCAN);
                    cout << "[disk] policy=SCAN\n";
                }
                else
                    cout << "Uso: disk policy fcfs|sstf|scan\n";
                continue;
            }
            if (input.rfind("disk req ", 0) == 0)
            {
                string nstr = input.substr(9);
                if (is_number(nstr))
                {
                    ds.request(stoi(nstr));
                    cout << "[disk] req " << nstr << "\n";
                }
                else
                    cout << "Uso: disk req CYL\n";
                continue;
            }
            if (input == "disk run")
            {
                int move = ds.run();
                cout << "[disk] run -> movimiento total " << move << "\n";
                continue;
            }
            if (input == "disk show")
            {
                cout << ds.show();
                continue;
            }
            if (input == "disk stats")
            {
                cout << ds.stats();
                continue;
            }
            if (input.rfind("disk dump ", 0) == 0)
            {
                string p = input.substr(10);
                cout << (ds.dumpCSV(p) ? "[disk] CSV -> " + p + "\n" : "Error CSV\n");
                continue;
            }
            cout << "Comando disk desconocido.\n";
            continue;
        }

if (input.rfind("heap ", 0) == 0)
{
    auto tok = spltstring(input, " ");
    if (tok.size() >= 2 && tok[1] == "init")
    {
        if (tok.size() < 3) { cout << "Uso: heap init NBYTES\n"; continue; }
        size_t n = stoul(tok[2]);
        heap.init(n);
        cout << "Heap inicializado con " << n << " bytes.\n";
        continue;
    }
    if (tok.size() >= 2 && tok[1] == "alloc")
    {
        if (tok.size() < 3) { cout << "Uso: heap alloc N\n"; continue; }
        size_t n = stoul(tok[2]);
        int id = heap.alloc(n);
        if (id < 0) cout << "Fallo: sin espacio.\n";
        else cout << "OK id=" << id << "\n";
        continue;
    }
    if (tok.size() >= 2 && tok[1] == "free")
    {
        if (tok.size() < 3) { cout << "Uso: heap free ID\n"; continue; }
        int id = stoi(tok[2]);
        cout << (heap.free(id) ? "Liberado.\n" : "ID no válido.\n");
        continue;
    }
    if (tok.size() >= 2 && tok[1] == "stats")
    {
        cout << heap.stats();
        continue;
    }
    cout << "Comando heap desconocido.\n";
    continue;
}

        // ===== IO / PRINTER =====
        if (input.rfind("io ", 0) == 0)
        {
            if (input == "io init")
            {
                printer.reset();
                cout << "[io] init ok\n";
                continue;
            }
            if (input.rfind("io submit ", 0) == 0)
            {
                auto parts = spltstring(input.substr(10), " ");
                if (parts.size() >= 2 && is_number(parts[0]) && is_number(parts[1]))
                {
                    int pid = stoi(parts[0]);
                    int pages = stoi(parts[1]);
                    int prio = 0;
                    if (parts.size() == 3 && (parts[2] == "0" || parts[2] == "1"))
                        prio = stoi(parts[2]);
                    printer.submit(pid, pages, prio);
                    cout << "[io] submit pid=" << pid << " pages=" << pages << " prio=" << prio << "\n";
                }
                else
                    cout << "Uso: io submit PID PAGES [prio0|1]\n";
                continue;
            }
            if (input == "io tick")
            {
                printer.tick();
                cout << "[io] tick\n";
                continue;
            }
            if (input.rfind("io run ", 0) == 0)
            {
                string nstr = input.substr(7);
                if (is_number(nstr))
                {
                    int n = stoi(nstr);
                    while (n--)
                        printer.tick();
                    cout << "[io] run ok\n";
                }
                else
                    cout << "Uso: io run N\n";
                continue;
            }
            if (input == "io show")
            {
                cout << printer.show();
                continue;
            }
            if (input == "io stats")
            {
                cout << printer.stats();
                continue;
            }
            cout << "Comando io desconocido.\n";
            continue;
        }

        // ===== PHILOSOPHERS =====
        if (input.rfind("phil ", 0) == 0)
        {
            if (input.rfind("phil init ", 0) == 0)
            {
                string nstr = input.substr(10);
                if (is_number(nstr))
                {
                    philos.init(stoi(nstr));
                    cout << "[phil] init " << nstr << "\n";
                }
                else
                    cout << "Uso: phil init N\n";
                continue;
            }
            if (input.rfind("phil run ", 0) == 0)
            {
                string nstr = input.substr(9);
                if (is_number(nstr))
                {
                    int n = stoi(nstr);
                    while (n--)
                        philos.tick();
                    cout << "[phil] run ok\n";
                }
                else
                    cout << "Uso: phil run N\n";
                continue;
            }
            if (input == "phil show")
            {
                cout << philos.show();
                continue;
            }
            if (input == "phil stats")
            {
                cout << philos.stats();
                continue;
            }
            cout << "Comando phil desconocido.\n";
            continue;
        }

        // ===== PC / Producer-Consumer =====
        // Grouped commands: pc init N | pc produce [X] | pc consume | pc stat
        // Shortcuts: produce | consume | stat
        if (input.rfind("pc ", 0) == 0 || input == "produce" || input.rfind("produce ", 0) == 0 || input == "consume" || input == "stat" || input == "pc stat")
        {
            if (input == "pc init")
            {
                cout << "Uso: pc init N\n";
                continue;
            }
            if (input.rfind("pc init ", 0) == 0)
            {
                string nstr = input.substr(8);
                if (is_number(nstr))
                {
                    size_t cap = stoul(nstr);
                    pc_cli::init(cap);
                    cout << "[pc] init capacity=" << cap << "\n";
                }
                else
                {
                    cout << "Uso: pc init N\n";
                }
                continue;
            }
            if (input == "pc produce" || input == "produce")
            {
                cout << pc_cli::produce() << "\n";
                continue;
            }
            if (input.rfind("pc produce ", 0) == 0 || input.rfind("produce ", 0) == 0)
            {
                string val = input.rfind("pc produce ", 0) == 0 ? input.substr(11) : input.substr(8);
                if (is_number(val))
                {
                    cout << pc_cli::produce(stoi(val)) << "\n";
                }
                else
                {
                    cout << "Uso: pc produce [X]\n";
                }
                continue;
            }
            if (input == "pc consume" || input == "consume")
            {
                cout << pc_cli::consume() << "\n";
                continue;
            }
            if (input == "pc stat" || input == "stat")
            {
                cout << pc_cli::stat() << "\n";
                continue;
            }
            cout << "Comando pc desconocido.\n";
            continue;
        }

        cout << "Comando no reconocido. Escribe 'help' para ver opciones.\n";
    }
    return 0;
}
