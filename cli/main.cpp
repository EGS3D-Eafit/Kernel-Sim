#include <iostream>
#include "../modules/cpu/scheduler.h"

int main() {
    Scheduler planificador(3); // quantum = 3

    PCB p1(1, 5);
    PCB p2(2, 7);
    PCB p3(3, 4);

    planificador.agregarProceso(&p1);
    planificador.agregarProceso(&p2);
    planificador.agregarProceso(&p3);

    planificador.ejecutar();

    return 0;
}
