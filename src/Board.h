#ifndef BOARD_H
#define BOARD_H

#include <cstdint>
#include <vector>
#include <string>

struct Move {
    int x, y;
    Move() : x(-1), y(-1) {}
    Move(int _x, int _y) : x(_x), y(_y) {}
    bool valid() const { return x >= 0 && x < 15 && y >= 0 && y < 15; }
    bool operator==(const Move& other) const { return x == other.x && y == other.y; }
};

class Board {
public:
    static const int SIZE = 15;
    static const int EMPTY = 0;
    static const int BLACK = 1;
    static const int WHITE = -1;
    // 评估辅助：统计指定颜色的棋型得分（不包含连五，连五已在搜索中处理）
    int evaluateColor(int color) const;
    Board();
    Board(const Board& other) = default;  // 显式默认拷贝
    Board& operator=(const Board& other) = default;
    void reset();

    int get(int x, int y) const { return board[x][y]; }
    void set(int x, int y, int stone) { board[x][y] = stone; }
    bool isEmpty(int x, int y) const { return board[x][y] == EMPTY; }
    bool inBoard(int x, int y) const { return x>=0 && x<SIZE && y>=0 && y<SIZE; }

    bool makeMove(int x, int y, int stone);
    void undoMove(int x, int y);

    Move getLastMove() const { return lastMove; }
    void setLastMove(Move m) { lastMove = m; }

    int getSide() const { return side; }
    void setSide(int s) { side = s; }

    int checkWinner() const;
    bool isFull() const;
    bool isForbidden(int x, int y) const;
    std::vector<Move> generateLegalMoves(bool includeForbidden = false) const;

    uint64_t zobrist() const { return hash; }

private:
    int board[SIZE][SIZE];
    Move lastMove;
    int side;
    uint64_t hash;

    static uint64_t zobristTable[SIZE][SIZE][2];
    static bool zobristInitialized;
    static void initZobrist();

    int countDirection(int x, int y, int dx, int dy, int stone) const;
    bool isFive(int x, int y, int stone) const;
    bool isOverline(int x, int y) const;
    int countOpenFour(int x, int y) const;
    int countOpenThree(int x, int y) const;
};

#endif