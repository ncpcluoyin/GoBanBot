#ifndef GAME_H
#define GAME_H

#include "board.h"
#include "ai_interface.h"
#include <memory>
#include <vector>

struct MoveRecord {
    int turn; // 0: player/AI1, 1: AI2
    int x, y;
    double eval;
    int sims;
    int cacheHits;
    double timeMs;
};

class Game {
public:
    Game(std::unique_ptr<GomokuAI> ai1, std::unique_ptr<GomokuAI> ai2, bool humanFirst, bool highlight);
    void run(); // 人机
    void runBattle(int numGames, bool savePGN, bool printBoardEachMove); // 增加第三个参数

private:
    std::unique_ptr<GomokuAI> playerAI;
    std::unique_ptr<GomokuAI> opponentAI;
    Board board;
    bool humanTurn;
    bool highlightLast;
    int lastPlayerX, lastPlayerY;
    int lastAIX, lastAIY;

    bool handleGameEnd(int x, int y, int piece, const std::string& winner);
    void playerMove();
    void aiMove(std::unique_ptr<GomokuAI>& ai, int piece, const std::string& name,
                int& lastX, int& lastY, bool isFirst);
    void printStatus(const std::string& info);
    void writePGN(const std::vector<MoveRecord>& moves, const std::string& filename) const;
};

#endif