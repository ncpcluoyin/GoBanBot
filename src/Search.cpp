#include "Search.h"
#include <algorithm>
#include <cstring>
#include <iostream>

Search::Search() : maxDepth(6), numThreads(1), stopFlag(nullptr), internalStop(false) {}

int Search::evaluate(const Board& board, int color) const {
    // color 是当前走棋方，返回该方的优势分值（正分表示对 color 有利）
    int blackScore = board.evaluateColor(Board::BLACK);
    int whiteScore = board.evaluateColor(Board::WHITE);
    int score = blackScore - whiteScore;
    return color * score;   // 若当前为黑方，则返回 black-white；若为白方，则返回 white-black
}

void Search::orderMoves(std::vector<Move>& moves, const Board& board, int color, const Move& hashMove) {
    if (hashMove.valid()) {
        auto it = std::find(moves.begin(), moves.end(), hashMove);
        if (it != moves.end()) std::iter_swap(moves.begin(), it);
    }
    // 可以添加中心偏好排序，省略
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
    if (depth <= 0) return color * evaluate(board, color);

    auto moves = board.generateLegalMoves(false);
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

    for (int d = 1; d <= maxDepth; ++d) {
        if (stopFlag && stopFlag->load()) break;
        Board board = rootBoard;
        int color = board.getSide();
        int bestVal = -INF;
        Move currentBest;

        auto moves = board.generateLegalMoves(false);
        if (moves.empty()) return Move(-1,-1);

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
    auto moves = rootBoard.generateLegalMoves(false);
    if (moves.empty()) return Move(-1,-1);
    if (numThreads <= 1 || moves.size() < 2) {
        return iterativeDeepening(rootBoard);
    }

    auto localStop = std::make_shared<std::atomic<bool>>(false);
    setStopFlag(localStop);  // 使用共享指针，生命周期由调用者管理

    std::vector<std::future<std::pair<Move, int>>> futures;
    size_t chunkSize = (moves.size() + numThreads - 1) / numThreads;
    for (size_t start = 0; start < moves.size(); start += chunkSize) {
        size_t end = std::min(start + chunkSize, moves.size());
        std::vector<Move> subMoves(moves.begin() + start, moves.begin() + end);
        futures.push_back(std::async(std::launch::async, [this, rootBoard, subMoves, localStop]() {
            int bestVal = -INF;
            Move bestMove;
            Board board = rootBoard;
            int color = board.getSide();
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

    // 监控超时60秒
    std::thread monitor([localStop](){
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
    *localStop = true; // 通知其他线程结束
    return globalBestMove;
}

Move Search::getBestMove(const Board& board) {
    if (numThreads > 1) {
        return parallelRootSearch(board);
    } else {
        return iterativeDeepening(board);
    }
}