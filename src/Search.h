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

    /** @brief 设置空着剪枝缩减量，0 表示禁用 */
    void setNullMoveReduction(int r) { nullMoveR = r; }

    /** @brief 设置外部停止标志（用于超时/用户中断） */
    void setStopFlag(std::shared_ptr<std::atomic<bool>> flag) { stopFlag = flag; }

    /** @brief 设置根着法边界过滤（开局限制用），min/max 均为 inclusive */
    void setRootBounds(int minX, int minY, int maxX, int maxY) {
        rootMinX = minX; rootMinY = minY;
        rootMaxX = maxX; rootMaxY = maxY;
        useRootFilter = true;
    }
    /** @brief 清除根着法边界过滤 */
    void clearRootBounds() { useRootFilter = false; }

    /**
     * @brief 获取当前局面的最佳着法
     *
     * 根据线程数自动选择单线程迭代加深或多线程并行根搜索。
     */
    Move getBestMove(const Board& board);

    /**
     * @brief 获取当前局面评分最高的前 N 个着法
     *
     * 对每个根着法执行全深度搜索，按评分降序返回。
     * 用于五手两打规则中的 AI 候选着法生成。
     */
    std::vector<Move> getTopMoves(const Board& board, int n);

    /**
     * @brief 获取评分最高的前 N 个着法（含评分）
     *
     * 返回 (着法, 评分) 对，评分从当前走棋方视角（越高越有利）。
     * 用于三手交换中的均衡着法选择。
     */
    std::vector<std::pair<Move, int>> getTopMovesScored(const Board& board, int n);

private:
    int maxDepth;                                   /**< 最大搜索深度 */
    int numThreads;                                 /**< 搜索线程数 */
    int maxTimeMs;                                  /**< 单步最大限时（毫秒） */
    int nullMoveR;                                  /**< 空着剪枝缩减量（0=禁用） */
    TranspositionTable tt;                          /**< 置换表 */
    std::shared_ptr<std::atomic<bool>> stopFlag;    /**< 外部停止标志 */
    std::atomic<bool> internalStop;                 /**< 内部停止标志 */

    static const int INF = 1000000;                 /**< 极大值 */
    static const int WIN_SCORE = 100000;            /**< 胜利分值基准 */
    static const int ASP_WINDOW = 300;              /**< Aspiration 窗口半宽 */
    static const int MAX_DEPTH = 64;                /**< 最大搜索层数（用于 killer 数组） */
    static const int NULL_MOVE_R = 3;               /**< 空着剪枝的深度缩减量 */
    static const int SCORE_OPEN_THREAT = 10000;    /**< 空着剪枝威胁阈值：评估低于此值禁用空着 */

    /** @brief Killer move 表：killerMoves[slot][depth]，slot 0 为最新 */
    Move killerMoves[2][MAX_DEPTH];

    /** @brief 历史启发表：history[x][y] 记录着法引发剪枝的累积分数 */
    int history[Board::SIZE][Board::SIZE];

    /** 根着法边界过滤 */
    int rootMinX = 0, rootMinY = 0, rootMaxX = 14, rootMaxY = 14;
    bool useRootFilter = false;

    /** @brief 按边界过滤着法列表 */
    std::vector<Move> filterByBounds(const std::vector<Move>& moves) const;

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
    int negamax(int alpha, int beta, int depth, Board& board, int color, uint64_t& nodeCount,
                bool allowNull = true);

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
     * 3. 其余着法按历史启发 + 位置评分排序
     */
    void orderMoves(std::vector<Move>& moves, const Board& board, int color,
                    const Move& hashMove, int depth);

    /** @brief 迭代加深搜索（单线程） */
    Move iterativeDeepening(const Board& rootBoard);

    /** @brief 多线程并行根搜索 */
    Move parallelRootSearch(const Board& rootBoard);
};

#endif
