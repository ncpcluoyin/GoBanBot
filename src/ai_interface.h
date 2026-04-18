#ifndef AI_INTERFACE_H
#define AI_INTERFACE_H

#include "board.h"
#include <utility>
#include <chrono>

struct AIResult {
    int x, y;
    double evaluation;      // 胜率或评估分数
    int simulations;        // MCTS模拟次数 / Negamax节点数
    int cacheHits;          // 置换表命中次数（Negamax）
    double timeMs;          // 计算耗时
};

class GomokuAI {
public:
    virtual ~GomokuAI() = default;
    virtual AIResult getMove(const Board& board, int aiPiece, int opponentPiece, int timeLimitMs = 0) = 0;
};

#endif