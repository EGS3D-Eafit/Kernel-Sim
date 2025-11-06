
#pragma once
#include <vector>
#include <map>
#include <string>
#include <optional>

class Buddy {
public:
    void init(size_t size_power2 = 1024);
    int alloc(size_t bytes);
    bool free(int id);
    std::string stats() const;
private:
    size_t N = 0; // total bytes (power of two)
    std::multimap<size_t, size_t> free_blocks; // size -> offset
    struct Block { size_t off; size_t size; };
    std::map<int, Block> used; // id -> block
    int next_id = 1;
    void clear();
    size_t roundPow2(size_t x) const;
};
