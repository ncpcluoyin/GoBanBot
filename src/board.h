#ifndef BOARD_H
#define BOARD_H

#include "utils.h"

class Board {
public:
    Board();
    void init();
    bool isFull() const;
    bool checkWin(int x, int y, int piece) const;
    bool isValidMove(int x, int y) const;
    void setPiece(int x, int y, int piece);
    int getPiece(int x, int y) const;
    void print(int lastPlayerX, int lastPlayerY, int lastAIX, int lastAIY) const;

private:
    int grid[BOARD_SIZE][BOARD_SIZE];
};

#endif // BOARD_H