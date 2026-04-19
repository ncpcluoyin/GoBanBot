#ifndef GAME_H
#define GAME_H

#include "Board.h"
#include "Search.h"
#include <atomic>
#include <memory>

class Game {
public:
    Game();
    void setup();
    void run();
    void waitExit();

private:
    Board board;
    Search aiBlack, aiWhite;
    std::shared_ptr<std::atomic<bool>> stopFlag;

    enum Mode { HUMAN_VS_AI, AI_VS_AI };
    Mode mode;
    bool highlight;
    int humanSide;

    void printBoard() const;
    void humanTurn();
    void aiTurn(Search& ai, const std::string& name);
    void checkGameOver(bool& over);
    void announceResult(int winner) const;
};

#endif