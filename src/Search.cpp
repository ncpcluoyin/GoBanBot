/**
 * @file Search.cpp
 * @brief AI 搜索算法实现
 *
 * 实现基于 Negamax + Alpha-Beta 剪枝的五子棋 AI 搜索。
 * 包含着法排序启发、多线程并行根搜索和迭代加深框架。
 *
 * 搜索流程：
 * 1. 立即获胜检测 → 直接返回获胜着法
 * 2. 对手必胜威胁检测 → 必须堵住威胁点
 * 3. 正常迭代加深/多线程搜索 → 获得最优着法
 */

#include "Search.h"
#include <algorithm>
#include <cstring>
#include <iostream>

Search::Search() : maxDepth(6), numThreads(1), stopFlag(nullptr), internalStop(false) {}

// =========================================================================
//  静态评估
// =========================================================================

/**
 * @brief 从 color 视角评估局面
 *
 * 评估公式：color * (黑方得分 - 白方得分)
 * 当 color=BLACK(1) 时，返回黑优分；color=WHITE(-1) 时，返回白优分。
 */
int Search::evaluate(const Board& board, int color) const {
    int blackScore = board.evaluateColor(Board::BLACK);
    int whiteScore = board.evaluateColor(Board::WHITE);
    int score = blackScore - whiteScore;
    return color * score;
}

// =========================================================================
//  着法排序
// =========================================================================

/**
 * @brief 对着法列表排序，提高 Alpha-Beta 剪枝效率
 *
 * 排序策略：
 * 1. 如果置换表有记录的最佳着法（hashMove），将其交换到列表首位
 * 2. 其余着法按启发式评分降序排列：
 *    - 离棋盘中心越近得分越高
 *    - 周围已落棋子数越多得分越高
 *
 * 好的着法排序能让 beta 剪枝更早触发，大幅减少搜索节点数。
 */
void Search::orderMoves(std::vector<Move>& moves, const Board& board, int color, const Move& hashMove) {
    // 将置换表的最佳着法放到第一位
    if (hashMove.valid()) {
        auto it = std::find(moves.begin(), moves.end(), hashMove);
        if (it != moves.end()) {
            std::iter_swap(moves.begin(), it);
        }
    }

    // 对其余着法计算位置评分
    int center = Board::SIZE / 2;                       // 棋盘中心 (7, 7)
    std::vector<std::pair<int, Move>> scored;
    for (size_t i = (hashMove.valid() ? 1 : 0); i < moves.size(); ++i) {
        const Move& m = moves[i];
        int score = 0;
        // 距离中心越近，评分越高
        int dist = std::abs(m.x - center) + std::abs(m.y - center);
        score += (Board::SIZE - dist) * 10;
        // 邻近已有棋子数越多，评分越高
        int adjacent = 0;
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy) {
                int nx = m.x + dx, ny = m.y + dy;
                if (board.inBoard(nx, ny) && board.get(nx, ny) != Board::EMPTY)
                    adjacent++;
            }
        score += adjacent * 20;
        scored.emplace_back(-score, m);                 // 取负以便升序排列（实现降序）
    }

    // 稳定排序
    std::sort(scored.begin(), scored.end(),
        [](const std::pair<int, Move>& a, const std::pair<int, Move>& b) {
            return a.first < b.first;
        });

    // 将排序后的着法放回列表
    for (size_t i = 0; i < scored.size(); ++i) {
        moves[i + (hashMove.valid() ? 1 : 0)] = scored[i].second;
    }
}

// =========================================================================
//  Negamax + Alpha-Beta 核心搜索
// =========================================================================

/**
 * @brief Negamax 搜索（含 Alpha-Beta 剪枝和置换表查询）
 *
 * Negamax 是 Minimax 的变体，利用 max(a, b) = -min(-a, -b) 的性质，
 * 将双方评估统一为一个递归形式：节点值 = max( -child_value )。
 *
 * 剪枝逻辑：
 * - 置换表命中且深度足够 → 直接返回或调整 alpha/beta
 * - alpha >= beta → 发生剪枝，终止当前节点搜索
 *
 * @param alpha  当前搜索窗下界
 * @param beta   当前搜索窗上界
 * @param depth  剩余搜索深度
 * @param board  当前局面
 * @param color  当前走棋方
 * @return 当前局面的评分（从 color 视角）
 */
int Search::negamax(int alpha, int beta, int depth, Board& board, int color) {
    // 外部停止信号
    if (stopFlag && stopFlag->load()) return 0;

    int alphaOrig = alpha;                              // 保存原始 alpha（用于判定节点类型）

    // 置换表查询
    uint64_t key = board.zobrist();
    TTEntry ttEntry;
    if (tt.probe(key, ttEntry) && ttEntry.depth >= depth) {
        if (ttEntry.flag == EXACT) return ttEntry.value;       // 精确值直接返回
        if (ttEntry.flag == LOWER) alpha = std::max(alpha, ttEntry.value);
        if (ttEntry.flag == UPPER) beta  = std::min(beta, ttEntry.value);
        if (alpha >= beta) return ttEntry.value;                // 剪枝发生
    }

    // 终止条件：已有胜者
    int winner = board.checkWinner();
    if (winner != Board::EMPTY) {
        return color * winner * (WIN_SCORE + depth);            // 离根越近获胜分值越高
    }
    // 终止条件：平局
    if (board.isFull()) return 0;
    // 终止条件：达到搜索深度，返回静态评估
    if (depth <= 0) return evaluate(board, color);

    // 生成合法着法（不含禁手，邻近模式）
    auto moves = board.generateLegalMoves(false, true, 2);
    if (moves.empty()) return 0;                        // 无合法着法

    // 对着法排序，提高剪枝效率
    Move hashBest = ttEntry.bestMove;
    orderMoves(moves, board, color, hashBest);

    int best = -INF;
    Move bestMove;

    // 遍历所有着法
    for (const Move& m : moves) {
        board.makeMove(m.x, m.y, color);
        int val = -negamax(-beta, -alpha, depth - 1, board, -color);  // Negamax 递归
        board.undoMove(m.x, m.y);

        if (val > best) {
            best = val;
            bestMove = m;
        }
        alpha = std::max(alpha, val);
        if (alpha >= beta) break;                       // Beta 剪枝：对手有更好的选择
    }

    // 存储到置换表
    // EXACT: 搜索得到了精确值   LOWER: beta 剪枝导致实际值 >= best
    // UPPER: 所有着法都无法超越 alphaOrig
    Flag flag = (best <= alphaOrig) ? UPPER : (best >= beta) ? LOWER : EXACT;
    tt.store(key, {depth, best, flag, bestMove});
    return best;
}

// =========================================================================
//  迭代加深搜索（单线程）
// =========================================================================

/**
 * @brief 迭代加深搜索：从 depth=1 逐层加深至 maxDepth
 *
 * 迭代加深的优势：
 * 1. 可在任意时刻中断并返回当前最佳着法（适合时间受限场景）
 * 2. 浅层搜索结果可用于着法排序，加速深层搜索
 * 3. 置换表在层间复用，提高命中率
 *
 * 搜索前会先检测：
 * - 自己能否一步获胜 → 直接返回
 * - 对手是否有必胜威胁 → 必须堵住
 */
Move Search::iterativeDeepening(const Board& rootBoard) {
    Move bestMove;
    internalStop = false;
    tt.clear();                                         // 每次搜索前清空置换表

    int color = rootBoard.getSide();
    auto moves = rootBoard.generateLegalMoves(false, true, 2);
    if (moves.empty()) return Move(-1, -1);

    // ---------- 立即获胜检测（自己） ----------
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, color);
        if (temp.checkWinner() == color) {
            return m;                                   // 直接获胜，无需搜索
        }
    }

    // ---------- 对手必胜威胁检测（堵住） ----------
    int oppColor = -color;
    std::vector<Move> threats;
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, oppColor);              // 假想对手下在这个位置
        if (temp.checkWinner() == oppColor) {
            threats.push_back(m);
        }
    }
    if (!threats.empty()) {
        return threats.front();                         // 必须堵住对手的获胜威胁
    }

    // ---------- 正常迭代加深 ----------
    for (int d = 1; d <= maxDepth; ++d) {
        if (stopFlag && stopFlag->load()) break;        // 外部中断
        Board board = rootBoard;
        int bestVal = -INF;
        Move currentBest;

        for (const Move& m : moves) {
            board.makeMove(m.x, m.y, color);
            int val = -negamax(-INF, INF, d - 1, board, -color);
            board.undoMove(m.x, m.y);
            if (val > bestVal) {
                bestVal = val;
                currentBest = m;
            }
        }
        bestMove = currentBest;                         // 每一层都有结果，随时可中断
    }
    return bestMove;
}

// =========================================================================
//  多线程并行根搜索
// =========================================================================

/**
 * @brief 多线程并行根搜索
 *
 * 将根节点的合法着法分成若干块，每块交给一个线程独立搜索。
 * 各线程共享相同的停止标志，最快 60 秒超时强制停止。
 *
 * 搜索前同样进行立即获胜和对手威胁检测。
 *
 * @note 若线程数 <= 1 或着法数 < 2，回退到单线程迭代加深。
 */
Move Search::parallelRootSearch(const Board& rootBoard) {
    auto moves = rootBoard.generateLegalMoves(false, true, 2);
    if (moves.empty()) return Move(-1, -1);

    int color = rootBoard.getSide();

    // 1. 自己能否一步获胜？
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, color);
        if (temp.checkWinner() == color) {
            return m;                                   // 直接赢
        }
    }

    // 2. 对手是否有必胜威胁？
    int oppColor = -color;
    std::vector<Move> threats;
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, oppColor);
        if (temp.checkWinner() == oppColor) {
            threats.push_back(m);
        }
    }
    if (!threats.empty()) {
        return threats.front();                         // 必须堵
    }

    // 3. 线程数不足或着法太少时，回退到迭代加深
    if (numThreads <= 1 || moves.size() < 2) {
        return iterativeDeepening(rootBoard);
    }

    // 创建局部停止标志，各线程共享
    auto localStop = std::make_shared<std::atomic<bool>>(false);
    setStopFlag(localStop);

    // 按线程数划分着法块
    std::vector<std::future<std::pair<Move, int>>> futures;
    size_t chunkSize = (moves.size() + numThreads - 1) / numThreads;   // 向上取整
    for (size_t start = 0; start < moves.size(); start += chunkSize) {
        size_t end = std::min(start + chunkSize, moves.size());
        std::vector<Move> subMoves(moves.begin() + start, moves.begin() + end);
        futures.push_back(std::async(std::launch::async, [this, rootBoard, subMoves, localStop, color]() {
            int bestVal = -INF;
            Move bestMove;
            Board board = rootBoard;
            for (const Move& m : subMoves) {
                if (localStop->load()) break;           // 收到停止信号
                board.makeMove(m.x, m.y, color);
                int val = -negamax(-INF, INF, maxDepth - 1, board, -color);
                board.undoMove(m.x, m.y);
                if (val > bestVal) {
                    bestVal = val;
                    bestMove = m;
                }
            }
            return std::make_pair(bestMove, bestVal);
        }));
    }

    // 监控线程：60 秒超时后强制设置停止标志
    std::thread monitor([localStop]() {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        *localStop = true;
    });
    monitor.detach();

    // 收集各线程结果，取最优着法
    int globalBestVal = -INF;
    Move globalBestMove;
    for (auto& f : futures) {
        auto [m, val] = f.get();
        if (val > globalBestVal) {
            globalBestVal = val;
            globalBestMove = m;
        }
    }
    *localStop = true;                                  // 通知所有线程结束
    return globalBestMove;
}

// =========================================================================
//  公共接口
// =========================================================================

Move Search::getBestMove(const Board& board) {
    if (numThreads > 1) {
        return parallelRootSearch(board);               // 多线程模式
    } else {
        return iterativeDeepening(board);               // 单线程模式
    }
}
