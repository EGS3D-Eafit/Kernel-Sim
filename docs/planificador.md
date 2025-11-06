# Planificador de CPU: Diseño, Supuestos y Evaluación (RR y SJF)

## 1. Objetivo

Diseñar y evaluar un planificador de CPU que soporte al menos **dos algoritmos**:

- **Round Robin (RR)** con **quantum** fijo.
- **Shortest Job First (SJF)** no expropiativo.

Se analizan **tiempos de espera**, **tiempos de retorno** y **fairness** (equidad de CPU).

---

## 2. Modelo de Proceso (PCB)

Cada proceso se representa con un **PCB** con los atributos principales:

- `id`: identificador único.
- `estado`: `Nuevo`, `Listo`, `Ejecutando`, `Bloqueado`, `Terminado`.
- `rafagas`: lista o contador de **tiempo CPU restante** (en ticks).
- Tiempos auxiliares:
  - `t_llegada`: tick en que el proceso entra al sistema/cola.
  - `t_inicio`: primer tick en CPU (para tiempo de respuesta si se requiere).
  - `t_fin`: tick en que termina.
  - `cpu_recibida`: ticks efectivamente ejecutados (para fairness).

> **Máquina de estados**: `Nuevo → Listo → Ejecutando → (Bloqueado ↔ Listo)* → Terminado`.

---

## 3. Algoritmos

### 3.1. Round Robin (RR)

- **Quantum** fijo `q` (por defecto `q = 3`).
- Cola de listos **FIFO** (`std::deque`).
- Regla: el proceso en CPU ejecuta hasta `min(q, ráfaga_restante)` y:
  - Si **termina**, pasa a `Terminado`.
  - Si **no termina**, vuelve al **final de la cola**.

**Ventajas**: buena **interactividad** y **fairness** (nadie monopoliza la CPU).  
**Desventajas**: cambia el orden natural; quantum mal elegido puede aumentar **cambios de contexto** o latencia.

### 3.2. Shortest Job First (SJF, no expropiativo)

- Selecciona entre `Listos` el proceso con **menor ráfaga restante**.
- Ejecuta **hasta completar** (sin expropiación).

**Ventajas**: minimiza el **tiempo de retorno promedio** si se conoce la ráfaga.  
**Desventajas**: posible **injusticia** con trabajos largos (starvation si llegan muchos cortos).

---

## 4. Supuestos de simulación

- **Llegadas**: por defecto, todos los procesos **llegan en `t=0`** salvo que se carguen dinámicamente desde disco/CLI.
- **Quantum (RR)**: `q = 3` si no se especifica con `quantum N`.
- **Cambio de contexto**: **costo 0** ticks (se puede extender con un parámetro si se requiere).
- **Bloqueos/IO**: no se modelan en este informe (el módulo IO existe por separado).
- **CPU monoprocesador**, un **tick** equivale a 1 unidad de tiempo.
- **SJF perfecto**: se asume ráfaga conocida (como proxy de “tiempo restante”).

> Si el docente pide variar algún supuesto, puede indicarse en la sección de **Experimentos** (p.ej., llegadas escalonadas o costo de cambio de contexto > 0).

---

## 5. Métricas

Para cada proceso `i`:

- **Tiempo de espera**:  
  \[
  W_i = (t_inicio - t_llegada) + \text{tiempo en colas intermedias}
  \]
  En la implementación, se acumula el tiempo total fuera de CPU desde `t_llegada` hasta `t_fin` menos el CPU recibido.
- **Tiempo de retorno** (_turnaround_):  
  \[
  T_i = t_fin - t_llegada
  \]
- **Tiempo de respuesta** (opcional):  
  \[
  R_i = t_inicio - t_llegada
  \]
- **Fairness** (índice de Jain sobre **CPU recibido**):  
  Sea \( x_i = \text{cpu_recibida}\_i \).  
  \[
  J = \frac{\left(\sum_i x_i\right)^2}{n \cdot \sum_i x_i^2} \quad \in (0, 1]
  \]
  **1 = perfectamente justo**. También se reporta la **varianza** de \(x_i\).

En tablas/CSV se reportan: `pid, t_llegada, t_inicio, t_fin, turnaround, espera, cpu_recibida`.

---

## 6. Interfaz de Línea de Comandos (CLI) relevante

**Comandos básicos**

```bash
sched rr # o: sched sjf
quantum 3 # solo RR
new P1 6 # proceso de 6 ticks
new P2 9
new P3 4
run 50 # avanza 50 ticks (o los necesarios)
ps # muestra procesos y estado
```

**Exportar métricas**

```bash
cpu stats results/cpu/rr_q3.csv

Campos esperados en `rr_q3.csv`:

pid, t_llegada, t_inicio, t_fin, turnaround, espera, cpu_recibida
```

---

## 7. Resultados esperados (patrones)

- **SJF**: menor **turnaround promedio** cuando todos llegan en `t=0` y se conoce la ráfaga.
- **RR**: mejor **fairness** (Jain más cercano a 1), **latencia acotada** por `q`.
- **Sensibilidad al quantum**:
  - **q pequeño** → más cambios de contexto, mejor respuesta interactiva.
  - **q grande** → RR se aproxima a FCFS.

Incluye en el informe las **gráficas** (si exportas a CSV):

- **`turnaround promedio` vs `número de procesos`** (RR vs SJF).
- **`Jain` vs `quantum`** para RR (bajo un conjunto fijo de procesos).

---

## 8. Discusión y trade-offs

- **RR** favorece equidad y tiempos de respuesta razonables para **tareas interactivas**.
- **SJF** optimiza **throughput/turnaround** cuando se conocen ráfagas, pero puede perjudicar a procesos largos.
- En sistemas reales, se usan **híbridos** (RR con colas multinivel, SRTF, prioridades + aging).
- Elegimos **RR** como política por defecto por su **robustez** y **predictibilidad**; SJF se expone para mostrar el extremo óptimo en turnaround bajo supuestos ideales.

---

## 9. Reproducibilidad (scripts)

Ver [`experimentos.md`](./experimentos.md) para:

- Scripts de CPU `scripts/cpu/*.txt`.
- Formato CSV y rutas (`results/cpu/*.csv`).
- Cómo generar las figuras (opcional) en `figs/`.

---
