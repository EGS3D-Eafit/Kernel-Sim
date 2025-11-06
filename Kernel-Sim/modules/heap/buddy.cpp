
#include "buddy.h"
#include <sstream>
#include <cmath>
#include <algorithm>

void Buddy::clear() {
    free_blocks.clear();
    used.clear();
    next_id = 1;
}

size_t Buddy::roundPow2(size_t x) const {
    size_t p = 1;
    while (p < x) p <<= 1;
    return p;
}

void Buddy::init(size_t size_power2) {
    N = roundPow2(size_power2);
    clear();
    free_blocks.insert({N, 0});
}

int Buddy::alloc(size_t bytes) {
    if (bytes == 0) bytes = 1;
    size_t need = roundPow2(bytes);
    // find first block with size >= need
    auto it = free_blocks.lower_bound(need);
    if (it == free_blocks.end()) {
        // try larger blocks
        for (auto jt = free_blocks.begin(); jt != free_blocks.end(); ++jt) {
            if (jt->first >= need) { it = jt; break; }
        }
        if (it == free_blocks.end()) return -1;
    }
    size_t sz = it->first;
    size_t off = it->second;
    free_blocks.erase(it);
    // split until exact size
    while (sz > need) {
        sz >>= 1;
        // buddy split: keep left half, push right half
        free_blocks.insert({sz, off + sz});
    }
    int id = next_id++;
    used[id] = {off, sz};
    return id;
}

bool Buddy::free(int id) {
    auto it = used.find(id);
    if (it == used.end()) return false;
    size_t off = it->second.off;
    size_t sz = it->second.size;
    used.erase(it);
    // coalescing
    while (true) {
        size_t buddy_off = off ^ sz; // flip bit at size
        auto range = free_blocks.equal_range(sz);
        bool merged = false;
        for (auto jt = range.first; jt != range.second; ++jt) {
            if (jt->second == buddy_off) {
                // merge
                if (buddy_off < off) off = buddy_off;
                free_blocks.erase(jt);
                sz <<= 1;
                merged = true;
                break;
            }
        }
        if (!merged) break;
    }
    free_blocks.insert({sz, off});
    return true;
}

std::string Buddy::stats() const {
    std::ostringstream os;
    os << "=== HEAP (Buddy) ===\\n";
    os << "Total: " << N << " bytes\\n";
    size_t free_sum = 0;
    os << "Free blocks:\\n";
    for (auto &kv : free_blocks) {
        os << "  size=" << kv.first << " off=" << kv.second << "\\n";
        free_sum += kv.first;
    }
    size_t used_sum = 0;
    os << "Used blocks:\\n";
    for (auto &kv : used) {
        os << "  id=" << kv.first << " off=" << kv.second.off << " size=" << kv.second.size << "\\n";
        used_sum += kv.second.size;
    }
    os << "Total used=" << used_sum << " | total free=" << free_sum << "\\n";
    return os.str();
}
