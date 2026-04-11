#ifndef AI_INTERFACE_H
#define AI_INTERFACE_H

#include "board.h"
#include <utility>

class GomokuAI {
public:
    virtual ~GomokuAI() = default;
    virtual std::pair<int, int> getMove(const Board& board, int aiPiece, int opponentPiece) = 0;
};

#endif // AI_INTERFACE_H