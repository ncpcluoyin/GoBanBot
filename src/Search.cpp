/**
 * @file Search.cpp
 * @brief AI 搜索算法实现
 *
 * 实现基于 Negamax + Alpha-Beta 剪枝的五子棋 AI 搜索。
 * 包含 Killer Move 启发、多线程并行根搜索、Aspiration Window
 * 和迭代加深框架，支持单步最大限时。
 */

#include "Search.h"
#include <algorithm>
#include <cstring>
#include <iostream>

Search::Search() : maxDepth(6), numThreads(1), maxTimeMs(0),
                   nullMoveR(NULL_MOVE_R), stopFlag(nullptr), internalStop(false) {
    std::memset(killerMoves, 0, sizeof(killerMoves));
}

// =========================================================================
//  时间控制
// =========================================================================

bool Search::isTimeUp() const {
    if (maxTimeMs <= 0) return false;
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
    return elapsed >= maxTimeMs;
}

// =========================================================================
//  静态评估
// =========================================================================

int Search::evaluate(const Board& board, int color) const {
    int blackScore, whiteScore;
    board.evaluateBoth(blackScore, whiteScore);
    int score = blackScore - whiteScore;
    return color * score;
}

// =========================================================================
//  着法排序（含 Killer Move 启发）
// =========================================================================

void Search::orderMoves(std::vector<Move>& moves, const Board& board, int color,
                         const Move& hashMove, int depth) {
    // 将置换表的最佳着法放到第一位
    size_t ordered = 0;
    if (hashMove.valid()) {
        auto it = std::find(moves.begin(), moves.end(), hashMove);
        if (it != moves.end()) {
            std::iter_swap(moves.begin(), it);
            ordered = 1;
        }
    }

    // Killer move 1：放到第二位（优先级仅次于置换表着法）
    const Move& k1 = killerMoves[0][depth];
    if (k1.valid() && k1 != hashMove && ordered < moves.size()) {
        auto it = std::find(moves.begin() + ordered, moves.end(), k1);
        if (it != moves.end()) {
            std::iter_swap(moves.begin() + ordered, it);
            ordered++;
        }
    }

    // Killer move 2：放到第三位
    const Move& k2 = killerMoves[1][depth];
    if (k2.valid() && k2 != hashMove && k2 != k1 && ordered < moves.size()) {
        auto it = std::find(moves.begin() + ordered, moves.end(), k2);
        if (it != moves.end()) {
            std::iter_swap(moves.begin() + ordered, it);
            ordered++;
        }
    }

    // 对其余着法计算位置评分
    int center = Board::SIZE / 2;
    std::vector<std::pair<int, Move>> scored;
    scored.reserve(moves.size() - ordered);
    for (size_t i = ordered; i < moves.size(); ++i) {
        const Move& m = moves[i];
        int score = 0;
        int dist = std::abs(m.x - center) + std::abs(m.y - center);
        score += (Board::SIZE - dist) * 10;
        int adjacent = 0;
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy) {
                int nx = m.x + dx, ny = m.y + dy;
                if (board.inBoard(nx, ny) && board.get(nx, ny) != Board::EMPTY)
                    adjacent++;
            }
        score += adjacent * 20;
        scored.emplace_back(-score, m);
    }

    std::sort(scored.begin(), scored.end(),
        [](const std::pair<int, Move>& a, const std::pair<int, Move>& b) {
            return a.first < b.first;
        });

    for (size_t i = 0; i < scored.size(); ++i) {
        moves[ordered + i] = scored[i].second;
    }
}

// =========================================================================
//  Negamax + Alpha-Beta 核心搜索
// =========================================================================

int Search::negamax(int alpha, int beta, int depth, Board& board, int color,
                     uint64_t& nodeCount, bool allowNull) {
    // 定期检查时限（每 1023 个节点检查一次，减少开销）
    if (maxTimeMs > 0 && (++nodeCount & 1023) == 0) {
        if (isTimeUp()) return 0;
    }

    // 外部停止信号
    if (stopFlag && stopFlag->load()) return 0;

    int alphaOrig = alpha;

    // 置换表查询
    uint64_t key = board.zobrist();
    TTEntry ttEntry;
    if (tt.probe(key, ttEntry) && ttEntry.depth >= depth) {
        if (ttEntry.flag == EXACT) return ttEntry.value;
        if (ttEntry.flag == LOWER) alpha = std::max(alpha, ttEntry.value);
        if (ttEntry.flag == UPPER) beta  = std::min(beta, ttEntry.value);
        if (alpha >= beta) return ttEntry.value;
    }

    // 终止条件：已有胜者
    int winner = board.checkWinner();
    if (winner != Board::EMPTY) {
        return color * winner * (WIN_SCORE + depth);
    }
    // 终止条件：平局
    if (board.isFull()) return 0;
    // 终止条件：达到搜索深度，返回静态评估
    if (depth <= 0) return evaluate(board, color);

    // ── 空着剪枝 (Null Move Pruning) ──
    // 条件：启用、允许空着、深度足够、非终局、且我方没有被一步杀的危险
    if (nullMoveR > 0 && allowNull && depth >= nullMoveR + 1
        && beta < WIN_SCORE - depth
        && evaluate(board, color) > -SCORE_OPEN_THREAT) {
        // 动态缩减量：深层搜索使用更激进的缩减
        int R = nullMoveR + (depth >= 7 ? 1 : 0);

        // 模拟空着：不落子，仅切换走棋方
        int origSide = board.getSide();
        board.setSide(-origSide);

        // 以零窗口搜索对手走棋后的局面（缩减深度）
        int nullScore = -negamax(-beta, -beta + 1, depth - 1 - R, board, -color,
                                 nodeCount, false);

        board.setSide(origSide);  // 恢复走棋方

        // 即使让对手多走一步，我方仍 >= beta → 剪枝
        if (nullScore >= beta) {
            return beta;
        }
    }

    // 生成合法着法（不含禁手，邻近模式）
    auto moves = board.generateLegalMoves(false, true, 2);
    if (moves.empty()) return 0;

    // 对着法排序，提高剪枝效率
    Move hashBest = ttEntry.bestMove;
    orderMoves(moves, board, color, hashBest, depth);

    int best = -INF;
    Move bestMove;
    bool firstMove = true;

    for (const Move& m : moves) {
        board.makeMove(m.x, m.y, color);
        int val;
        if (firstMove) {
            // 第一个着法：全窗口搜索
            int score = -negamax(-beta, -alpha, depth - 1, board, -color, nodeCount);
            // 首个着法若仍然触发剪枝，说明该着法极强，记录为 killer
            if (score >= beta) {
                if (killerMoves[0][depth] != m) {
                    killerMoves[1][depth] = killerMoves[0][depth];
                    killerMoves[0][depth] = m;
                }
                history[m.x][m.y] += depth * depth;
            }
            val = score;
            firstMove = false;
        } else {
            // PVS: 零窗口侦察搜索
            val = -negamax(-alpha - 1, -alpha, depth - 1, board, -color, nodeCount);
            if (val > alpha && val < beta) {
                // 侦察搜索触发 fail-high，用收紧窗口重搜
                val = -negamax(-beta, -val, depth - 1, board, -color, nodeCount);
            }
        }
        board.undoMove(m.x, m.y);

        if (val > best) {
            best = val;
            bestMove = m;
        }
        alpha = std::max(alpha, val);
        if (alpha >= beta) {
            // Beta 剪枝：记录引起剪枝的着法为 killer move，更新历史启发
            if (m.valid()) {
                if (killerMoves[0][depth] != m) {
                    killerMoves[1][depth] = killerMoves[0][depth];
                    killerMoves[0][depth] = m;
                }
                history[m.x][m.y] += depth * depth;
            }
            break;
        }
    }

    // 存储到置换表
    Flag flag = (best <= alphaOrig) ? UPPER : (best >= beta) ? LOWER : EXACT;
    tt.store(key, {depth, best, flag, bestMove});
    return best;
}

// =========================================================================
//  迭代加深搜索（单线程）
// =========================================================================

Move Search::iterativeDeepening(const Board& rootBoard) {
    Move bestMove;
    internalStop = false;
    startTime = std::chrono::steady_clock::now();

    int color = rootBoard.getSide();
    auto moves = rootBoard.generateLegalMoves(false, true, 2);
    if (moves.empty()) return Move(-1, -1);

    // ---------- 立即获胜检测（自己） ----------
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, color);
        if (temp.checkWinner() == color) {
            return m;
        }
    }

    // ---------- 对手必胜威胁检测（堵住） ----------
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
        return threats.front();
    }

    // ---------- 正常迭代加深（含 Aspiration Window） ----------
    int prevScore = 0;
    for (int d = 1; d <= maxDepth; ++d) {
        if (isTimeUp()) break;

        Board board = rootBoard;
        uint64_t nodeCount = 0;
        int bestVal = -INF;
        Move currentBest;
        int alpha = -INF, beta = INF;

        // Aspiration Window：第 3 层起用窄窗口
        if (d >= 3) {
            alpha = prevScore - ASP_WINDOW;
            beta  = prevScore + ASP_WINDOW;
        }

        for (const Move& m : moves) {
            if (isTimeUp()) break;
            board.makeMove(m.x, m.y, color);
            int val = -negamax(-beta, -alpha, d - 1, board, -color, nodeCount);
            board.undoMove(m.x, m.y);
            if (val > bestVal) {
                bestVal = val;
                currentBest = m;
            }
            alpha = std::max(alpha, val);
        }

        // 若窄窗口搜索失败，用全窗口重搜
        if (d >= 3 && isTimeUp() == false &&
            (bestVal <= prevScore - ASP_WINDOW || bestVal >= prevScore + ASP_WINDOW)) {
            alpha = -INF; beta = INF;
            bestVal = -INF;
            for (const Move& m : moves) {
                if (isTimeUp()) break;
                board.makeMove(m.x, m.y, color);
                int val = -negamax(-beta, -alpha, d - 1, board, -color, nodeCount);
                board.undoMove(m.x, m.y);
                if (val > bestVal) {
                    bestVal = val;
                    currentBest = m;
                }
                alpha = std::max(alpha, val);
            }
        }

        if (isTimeUp()) break;
        prevScore = bestVal;
        bestMove = currentBest;
    }
    return bestMove;
}

// =========================================================================
//  多线程并行根搜索
// =========================================================================

Move Search::parallelRootSearch(const Board& rootBoard) {
    auto moves = rootBoard.generateLegalMoves(false, true, 2);
    if (moves.empty()) return Move(-1, -1);

    int color = rootBoard.getSide();

    // 1. 自己能否一步获胜？
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, color);
        if (temp.checkWinner() == color) {
            return m;
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
        return threats.front();
    }

    // 3. 线程数不足或着法太少时，回退到迭代加深
    if (numThreads <= 1 || moves.size() < 2) {
        return iterativeDeepening(rootBoard);
    }

    // 创建局部停止标志，各线程共享
    auto localStop = std::make_shared<std::atomic<bool>>(false);
    setStopFlag(localStop);

    // 使用配置的时限或默认 60 秒
    int timeoutMs = (maxTimeMs > 0) ? maxTimeMs : 60000;

    // 按线程数划分着法块
    std::vector<std::future<std::pair<Move, int>>> futures;
    size_t chunkSize = (moves.size() + numThreads - 1) / numThreads;
    for (size_t start = 0; start < moves.size(); start += chunkSize) {
        size_t end = std::min(start + chunkSize, moves.size());
        std::vector<Move> subMoves(moves.begin() + start, moves.begin() + end);
        futures.push_back(std::async(std::launch::async, [this, rootBoard, subMoves, localStop, color]() {
            int bestVal = -INF;
            Move bestMove;
            Board board = rootBoard;
            uint64_t nodeCount = 0;
            for (const Move& m : subMoves) {
                if (localStop->load()) break;
                board.makeMove(m.x, m.y, color);
                int val = -negamax(-INF, INF, maxDepth - 1, board, -color, nodeCount);
                board.undoMove(m.x, m.y);
                if (val > bestVal) {
                    bestVal = val;
                    bestMove = m;
                }
            }
            return std::make_pair(bestMove, bestVal);
        }));
    }

    // 监控线程：超时后设置停止标志
    std::thread monitor([localStop, timeoutMs]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeoutMs));
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
    *localStop = true;
    return globalBestMove;
}

// =========================================================================
//  公共接口
// =========================================================================

Move Search::getBestMove(const Board& board) {
    tt.clear();
    std::memset(killerMoves, 0, sizeof(killerMoves));
    std::memset(history, 0, sizeof(history));
    if (numThreads > 1) {
        return parallelRootSearch(board);
    } else {
        return iterativeDeepening(board);
    }
}
