#pragma once
#include <iostream>
#include <string>
#include <atomic>

using namespace std;

enum class ProcState
{
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    SUSPENDED,
    TERMINATED
};

class PCB
{
public:
    static int next_pid;

    int pid;
    string name;
    int tiempoEjecucion; // tiempo total restante (en “unidades”)
    ProcState state;

    // Métricas simples
    int arrived_order = 0; // para desempates en SJF y orden de alta

    PCB(const string &name, int tiempoEjecucion);

    // Ejecuta hasta 'tick' unidades o hasta terminar
    // Retorna cuánto ejecutó efectivamente.
    int ejecutar(int tick);

    bool terminado() const;

    // Transiciones de estado
    void suspend();
    void resume();
    void block();
    void unblock();
    void terminate();
};
