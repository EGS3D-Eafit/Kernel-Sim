#include "../modules/cpu/cpu.h"

int main() {
    CPU cpu(2); // quantum = 2

    cpu.agregarProceso(PCB(1, 5));
    cpu.agregarProceso(PCB(2, 3));
    cpu.agregarProceso(PCB(3, 7));

    cpu.ejecutarRoundRobin();

    return 0;
}
