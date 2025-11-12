
# 🧠 Diseño del Planificador de CPU

## Objetivo
Implementar y comparar algoritmos de planificación de CPU dentro del simulador *Kernel‑Sim* (SJF no expropiativo y Round Robin).

## Diseño general
- **Entrada:** procesos definidos como `(id, llegada, ráfaga)`.
- **Salida:** tabla de métricas (tiempo de espera, retorno y fairness de Jain).
- **Estructuras internas:**
  - Cola de listos (prioridad por ráfaga o quantum).
  - Lista de procesos terminados.
  - Reloj interno (`tick`) que avanza simulando la CPU.

## Algoritmos implementados

### SJF (Shortest Job First, no expropiativo)
Selecciona el proceso con la menor ráfaga disponible en cada instante.
```bash
sched sjf  1,0,6  2,1,3  3,2,8  4,4,2  5,5,5
```
- Procesos con llegada tardía esperan hasta quedar disponibles.
- No hay cambio de contexto antes de terminar la ráfaga actual.

### Round Robin (RR)
Utiliza una cola circular y un quantum fijo.
```bash
sched rr 3  1,0,9  2,2,4  3,3,5  4,5,2
```
- Cada proceso recibe un tiempo máximo `quantum`.
- Si no termina, vuelve a la cola al final.

## Supuestos
- Todos los procesos son CPU‑bound (no hay E/S).
- Tiempos de llegada conocidos y discretos.
- Costo de cambio de contexto ignorado.
- Quantum constante durante toda la simulación.

## Resultados esperados
- Tiempos promedio de espera y retorno.
- Métrica de equidad: índice de Jain.
- Exportación a CSV o consola con la tabla de resultados.

## Evidencia
Comando de ejecución típico:
```bash
os> sched sjf 1,0,8 2,1,4 3,2,9 4,3,5
os> sched rr 2 1,0,5 2,1,3 3,2,8 4,3,6
```
Salida (parcial):
```
ID  Llegada  Ráfaga  Espera  Retorno
1   0        8       0       8
2   1        4       7       11
3   2        9       12      21
4   3        5       18      23
Fairness (Jain): 0.92
```
