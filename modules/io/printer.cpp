#include "printer.hpp"

namespace os
{
    namespace io
    {

        void Printer::request(int id)
        {
            std::unique_lock<std::mutex> lock(mtx);
            std::cout << "Empleado " << id << " está esperando para imprimir.\n";

            // Esperar hasta que la impresora esté libre
            cv.wait(lock, [this]()
                    { return !busy; });

            // Fase crítica
            busy = true;
            std::cout << "Empleado " << id << " está imprimiendo...\n";
        }

        void Printer::release(int id)
        {
            std::unique_lock<std::mutex> lock(mtx);
            std::cout << "Empleado " << id << " ha terminado de imprimir.\n";

            busy = false;
            cv.notify_one(); // Despierta un hilo que esté esperando
        }

        void employee_task(Printer &printer, int id)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> prep(1, 3);

            int prep_time = prep(gen);
            std::this_thread::sleep_for(std::chrono::seconds(prep_time));

            printer.request(id);

            std::this_thread::sleep_for(std::chrono::seconds(2)); // simulación de impresión
            printer.release(id);
        }

        void demo_printer(int num_empleados)
        {
            Printer printer;
            std::vector<std::thread> empleados;

            for (int i = 1; i <= num_empleados; ++i)
            {
                empleados.emplace_back(employee_task, std::ref(printer), i);
            }

            for (auto &hilo : empleados)
                hilo.join();

            std::cout << "Todos los empleados han terminado de imprimir.\n";
        }

    }
}
