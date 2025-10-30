#include "disk.h"
#include <iostream>
#include <optional>
#include <sstream>
#include <variant>
#include "../cpu/cpu.h"
// GENERAL


vector<string> splitstring(const string &str, const string &delimiter) {
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

// Information

string Information::get_name() {
    return Information::name;
}

variant<string, PCB*, vector<Information*>> Information::get_content() {
    return content;
}

void Information::set_content(const string& content) {
    this->content = content;
}

void Information::set_content(const vector<Information*>& content) {
    this->content = content;
}

void Information::set_content(const PCB& content) {
    this->content = new PCB(content);
}

int Information::get_size() {
    return 0;
}

Information::Information(const string& name) {
    this->name = name;
}

template<typename T>
Information::Information(const string& name, T content) {
    this->name = name;
    this->content = content;
}

// File

int File::get_size() {
    if (get_extension() == "exe") {
        return get<PCB*>(get_content())->tiempoEjecucion * 10;
    }

    if (get_extension() == "txt") {
        return get<string>(get_content()).length()*8;
    }
    return 0;
}

string File::get_extension() {
    return extension;
}

void File::edit_content(const string &content) {
    set_content(content);
}

void File::edit_content(PCB* program) {
    content = program;
}

File::File(const string &name, const string &extension): Information(name) {
    this->extension = extension;
}

template<typename T> //template
File::File(const string &name, const string &extension, T content): Information(name, content) {
    this->extension = extension;
}


// Directory

Directory::Directory(const string &name):Information(name) {}

void Directory::push_content(Information* info) {
    if (std::holds_alternative<std::vector<Information*>>(get_content())) {
        auto content = std::get<std::vector<Information*>>(get_content());
        content.push_back(info);
        set_content(content);
    } else {
        set_content(std::vector<Information*>{info});
    }
}

int Directory::get_size() {
    auto vec = get_if<vector<Information*>>(&content);
    if (vec) {
        int size = 0;
        for (Information* info : *vec) {
            size += info->get_size();
        }
        return size;
    }
    return 0;
}

string Directory::command(string command, optional<CPU> s) {
    stringstream ss(command);
    string n;
    ss >> n;

    if (n == "new") {
        ss >> n;
        return new_(n);
    } else if (n == "ls") {
        return ls_();
    } else if (n == "run") {
        ss >> n; // Extrae el siguiente argumento
        return run_(n, s.value());
    } else if (n == "kill") {
        ss >> n;
        return kill_(n);;
    }

    return "No se a reconocido el comando dado";
}

string Directory::new_(string n) {

    stringstream ss(n);
    ss >> n;
    if (n.empty())
        return "No se a podiddo realizar la operacion debido a que no se dio un argumento para crear.";

    vector<string> div = splitstring(n, ".");

    if (div.size() > 2)
        return "El archivo no es soportado por el sistema";

    Information* newInfo;
    string newType;
    if (div.size() == 2) {
        if (div[1] == "exe")
            newType = "Programa";
        else if (div[1] == "txt")
            newType = "Archivo";
        else
            return "El archivo no es soportado por el sistema";
        newInfo = new File(div[0], div[1]);
    }
    else {
        newInfo = new Directory(div[0]);
        newType = "Directorio";
    }

    push_content(newInfo);

    return "Se a creado exitosamente el " + newType;
}


string Directory::ls_() {
    try {
        string ls_ret;
        for (Information* info: get<vector<Information*>>(content)) {
            ls_ret += info->get_name();
            if (File* file = dynamic_cast<File*>(info))
                ls_ret += "." + file->get_extension();
            ls_ret += "\n";
        }
        return ls_ret;
    }catch (const std::exception& e) {
        return "hubo un error";
    }
}

string Directory::run_(const string& n, CPU s) {
    try {
        if (n.empty())
            return "No se a proveeido un nombre de un programa";
        for (Information* info: get<vector<Information*>>(content)) {
            File* file = dynamic_cast<File*>(info);

            if (file != nullptr && file->get_name()+".exe"==n)
                s.add_process(get<PCB*>(file->get_content()));
            else
                return "el proceso dado no existe en el directorio";
        }
        s.ejecutarRoundRobin(); //por ahora que se ejecute directamente
        return "proceso añadido a la cola para correr";
    }catch (const std::exception& e) {
        return "hubo un error";
    }
}

string Directory::kill_(const string& n) {
    try {
        auto& vec = get<vector<Information*>>(content);
        auto split = splitstring(n, ".");
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            File* file = dynamic_cast<File*>(*it);
            if (split.size() == 2 && file != nullptr) {
                if (split.size() == 2 && file->get_name() == split[0] && file->get_extension() == split[1]) {
                    vec.erase(it); // Elimina el proceso del directorio
                    return n + " eliminado correctamente.";
                }
            }
            else if (Directory* dir = dynamic_cast<Directory *>(*it)) {
                if (dir->get_name() == n) {
                    vec.erase(it); // Elimina el proceso del directorio
                    return n + " eliminado correctamente.";
                }
            }
        }
        return "El proceso '" + n + "' no fue encontrado.";
    } catch (const std::exception& e) {
        return "Hubo un error al intentar eliminar el proceso: " + string(e.what());
    }
}

// Disk

Disk::Disk(const string& name) {
    this->name = name;

    cur_path = "/";
    root = new Directory("/"); // Directorio raíz
    cur_dir = root;
}

string Disk::get_name() {
    return name;
}

string Disk::get_current_directory_path() {
    return cur_path;
}

Directory* Disk::get_current_directory() {
    return cur_dir;
}


string Disk::go_to_path(const string& path) {
    if (path == "/") {
        cur_dir = root;
        path_stack.clear();
        cur_path = "/";
        return "Ruta cambiada a raíz.";
    }

    if (path == "..") {
        if (!path_stack.empty()) {
            cur_dir = path_stack.back();
            path_stack.pop_back();
            cur_path = cur_path.substr(0, cur_path.find_last_of('/'));
            if (cur_path.empty()) cur_path = "/";
            return "Subiste un nivel.";
        } else {
            return "Ya estás en el directorio raíz.";
        }
    }

    auto vari = cur_dir->get_content();
    auto vec = get_if<vector<Information*>>(&vari);
    if (!vec) return "El directorio actual no tiene contenido válido.";

    for (Information* info : *vec) {
        Directory* subdir = dynamic_cast<Directory*>(info);
        if (subdir && subdir->get_name() == path) {
            path_stack.push_back(cur_dir);
            cur_dir = subdir;
            cur_path += (cur_path == "/" ? "" : "/") + path;
            return "Ruta cambiada a: " + cur_path;
        }
    }

    return "No se encontró el subdirectorio: " + path;
}



