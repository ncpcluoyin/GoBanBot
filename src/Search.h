#ifndef SEARCH_H
#define SEARCH_H

#include "Board.h"
#include "TransTable.h"
#include <atomic>
#include <vector>
#include <thread>
#include <future>
#include <memory>

class Search {
public:
    Search();
    void setDepth(int d) { maxDepth = d; }
    void setThreads(int n) { numThreads = n; }
    void setStopFlag(std::shared_ptr<std::atomic<bool>> flag) { stopFlag = flag; }

    Move getBestMove(const Board& board);

private:
    int maxDepth;
    int numThreads;
    TranspositionTable tt;
    std::shared_ptr<std::atomic<bool>> stopFlag;
    std::atomic<bool> internalStop;

    static const int INF = 1000000;
    static const int WIN_SCORE = 100000;

    int negamax(int alpha, int beta, int depth, Board& board, int color);
    int evaluate(const Board& board, int color) const;
    void orderMoves(std::vector<Move>& moves, const Board& board, int color, const Move& hashMove);

    Move iterativeDeepening(const Board& rootBoard);
    Move parallelRootSearch(const Board& rootBoard);
};

#endif