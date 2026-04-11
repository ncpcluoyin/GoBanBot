#ifndef GAME_H
#define GAME_H

#include <memory>          // 必须添加
#include "board.h"
#include "ai_interface.h"

class Game {
public:
    Game(std::unique_ptr<GomokuAI> ai, bool playerFirst);
    void run();

private:
    std::unique_ptr<GomokuAI> ai;
    Board board;
    bool playerTurnFlag;
    int lastPlayerX, lastPlayerY;
    int lastAIX, lastAIY;

    void playerMove();
    void aiMove();
    bool handleGameEnd(int x, int y, int piece);
};

#endif // GAME_H