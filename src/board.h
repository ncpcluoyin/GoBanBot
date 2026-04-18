#ifndef BOARD_H
#define BOARD_H

#include "utils.h"
#include <string>
#include <vector>

class Board {
public:
    Board();
    void init();
    bool isFull() const;
    bool checkWin(int x, int y, int piece) const;
    bool isValidMove(int x, int y, int piece);          // 可变 (临时修改)
    int getForbiddenType(int x, int y, int piece);      // 可变
    void setPiece(int x, int y, int piece);
    int getPiece(int x, int y) const;
    void print(int lastPlayerX, int lastPlayerY, int lastAIX, int lastAIY,
               const std::string& rightPanel = "") const;
    ZobristKey getZobristHash() const { return zobristHash; }
    void updateZobrist(int x, int y, int oldPiece, int newPiece);

private:
    int grid[BOARD_SIZE][BOARD_SIZE];
    ZobristKey zobristHash;
    static ZobristKey zobristTable[BOARD_SIZE][BOARD_SIZE][3];
    static bool zobristInit;
    static void initZobrist();

    bool isOverline(int x, int y, int piece);      // 可变
    bool isThreeThree(int x, int y, int piece);    // 可变
    bool isFourFour(int x, int y, int piece);      // 可变
    int countDirection(int x, int y, int dx, int dy, int piece) const;
};

#endif