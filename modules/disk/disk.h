#pragma once

#include <string>
#include <vector>
#include <variant>
#include <optional>

// Necesitamos porque Directory::command usa optional<CPU>
#include "../cpu/pcb.h"
#include "../cpu/cpu.h"

// ============================================================
//   Sistema de archivos simulado (FS): Information / File / Directory / Disk
// ============================================================

class Information
{
public:
    virtual ~Information() = default;

    std::string get_name();

    virtual std::variant<std::string, PCB *, std::vector<Information *>> get_content();

    void set_content(const std::string &content);
    void set_content(const std::vector<Information *> &content);
    void set_content(const PCB &process);

    virtual int get_size();

    explicit Information(const std::string &name);

    template <typename T>
    Information(const std::string &name, T content); // ctor genérico

protected:
    std::string name;
    std::variant<std::string, PCB *, std::vector<Information *>> content;
};

class File : public Information
{
public:
    int get_size() override;
    std::string get_extension();

    void edit_content(const std::string &content); // para .txt
    void edit_content(PCB *program);               // para .exe

    File(const std::string &name, const std::string &extension);

    template <typename T>
    File(const std::string &name, const std::string &extension, T content);

private:
    std::string extension;
};

class Directory : public Information
{
public:
    int get_size() override;

    // Comandos de FS: "new X", "ls", "run nombre", "kill X"
    std::string command(std::string command, std::optional<CPU> s);

    void push_content(Information *info);

    explicit Directory(const std::string &name);

private:
    std::string new_(std::string n);
    std::string ls_();
    std::string run_(const std::string &n, CPU &s);
    std::string kill_(const std::string &n);
};

class Disk
{
public:
    std::string get_name();
    std::string get_current_directory_path();
    Directory *get_current_directory();

    // Navegación: "cd nombre", "cd ..", "cd /"
    std::string go_to_path(const std::string &path);

    explicit Disk(const std::string &name);

private:
    std::string name;

    // navegación
    std::string cur_path;
    Directory *root;
    Directory *cur_dir = nullptr;
    std::vector<Directory *> path_stack;
};

namespace diskdemo
{

    void run_trace_file(const std::string &path);

    // Muestra recorrido y estado del cabezal
    void stat();
}
