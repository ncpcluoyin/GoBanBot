#ifndef TRANSTABLE_H
#define TRANSTABLE_H

#include <cstdint>
#include <unordered_map>
#include <mutex>
#include "Board.h"

enum Flag { EXACT, LOWER, UPPER };

struct TTEntry {
    int depth;
    int value;
    Flag flag;
    Move bestMove;
};

class TranspositionTable {
public:
    void store(uint64_t key, const TTEntry& entry);
    bool probe(uint64_t key, TTEntry& entry) const;
    void clear();

private:
    std::unordered_map<uint64_t, TTEntry> table;
    mutable std::mutex mtx;   // 添加互斥锁
};

#endif