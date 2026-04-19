#include "TransTable.h"

void TranspositionTable::store(uint64_t key, const TTEntry& entry) {
    std::lock_guard<std::mutex> lock(mtx);
    table[key] = entry;
}

bool TranspositionTable::probe(uint64_t key, TTEntry& entry) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = table.find(key);
    if (it != table.end()) {
        entry = it->second;
        return true;
    }
    return false;
}

void TranspositionTable::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    table.clear();
}