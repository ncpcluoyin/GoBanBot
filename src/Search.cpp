#include "Search.h"
#include <algorithm>
#include <cstring>
#include <iostream>

Search::Search() : maxDepth(6), numThreads(1), stopFlag(nullptr), internalStop(false) {}

int Search::evaluate(const Board& board, int color) const {
    int blackScore = board.evaluateColor(Board::BLACK);
    int whiteScore = board.evaluateColor(Board::WHITE);
    int score = blackScore - whiteScore;
    return color * score;
}

void Search::orderMoves(std::vector<Move>& moves, const Board& board, int color, const Move& hashMove) {
    if (hashMove.valid()) {
        auto it = std::find(moves.begin(), moves.end(), hashMove);
        if (it != moves.end()) {
            std::iter_swap(moves.begin(), it);
        }
    }

    int center = Board::SIZE / 2;
    std::vector<std::pair<int, Move>> scored;
    for (size_t i = (hashMove.valid() ? 1 : 0); i < moves.size(); ++i) {
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
        moves[i + (hashMove.valid() ? 1 : 0)] = scored[i].second;
    }
}

int Search::negamax(int alpha, int beta, int depth, Board& board, int color) {
    if (stopFlag && stopFlag->load()) return 0;

    int alphaOrig = alpha;
    uint64_t key = board.zobrist();
    TTEntry ttEntry;
    if (tt.probe(key, ttEntry) && ttEntry.depth >= depth) {
        if (ttEntry.flag == EXACT) return ttEntry.value;
        if (ttEntry.flag == LOWER) alpha = std::max(alpha, ttEntry.value);
        if (ttEntry.flag == UPPER) beta  = std::min(beta, ttEntry.value);
        if (alpha >= beta) return ttEntry.value;
    }

    int winner = board.checkWinner();
    if (winner != Board::EMPTY) {
        return color * winner * (WIN_SCORE + depth);
    }
    if (board.isFull()) return 0;
    if (depth <= 0) return evaluate(board, color);

    auto moves = board.generateLegalMoves(false, true, 2);
    if (moves.empty()) return 0;

    Move hashBest = ttEntry.bestMove;
    orderMoves(moves, board, color, hashBest);

    int best = -INF;
    Move bestMove;

    for (const Move& m : moves) {
        board.makeMove(m.x, m.y, color);
        int val = -negamax(-beta, -alpha, depth - 1, board, -color);
        board.undoMove(m.x, m.y);

        if (val > best) {
            best = val;
            bestMove = m;
        }
        alpha = std::max(alpha, val);
        if (alpha >= beta) break;
    }

    Flag flag = (best <= alphaOrig) ? UPPER : (best >= beta) ? LOWER : EXACT;
    tt.store(key, {depth, best, flag, bestMove});
    return best;
}

Move Search::iterativeDeepening(const Board& rootBoard) {
    Move bestMove;
    internalStop = false;
    tt.clear();

    int color = rootBoard.getSide();
    auto moves = rootBoard.generateLegalMoves(false, true, 2);
    if (moves.empty()) return Move(-1, -1);

    // ---------- 立即获胜（自己） ----------
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, color);
        if (temp.checkWinner() == color) {
            return m;   // 自己直接赢
        }
    }

    // ---------- 对手立即获胜威胁（堵住） ----------
    int oppColor = -color;
    std::vector<Move> threats;   // 对手的必胜点
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, oppColor);   // 假想对手下这里
        if (temp.checkWinner() == oppColor) {
            threats.push_back(m);             // 这个点必须堵
        }
    }
    if (!threats.empty()) {
        // 若存在多个威胁点，通常只需堵一个（普通五子棋一般一个活四）
        return threats.front();   // 直接返回第一个威胁点
    }
    // ----------------------------------------

    // 正常迭代加深搜索
    for (int d = 1; d <= maxDepth; ++d) {
        if (stopFlag && stopFlag->load()) break;
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
        bestMove = currentBest;
    }
    return bestMove;
}

Move Search::parallelRootSearch(const Board& rootBoard) {
    auto moves = rootBoard.generateLegalMoves(false, true, 2);
    if (moves.empty()) return Move(-1, -1);

    int color = rootBoard.getSide();

    // 1. 自己能否一步获胜？
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, color);
        if (temp.checkWinner() == color) {
            return m;   // 直接赢，不再搜索
        }
    }

    // 2. 对手是否已经形成四连威胁（下一步必赢）？
    int oppColor = -color;
    std::vector<Move> threats;
    for (const Move& m : moves) {
        Board temp = rootBoard;
        temp.makeMove(m.x, m.y, oppColor);   // 假想对手下这里
        if (temp.checkWinner() == oppColor) {
            threats.push_back(m);             // 必须堵的点
        }
    }
    if (!threats.empty()) {
        return threats.front();               // 直接堵住威胁点
    }

    // 3. 无立即胜负，进行正常多线程搜索
    if (numThreads <= 1 || moves.size() < 2) {
        return iterativeDeepening(rootBoard); // 线程数不足时回退到单线程迭代加深
    }

    auto localStop = std::make_shared<std::atomic<bool>>(false);
    setStopFlag(localStop);

    std::vector<std::future<std::pair<Move, int>>> futures;
    size_t chunkSize = (moves.size() + numThreads - 1) / numThreads;
    for (size_t start = 0; start < moves.size(); start += chunkSize) {
        size_t end = std::min(start + chunkSize, moves.size());
        std::vector<Move> subMoves(moves.begin() + start, moves.begin() + end);
        futures.push_back(std::async(std::launch::async, [this, rootBoard, subMoves, localStop, color]() {
            int bestVal = -INF;
            Move bestMove;
            Board board = rootBoard;
            for (const Move& m : subMoves) {
                if (localStop->load()) break;
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

    // 监控线程：60 秒超时强制停止
    std::thread monitor([localStop]() {
        std::this_thread::sleep_for(std::chrono::seconds(60));
        *localStop = true;
    });
    monitor.detach();

    int globalBestVal = -INF;
    Move globalBestMove;
    for (auto& f : futures) {
        auto [m, val] = f.get();
        if (val > globalBestVal) {
            globalBestVal = val;
            globalBestMove = m;
        }
    }
    *localStop = true;   // 通知所有线程结束
    return globalBestMove;
}

Move Search::getBestMove(const Board& board) {
    if (numThreads > 1) {
        return parallelRootSearch(board);
    } else {
        return iterativeDeepening(board);
    }
}