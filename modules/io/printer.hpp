#pragma once
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <random>

namespace os
{
    namespace io
    {

        // Clase que simula una impresora compartida entre múltiples hilos (empleados)
        class Printer
        {
        private:
            std::mutex mtx;
            std::condition_variable cv;
            bool busy = false;

        public:
            // Solicitar acceso a la impresora
            void request(int id);

            // Liberar la impresora después de imprimir
            void release(int id);
        };

        // Función que simula el trabajo de un empleado
        void employee_task(Printer &printer, int id);

        // Prueba general con N empleados
        void demo_printer(int num_empleados = 5);

    }
}
