# Kernel-Sim — Núcleo de Sistema Operativo Simulado

## Integrantes

- Alejandro Jaramillo Rodríguez
- Athina Cappelletti
- David Alejandro Ballesteros Jaimes

## Descripción General

**Kernel-Sim** es una simulación funcional de un **núcleo de sistema operativo simplificado**, desarrollada en **C++**.  
Su propósito es **recrear los procesos internos básicos de un sistema operativo real**, incluyendo:

- Gestión de **memoria**
- **Planificación** de procesos
- **Sincronización** y **comunicación** entre procesos
- Mecanismos de **entrada/salida (E/S)**
- **Interfaz de línea de comandos (CLI)** para interacción con el usuario

Este proyecto tiene fines **académicos y educativos**, orientado a comprender cómo interactúan los componentes esenciales de un sistema operativo.

---

## Objetivo General

Simular el comportamiento básico de un núcleo de sistema operativo mediante la implementación modular de sus componentes principales, permitiendo la experimentación con algoritmos de planificación, manejo de memoria y comunicación entre procesos.

---

## Estructura del Proyecto

<img width="658" height="319" alt="image" src="https://github.com/user-attachments/assets/b7881db9-044d-4ef5-bd77-cc089a58db51" />

## Stack Tecnológico

- **Lenguaje principal:** C++
- **Interfaz:** CLI (Command Line Interface)
- **Sistema de compilación:** CMake o Makefile
- **Pruebas unitarias:** GoogleTest o Catch2

---

## Estado del Proyecto

# Informe Proyecto SO – Kernel-Sim

## 1. Planificador de CPU

- **Algoritmos**: Round Robin (quantum configurable) y SJF (no expropiativo).
- **Supuestos**: todos arriban a t=0; quantum=3 por defecto.
- **Métricas**: tiempo de espera, turnaround, fairness (distribución de CPU).

## 2. Memoria Virtual y Paginación

- **Gestor de marcos**: tamaño fijo, tablas por proceso.
- **Algoritmos**: FIFO, LRU y Working Set (τ configurable).
- **Métricas**: hits, misses, Miss Ratio, PFF.
- **Gráficas**: generadas desde CSV con `scripts/plot_memory.py`.

## 3. E/S y Recursos

- **Dispositivo**: impresora con dos colas (alta/normal).
- **Política**: prioridad fija (alta > normal). Métricas de throughput.

## 4. Planificación de Disco

- **Algoritmos**: FCFS, SSTF y SCAN.
- **Métrica**: movimiento total del cabezal (comparado en misma carga).
- **CSV**: `disk dump` para graficar recorrido.

## 5. Sincronización

- **Productor–Consumidor** (del módulo /sync).
- **Cena de los Filósofos**: estrategia camarero + orden total (sin interbloqueo).
- **Invariantes**: nunca dos vecinos comiendo; no deadlock.

## 6. Scripts y Reproducibilidad

- Scripts en `/scripts` para memoria, disco y CPU.
- Resultados en `/out` (CSV y PNG).
