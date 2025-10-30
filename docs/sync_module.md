# Módulo de Sincronización y Concurrencia

Este módulo agrega:

- `Semaphore`: semáforo de conteo simple (también sirve como binario con valor inicial 1).
- `BoundedBuffer<T>`: buffer acotado **thread-safe** para productor–consumidor.
- `pc_cli`: adaptador minimalista para exponer comandos `produce`, `consume`, `stat` **sin tocar** la CLI principal (la integra _Ballesta_).

## Estructura

```
modules/concurrency/
  semaphore.h
  bounded_buffer.h
  pc_cli.h
  pc_cli.cpp
```

## Cómo integrarlo en la CLI existente (Ballesta)

En el parser de comandos, agregar:

```cpp
#include "../modules/concurrency/pc_cli.h"

// Al iniciar la app (opcional, capacidad por defecto 5)
pc_cli::init(5);

// En el loop de comandos:
if (cmd == "produce") {
    // opcional: `produce 42`
    if (args.size() >= 2) {
        int v = std::stoi(args[1]);
        std::cout << pc_cli::produce(v) << std::endl;
    } else {
        std::cout << pc_cli::produce() << std::endl;
    }
} else if (cmd == "consume") {
    std::cout << pc_cli::consume() << std::endl;
} else if (cmd == "stat") {
    std::cout << pc_cli::stat() << std::endl;
}
```

## Notas

- `BoundedBuffer::produce` **bloquea** si el buffer está lleno; `try_consume()` no bloquea (útil para CLI paso a paso).
- Si quieren una simulación con hilos reales, pueden lanzar `std::thread` que llamen a `produce/consume` en bucles.
- Para pruebas, pueden usar `pc_cli` directamente sin hilos.
