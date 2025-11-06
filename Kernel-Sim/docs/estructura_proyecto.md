## Estructura del Proyecto — Núcleo de Sistema Operativo Simulado

### Descripción General

Este documento explica la **estructura base del proyecto** del núcleo de sistema operativo simulado desarrollado en **C++**, su **propósito**, y el **aporte de cada componente** dentro del sistema.  

El objetivo principal del proyecto es **simular el funcionamiento interno de un sistema operativo simplificado**, integrando componentes esenciales como:
- Gestión de memoria  
- Planificación de procesos  
- Sincronización y comunicación entre procesos  
- Entrada/Salida básica  

Esta estructura base constituye el **esqueleto inicial del proyecto**, sobre el cual se implementarán progresivamente los módulos funcionales.

---

## Estructura de Carpetas

### `/kernel-sim`
Contiene el **núcleo principal del sistema**.  
Aquí se ubican los archivos fuente que definen el comportamiento central del sistema operativo, como:
- Inicialización del kernel.
- Bucle principal de ejecución.
- Gestión general de recursos.

**Aporta:** el punto de entrada del sistema, donde se coordinan los demás módulos.  
**Estado actual:** base conceptual (sin lógica funcional).

---

### `/modules/`
Carpeta que agrupa todos los **módulos funcionales del sistema**, organizados por áreas clave.  
Cada subcarpeta representa un **componente esencial del sistema operativo**.

#### `/modules/cpu`
Simula la **unidad central de procesamiento**.  
Incluye clases y funciones encargadas de:
- Planificación de procesos (round robin, FIFO, etc.)
- Ejecución de instrucciones simuladas.
- Manejo de interrupciones.

**Aporta:** la lógica que imita la ejecución de procesos.  
**Estado:** pendiente de implementación.

#### `/modules/mem`
Simula la **gestión de memoria**.  
Contendrá estructuras como:
- Tablas de páginas o segmentos.
- Algoritmos de asignación (First Fit, Best Fit, etc.)
- Simulación de espacio de direcciones y memoria virtual.

**Aporta:** representación de cómo un sistema operativo asigna y libera memoria.  
**Estado:** base conceptual.

#### `/modules/io`
Simula los **dispositivos de entrada/salida**.  
Ejemplo: teclado, pantalla, impresora o disco virtual.  
Gestiona colas de operaciones y tiempos de espera.

**Aporta:** modelo de comunicación entre procesos y hardware simulado.  
**Estado:** pendiente de implementación.

#### `/modules/disk`
Simula el **almacenamiento secundario** (disco).  
Contendrá:
- Estructura de archivos básicos.
- Operaciones de lectura/escritura.
- Manejo de bloques o sectores simulados.

**Aporta:** persistencia y operaciones de E/S en segundo plano.  
**Estado:** conceptual.

---

### `/cli`
Contiene la **interfaz de línea de comandos (Command Line Interface)**.  
Permite que el usuario interactúe con el sistema simulado ejecutando comandos, por ejemplo:
```
run process1
show memory
list io
```

**Aporta:** capa de interacción entre el usuario y el kernel.  
**Estado:** pendiente de implementación básica (parser de comandos y menú principal).

---

### `/docs`
Directorio para toda la **documentación del proyecto**.  
Aquí se incluirán:
- Este documento de estructura.
- Manuales técnicos.
- Diagramas de arquitectura.
- Registro de versiones (CHANGELOG).

**Aporta:** claridad sobre el diseño, decisiones y progresos del proyecto.  
**Estado:** inicializado.

---

### Plantillas de pruebas
Se crearán plantillas de pruebas unitarias (por ejemplo, usando `GoogleTest` o `Catch2`) para cada módulo dentro de una carpeta `tests/`.  
Estas pruebas permitirán verificar el correcto funcionamiento de cada componente (memoria, CPU, etc.) de forma aislada.

**Aporta:** calidad y validación continua del desarrollo.  
**Estado:** aún no implementado.

---

## Nivel de Avance Actual

Con la estructura creada, el proyecto se encuentra en la **fase de configuración inicial**:
- Organización del repositorio.  
- Definición de componentes principales.  
- Pendiente: implementación de lógica funcional por módulos.  
- Pendiente: integración y comunicación entre componentes.  
- Pendiente: CLI funcional y pruebas unitarias.  

Esta estructura representa aproximadamente un **10-15% del avance total**, ya que es el punto de partida sobre el cual se construirá toda la funcionalidad real del sistema operativo simulado.
