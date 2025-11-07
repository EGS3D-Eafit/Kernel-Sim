#include "mem.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <deque>
#include <unordered_set>
#include <set>
#include <cctype>
#include <algorithm>

using namespace std;

Memoria::Memoria(int tamaño) : bloques(tamaño, 0) {}

int Memoria::asignar(int cantidad)
{
    for (size_t i = 0; i < bloques.size(); ++i)
    {
        if (bloques[i] == 0)
        {
            bloques[i] = 1;
            return i;
        }
    }
    return -1;
}

void Memoria::liberar(int inicio, int cantidad)
{
    for (int i = inicio; i < inicio + cantidad && i < (int)bloques.size(); ++i)
        bloques[i] = 0;
}

void Memoria::mostrar()
{
    for (auto b : bloques)
        cout << (b ? "#" : "_");
    cout << "\n";
}

namespace
{
    // Parámetros configurables desde cabeceras del archivo, si no hay se usan estos valores por defecto
    int g_frames = 24;
    int g_ps = 4;
    int g_ws = 5;
    string g_repl = "FIFO";

    // Estructuras para las páginas cargadas
    using Page = pair<int, int>;
    struct PageHash
    {
        size_t operator()(const Page &p) const noexcept
        {
            return (static_cast<size_t>(p.first) << 32) ^ static_cast<size_t>(p.second);
        }
    };

    deque<Page> fifo;
    unordered_set<Page, PageHash> loaded;
    deque<Page> ws_window_buf;

    long long hits = 0;
    long long faults = 0;

    static inline string trim(string s)
    {
        while (!s.empty() && isspace((unsigned char)s.front()))
            s.erase(s.begin());
        while (!s.empty() && isspace((unsigned char)s.back()))
            s.pop_back();
        return s;
    }
}

void memdemo::run_trace_file(const string &path)
{
    // Reiniciar todo antes de una nueva corrida
    fifo.clear();
    loaded.clear();
    ws_window_buf.clear();
    hits = faults = 0;

    ifstream f(path);
    if (!f.good())
    {
        cout << "No existe " << path << "\n";
        return;
    }

    // Defaults
    g_frames = 24;
    g_ps = 4;
    g_ws = 5;
    g_repl = "FIFO";

    string line;
    while (getline(f, line))
    {
        if (line.empty())
            continue;

        // Procesar cabeceras tipo "# frames=24"
        if (line[0] == '#')
        {
            auto pos = line.find('=');
            if (pos != string::npos)
            {
                string k = trim(line.substr(1, pos - 1));
                string v = trim(line.substr(pos + 1));
                if (k == "frames")
                    g_frames = max(1, stoi(v));
                else if (k == "page_size")
                    g_ps = max(1, stoi(v));
                else if (k == "repl")
                {
                    transform(v.begin(), v.end(), v.begin(), ::toupper);
                    g_repl = v;
                }
                else if (k == "ws_window")
                    g_ws = max(0, stoi(v));
            }
            continue;
        }

        // Leer línea de traza: pid page
        istringstream iss(line);
        int pid, pg;
        if (!(iss >> pid >> pg))
            continue;

        Page key{pid, pg};

        // Guardar acceso en ventana de working set (solo estadístico)
        if (g_ws > 0)
        {
            ws_window_buf.push_back(key);
            if ((int)ws_window_buf.size() > g_ws)
                ws_window_buf.pop_front();
        }

        // Chequear hit/fault
        if (loaded.find(key) != loaded.end())
        {
            ++hits;
            continue;
        }

        // Fallo de página
        ++faults;

        // Si memoria llena -> reemplazo FIFO
        if ((int)loaded.size() >= g_frames)
        {
            if (!fifo.empty())
            {
                Page victim = fifo.front();
                fifo.pop_front();
                loaded.erase(victim);
            }
        }

        // Cargar nueva página
        loaded.insert(key);
        fifo.push_back(key);
    }
}

void memdemo::stat()
{
    cout << "MEM DEMO  | frames=" << g_frames
         << " | page_size=" << g_ps
         << " | repl=" << g_repl
         << " | ws_window=" << g_ws << "\n";
    cout << "hits=" << hits << "  faults=" << faults
         << "  frames_usados=" << loaded.size() << "\n";

    cout << "loaded_pages={";
    bool first = true;
    for (const auto &p : loaded)
    {
        if (!first)
            cout << ", ";
        cout << "(" << p.first << "," << p.second << ")";
        first = false;
    }
    cout << "}\n";

    if (g_ws > 0)
    {
        cout << "last_ws=(";
        for (size_t i = 0; i < ws_window_buf.size(); ++i)
        {
            if (i)
                cout << " ";
            cout << ws_window_buf[i].first << ":" << ws_window_buf[i].second;
        }
        cout << ")\n";
    }
}
