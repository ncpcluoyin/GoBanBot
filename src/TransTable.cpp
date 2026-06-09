/**
 * @file TransTable.cpp
 * @brief 置换表实现
 *
 * 所有公开接口均使用 lock_guard 加锁，保证在多线程搜索环境下
 * store / probe / clear 操作的原子性和数据一致性。
 */

#include "TransTable.h"

void TranspositionTable::store(uint64_t key, const TTEntry& entry) {
    std::lock_guard<std::mutex> lock(mtx);  // 加锁保护写入
    table[key] = entry;                      // 直接覆盖可能存在的旧条目
}

bool TranspositionTable::probe(uint64_t key, TTEntry& entry) const {
    std::lock_guard<std::mutex> lock(mtx);  // 加锁保护读取
    auto it = table.find(key);
    if (it != table.end()) {
        entry = it->second;                  // 命中：拷贝条目
        return true;
    }
    return false;                            // 未命中
}

void TranspositionTable::clear() {
    std::lock_guard<std::mutex> lock(mtx);  // 加锁保护清空操作
    table.clear();
}