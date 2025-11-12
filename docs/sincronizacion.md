
# ⚙️ Diseño de Sincronización e Invariantes

## Objetivo
Describir la implementación de los mecanismos de sincronización dentro del simulador *Kernel‑Sim*, asegurando la correcta interacción entre procesos concurrentes.

## Diseño general
Se emplean `std::mutex` y `std::condition_variable` como primitivas básicas.  
Se implementan dos módulos principales:

- `sync pc` — **Productor‑Consumidor**
- `sync ph` — **Filósofos comensales**

---

## 🧺 Productor‑Consumidor

### Descripción
Implementa múltiples productores y consumidores que comparten un buffer circular de capacidad fija.

### Comandos de prueba
```bash
sync pc start 3 2 5 100 150 --silencio
sync pc stat
sync pc stop
```

### Invariantes
1. `0 ≤ buffer.count ≤ buffer.capacidad`
2. Ningún productor escribe si el buffer está lleno.
3. Ningún consumidor lee si el buffer está vacío.
4. Al detener (`stop()`), todos los hilos finalizan limpiamente.

### Flujo
1. `start()` lanza hilos productores/consumidores.
2. Los productores bloquean al llenar el buffer.
3. Los consumidores bloquean al vaciarlo.
4. `stat` muestra métricas de ejecución.

---

## 🍽️ Filósofos comensales

### Descripción
Simula `n` filósofos que alternan entre pensar y comer usando mutex por tenedor.

### Comandos de prueba
```bash
sync ph start 5 120 80 --silencio
sync ph stat
sync ph stop
```

### Invariantes
1. Un solo filósofo puede usar cada tenedor a la vez.
2. Nadie toma dos tenedores mientras un vecino espera uno.
3. No hay inanición (cada filósofo eventualmente come).

### Flujo
- Cada filósofo piensa → intenta comer → come → suelta los tenedores.
- Se evita deadlock alternando el orden de adquisición o numeración.

---

## Validación
Durante la simulación:
- `sync pc stat` reporta items producidos y consumidos sin pérdida.
- `sync ph stat` muestra estado de ejecución (`ejecutando=sí`, `n=5`).

Los invariantes se verifican implícitamente: ningún hilo accede a recursos fuera de los límites definidos y todos los bloqueos se liberan correctamente.
