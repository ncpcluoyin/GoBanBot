#ifndef NEGAMAX_AI_H
#define NEGAMAX_AI_H

#include "ai_interface.h"
#include "board.h"
#include <unordered_map>
#include <future>
#include <vector>

struct TTEntry {
    int depth;
    int value;
    int flag; // 0: exact, 1: lower, 2: upper
    ZobristKey hash;
};

class NegamaxAI : public GomokuAI {
public:
    NegamaxAI(int maxDepth, int numThreads);
    AIResult getMove(const Board& board, int aiPiece, int opponentPiece, int timeLimitMs = 0) override;

private:
    int maxDepth;
    int numThreads;
    std::unordered_map<ZobristKey, TTEntry> transTable;
    int cacheHits; // 用于记录本次搜索命中次数

    int evaluatePosition(const Board& b, int x, int y, int piece) const;
    int getDirectionScore(const Board& b, int x, int y, int dx, int dy, int piece) const;
    std::vector<std::pair<int,int>> getCandidatePositions(const Board& b) const;
    int negamax(const Board& b, int depth, int alpha, int beta, int currentPlayer,
                int aiPiece, int opponentPiece, ZobristKey hash, int& nodes);
    void sortMoves(const Board& b, std::vector<std::pair<int,int>>& moves, int piece) const;
};

#endif