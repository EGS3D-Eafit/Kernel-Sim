#include <iostream>
#include <bits/stdc++.h>
#include "../modules/disk/disk.h"
#include "../modules/cpu/pcb.h"
#include "../modules/cpu/cpu.h"

vector<string> spltstring(const string &str, const string &delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end;

    while ((end = str.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }

    tokens.push_back(str.substr(start)); // Último fragmento
    return tokens;
}

int main() {

    Disk disk("C");
    CPU cpu(5);

    string input;
    while (true) {
        cout << disk.get_current_directory_path() << " > ";
        getline(cin, input);

        if (input == "exit") {
            cout << "Saliendo del sistema...\n";
            break;
        }

        if (input == "help") {
            cout << "Comandos disponibles:\n";
            cout << "  new nombre.ext      - Crear archivo/programa\n";
            cout << "  new nombre          - Crear directorio\n";
            cout << "  ls                  - Listar contenido\n";
            cout << "  edit nombre.ext     - Editar archivo/programa\n";
            cout << "  run nombre          - Ejecutar proceso\n";
            cout << "  kill nombre.ext     - Eliminar archivo\n";
            cout << "  kill nombre         - Eliminar directorio\n";
            cout << "  cd nombre           - Cambiar a subdirectorio\n";
            cout << "  cd ..               - Subir un nivel\n";
            cout << "  cd /                - Ir a raíz\n";
            cout << "  pwd                 - Mostrar ruta actual\n";
            cout << "  exit                - Salir\n";
            continue;
        }

        if (input == "pwd") {
            cout << "Ruta actual: " << disk.get_current_directory_path() << "\n";
            continue;
        }

        if (input.rfind("cd ", 0) == 0) {
            string path = input.substr(3);
            cout << disk.go_to_path(path) << "\n";
            continue;
        }

        Directory* current = disk.get_current_directory();


        if (input.rfind("edit ", 0) == 0) {
            string arg = input.substr(5);
            auto split = spltstring(arg, ".");

            if (split.size() != 2) {
                cout << "Formato inválido. Usa: edit nombre.ext\n";
                continue;
            }

            Directory* current = disk.get_current_directory();
            auto cur_cont = current->get_content();
            auto vec = get_if<vector<Information*>>(&cur_cont);
            if (!vec) {
                cout << "El directorio actual no tiene contenido válido.\n";
                continue;
            }

            bool found = false;
            for (Information* info : *vec) {
                File* file = dynamic_cast<File*>(info);
                if (file && file->get_name() == split[0] && file->get_extension() == split[1]) {
                    found = true;
                    if (file->get_extension() == "txt") {
                        cout << "Ingresa el nuevo contenido del archivo:\n";
                        string nuevoContenido;
                        getline(cin, nuevoContenido);
                        file->edit_content(nuevoContenido);
                        cout << "Archivo editado correctamente.\n";
                    } else if (file->get_extension() == "exe") {
                        cout << "Ingresa el nuevo tiempo de ejecución del proceso:\n";
                        int tiempo;
                        cin >> tiempo;
                        cin.ignore(); // Limpiar el buffer
                        PCB* nuevoPCB = new PCB(file->get_name(), tiempo);
                        file->edit_content(nuevoPCB);
                        cout << "Proceso editado correctamente.\n";
                    } else {
                        cout << "Tipo de archivo no editable.\n";
                    }
                    break;
                }
            }

            if (!found) {
                cout << "Archivo no encontrado.\n";
            }

            continue;
        }


        if (input.rfind("new ", 0) == 0) {
            string arg = input.substr(4);
            cout << current->command("new " + arg, {}) << "\n";
            continue;
        }

        if (input == "ls") {
            cout << current->command("ls", {}) << "\n";
            continue;
        }

        if (input.rfind("run ", 0) == 0) {
            string arg = input.substr(4);
            cout << current->command("run " + arg, cpu) << "\n";
            continue;
        }

        if (input.rfind("kill ", 0) == 0) {
            string arg = input.substr(5);
            cout << current->command("kill " + arg, {}) << "\n";
            continue;
        }

        cout << "Comando no reconocido. Escribe 'help' para ver opciones.\n";
    }

    return 0;
}

