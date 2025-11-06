#include "io.h"
#include <iostream>

void IO::escribir(const std::string& msg) {
    std::cout << "[IO] " << msg << std::endl;
}
