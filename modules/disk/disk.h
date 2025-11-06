#pragma once
#include "disk.h"
#include <optional>
#include <variant>
#include <bits/stdc++.h>
#include "../cpu/cpu.h"

#ifndef DISK_H
#define DISK_H

using namespace std;

class Information {
public:
    virtual ~Information() = default;

    string get_name();
    virtual variant<string, PCB*, vector<Information*>> get_content(); // Cambiado a string para devolver contenido
    void set_content(const string& content);
    void set_content(const vector<Information*>& content);
    void set_content(const PCB& process);
    virtual int get_size();

    explicit Information(const string& name);

    template<typename T>
    Information(const string&, T content); // Constructor genérico
protected:
    string name;
    variant<string, PCB*, vector<Information*>> content; // Ahora soporta los 3 tipos
};

class File : public Information {
public:
    int get_size() override;
    string get_extension();
    void edit_content(const string& content);
    void edit_content(PCB* program);

    File(const string &name, const string &extension);
    template<typename T>
    File(const string &name, const string &extension, T content);
private:
    string extension;
};

class Directory : public Information {
public:
    int get_size() override;
    string command(string command, optional<CPU> s);
    void push_content(Information* info);

    explicit Directory(const string &name);
private:
    string new_(string n);
    string ls_();
    string run_(const string& n, CPU& s);
    string kill_(const string& n);
};

class Disk {
public:
    string get_name();
    string get_current_directory_path();
    Directory* get_current_directory();
    string go_to_path(const string& path);

    explicit Disk(const string &name);
private:
    string name;

    //navigation
    string cur_path;
    Directory* root;
    Directory* cur_dir = nullptr;
    vector<Directory*> path_stack;
};

#endif