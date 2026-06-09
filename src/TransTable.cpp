/**
 * @file TransTable.cpp
 * @brief 置换表实现
 *
 * 使用 shared_mutex 实现读写锁：
 * - probe() 使用 shared_lock，允许多线程并发读取
 * - store() / clear() 使用 unique_lock，独占写入
 */

#include "TransTable.h"
#include <mutex>

void TranspositionTable::store(uint64_t key, const TTEntry& entry) {
    std::unique_lock<std::shared_mutex> lock(mtx);
    table[key] = entry;
}

bool TranspositionTable::probe(uint64_t key, TTEntry& entry) const {
    std::shared_lock<std::shared_mutex> lock(mtx);
    auto it = table.find(key);
    if (it != table.end()) {
        entry = it->second;
        return true;
    }
    return false;
}

void TranspositionTable::clear() {
    std::unique_lock<std::shared_mutex> lock(mtx);
    table.clear();
}