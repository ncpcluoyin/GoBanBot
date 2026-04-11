#include "board.h"
#include <iostream>
using namespace std;

Board::Board() { init(); }

void Board::init() {
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            grid[i][j] = EMPTY;
}

bool Board::isFull() const {
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            if (grid[i][j] == EMPTY) return false;
    return true;
}

bool Board::checkWin(int x, int y, int piece) const {
    const int dx[4] = {1, 0, 1, -1};
    const int dy[4] = {0, 1, 1, 1};
    for (int dir = 0; dir < 4; ++dir) {
        int count = 1;
        for (int step = 1; step <= 5; ++step) {
            int nx = x + dx[dir] * step, ny = y + dy[dir] * step;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (grid[nx][ny] == piece) ++count;
            else break;
        }
        for (int step = 1; step <= 5; ++step) {
            int nx = x - dx[dir] * step, ny = y - dy[dir] * step;
            if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
            if (grid[nx][ny] == piece) ++count;
            else break;
        }
        if (count >= 5) return true;
    }
    return false;
}

bool Board::isValidMove(int x, int y) const {
    return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE && grid[x][y] == EMPTY;
}

void Board::setPiece(int x, int y, int piece) { grid[x][y] = piece; }

int Board::getPiece(int x, int y) const { return grid[x][y]; }

void Board::print(int lastPlayerX, int lastPlayerY, int lastAIX, int lastAIY) const {
    // 列标号：开头5个空格，每个数字后跟一个空格
    cout << "\n     ";  // 5个空格
    for (int j = 0; j < BOARD_SIZE; ++j) {
        int num = (j + 1) % 10;
        cout << num << " ";
    }
    cout << "\n";

    for (int i = 0; i < BOARD_SIZE; ++i) {
        // 行序号：2个前导空格
        cout << "  ";
        if (i + 1 < 10) cout << " " << (i + 1);
        else cout << (i + 1);
        cout << " ";   // 行号后的空格

        for (int j = 0; j < BOARD_SIZE; ++j) {
            char c;
            if (grid[i][j] == EMPTY) c = '.';
            else if (grid[i][j] == PLAYER) c = 'X';
            else c = 'O';

            if (i == lastAIX && j == lastAIY && grid[i][j] == AI_PIECE)
                cout << "\033[31m" << c << "\033[0m ";
            else if (i == lastPlayerX && j == lastPlayerY && grid[i][j] == PLAYER)
                cout << "\033[32m" << c << "\033[0m ";
            else
                cout << c << " ";
        }
        cout << "\n";
    }
    cout << endl;
}