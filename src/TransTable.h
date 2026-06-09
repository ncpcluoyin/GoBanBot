/**
 * @file TransTable.h
 * @brief 置换表（Transposition Table）定义
 *
 * 使用 Zobrist 哈希键存储搜索过的局面信息，避免重复搜索。
 * 支持三种节点类型：精确值 (EXACT)、下界 (LOWER)、上界 (UPPER)。
 */

#ifndef TRANSTABLE_H
#define TRANSTABLE_H

#include <cstdint>
#include <unordered_map>
#include <mutex>
#include "Board.h"

/** @brief 置换表条目标记，区分 Alpha-Beta 搜索中的节点类型 */
enum Flag {
    EXACT,  /**< 精确值：搜索得到了该局面的准确评分 */
    LOWER,  /**< 下界：实际值 >= 存储值（因 beta 剪枝中止） */
    UPPER   /**< 上界：实际值 <= 存储值（因 alpha 未能提升） */
};

/** @brief 置换表单一条目 */
struct TTEntry {
    int depth;      /**< 搜索深度 */
    int value;      /**< 局面评分 */
    Flag flag;      /**< 节点类型标记 */
    Move bestMove;  /**< 该局面下的最优着法（用于着法排序启发） */
};

/**
 * @brief 线程安全的置换表
 *
 * 底层使用 unordered_map 存储，通过 std::mutex 保证多线程并发读写安全。
 * 键值为 Zobrist 哈希生成的 64 位无符号整数。
 */
class TranspositionTable {
public:
    /** @brief 存储条目（覆盖已存在的同键条目） */
    void store(uint64_t key, const TTEntry& entry);

    /** @brief 查询条目，命中则返回 true 并填充 entry */
    bool probe(uint64_t key, TTEntry& entry) const;

    /** @brief 清空置换表 */
    void clear();

private:
    std::unordered_map<uint64_t, TTEntry> table;  /**< 哈希表存储 */
    mutable std::mutex mtx;  /**< 互斥锁，保证多线程安全读写 */
};

#endif