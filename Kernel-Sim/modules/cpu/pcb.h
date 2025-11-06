#pragma once
#include <string>
#include <vector>

class PCB
{
public:
    enum class Estado
    {
        Listo,
        Ejecutando,
        Bloqueado,
        Terminado
    };

    // Atributos básicos
    int id;
    std::string name;

    // Modelo de ráfagas de CPU (simple): cada entrada son unidades de CPU.
    // Si no se usa, se toma total_cpu_restante.
    std::vector<int> rafagasCPU;

    // Tiempo restante total (si no se usan ráfagas explícitas)
    int total_cpu_restante;

    // Estado y métricas
    Estado estado = Estado::Listo;
    int llegada_tick = 0;
    int primera_respuesta_tick = -1;
    int fin_tick = -1;
    int tiempo_espera_acum = 0;

    // Constructores
    PCB(const std::string &name, int tiempoEjecucion);             // compatible con código existente
    PCB(const std::string &name, const std::vector<int> &rafagas); // con ráfagas

    // Un tick de CPU (consume 1 unidad)
    void ejecutar_un_tick();

    bool terminado() const;

    // Helpers de métricas
    int turnaround() const { return fin_tick >= 0 && llegada_tick >= 0 ? (fin_tick - llegada_tick) : -1; }

private:
    static int next_id;
};
