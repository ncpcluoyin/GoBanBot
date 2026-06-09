/**
 * @file Search.h
 * @brief AI 搜索引擎定义
 *
 * 基于 Negamax + Alpha-Beta 剪枝的搜索算法，支持：
 * - 迭代加深：逐层加深搜索，确保指定时间/深度内获得最佳结果
 * - 多线程并行根搜索：将根节点着法分块并行计算
 * - 置换表加速：利用 Zobrist 哈希避免重复搜索
 * - 着法排序：置换表启发 + killer move + 位置评分排序，提高剪枝效率
 * - 单步最大限时：超时自动返回当前最佳着法
 * - Aspiration Window：渐进式搜索窗口，加速深层迭代
 */

#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"
#include "TransTable.h"
#include <atomic>
#include <vector>
#include <thread>
#include <future>
#include <memory>
#include <chrono>

class Search {
public:
    Search();

    /** @brief 设置最大搜索深度 */
    void setDepth(int d) { maxDepth = d; }

    /** @brief 设置搜索线程数 */
    void setThreads(int n) { numThreads = n; }

    /** @brief 设置单步最大限时（毫秒），0 表示不限时 */
    void setMaxTime(int ms) { maxTimeMs = ms; }

    /** @brief 设置外部停止标志（用于超时/用户中断） */
    void setStopFlag(std::shared_ptr<std::atomic<bool>> flag) { stopFlag = flag; }

    /**
     * @brief 获取当前局面的最佳着法
     *
     * 根据线程数自动选择单线程迭代加深或多线程并行根搜索。
     */
    Move getBestMove(const Board& board);

private:
    int maxDepth;                                   /**< 最大搜索深度 */
    int numThreads;                                 /**< 搜索线程数 */
    int maxTimeMs;                                  /**< 单步最大限时（毫秒） */
    TranspositionTable tt;                          /**< 置换表 */
    std::shared_ptr<std::atomic<bool>> stopFlag;    /**< 外部停止标志 */
    std::atomic<bool> internalStop;                 /**< 内部停止标志 */

    static const int INF = 1000000;                 /**< 极大值 */
    static const int WIN_SCORE = 100000;            /**< 胜利分值基准 */
    static const int ASP_WINDOW = 300;              /**< Aspiration 窗口半宽 */
    static const int MAX_DEPTH = 64;                /**< 最大搜索层数（用于 killer 数组） */

    /** @brief Killer move 表：killerMoves[slot][depth]，slot 0 为最新 */
    Move killerMoves[2][MAX_DEPTH];

    /** @brief 搜索开始时间点 */
    std::chrono::steady_clock::time_point startTime;

    /** @brief 检查是否超过单步限时 */
    bool isTimeUp() const;

    /**
     * @brief Negamax + Alpha-Beta 剪枝搜索
     * @param alpha  下界
     * @param beta   上界
     * @param depth  剩余搜索深度
     * @param board  当前局面（引用传递，递归中会回退）
     * @param color  当前走棋方（BLACK=1 或 WHITE=-1）
     * @param nodeCount 已访问节点计数器（引用，用于定期时间检查）
     * @return 当前局面的评分（从 color 视角）
     */
    int negamax(int alpha, int beta, int depth, Board& board, int color, uint64_t& nodeCount);

    /**
     * @brief 静态局面评估
     * @return color 视角下的评分（正数有利于 color）
     */
    int evaluate(const Board& board, int color) const;

    /**
     * @brief 对着法列表排序，提高剪枝效率
     *
     * 排序优先级：
     * 1. 置换表记录的最佳着法（hashMove）置于首位
     * 2. Killer move（兄弟节点产生剪枝的着法）
     * 3. 其余着法按靠近中心和邻近已有棋子数排序
     */
    void orderMoves(std::vector<Move>& moves, const Board& board, int color,
                    const Move& hashMove, int depth);

    /** @brief 迭代加深搜索（单线程） */
    Move iterativeDeepening(const Board& rootBoard);

    /** @brief 多线程并行根搜索 */
    Move parallelRootSearch(const Board& rootBoard);
};

#endif
