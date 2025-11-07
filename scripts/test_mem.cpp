#include "../modules/mem/mem.h"

int main() {
    Memoria m(10);
    m.asignar(3);
    m.mostrar();
    m.liberar(0, 3);
    m.mostrar();
    return 0;
}
