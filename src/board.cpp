#include "board.h"
#include <iostream>
#include <random>
#include <cstring>

using namespace std;

ZobristKey Board::zobristTable[BOARD_SIZE][BOARD_SIZE][3];
bool Board::zobristInit = false;

Board::Board() {
    if (!zobristInit) initZobrist();
    init();
}

void Board::initZobrist() {
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<ZobristKey> dist;
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            for (int k = 0; k < 3; ++k)
                zobristTable[i][j][k] = dist(gen);
    zobristInit = true;
}

void Board::init() {
    memset(grid, 0, sizeof(grid));
    zobristHash = 0;
}

bool Board::isFull() const {
    for (int i = 0; i < BOARD_SIZE; ++i)
        for (int j = 0; j < BOARD_SIZE; ++j)
            if (grid[i][j] == EMPTY) return false;
    return true;
}

int Board::countDirection(int x, int y, int dx, int dy, int piece) const {
    int cnt = 1;
    for (int step = 1; step <= 5; ++step) {
        int nx = x + dx * step, ny = y + dy * step;
        if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
        if (grid[nx][ny] == piece) cnt++;
        else break;
    }
    for (int step = 1; step <= 5; ++step) {
        int nx = x - dx * step, ny = y - dy * step;
        if (nx < 0 || nx >= BOARD_SIZE || ny < 0 || ny >= BOARD_SIZE) break;
        if (grid[nx][ny] == piece) cnt++;
        else break;
    }
    return cnt;
}

bool Board::checkWin(int x, int y, int piece) const {
    const int dx[4] = {1, 0, 1, -1};
    const int dy[4] = {0, 1, 1, 1};
    for (int dir = 0; dir < 4; ++dir) {
        int cnt = countDirection(x, y, dx[dir], dy[dir], piece);
        if (cnt >= 5) return true;
    }
    return false;
}

bool Board::isOverline(int x, int y, int piece) {
    const int dx[4] = {1, 0, 1, -1};
    const int dy[4] = {0, 1, 1, 1};
    for (int dir = 0; dir < 4; ++dir) {
        int cnt = countDirection(x, y, dx[dir], dy[dir], piece);
        if (cnt > 5) return true;
    }
    return false;
}

bool Board::isThreeThree(int x, int y, int piece) {
    if (piece != PLAYER) return false;
    int old = grid[x][y];
    grid[x][y] = piece;
    int threeCnt = 0;
    const int dx[4] = {1, 0, 1, -1};
    const int dy[4] = {0, 1, 1, 1};
    for (int dir = 0; dir < 4; ++dir) {
        int cnt = countDirection(x, y, dx[dir], dy[dir], piece);
        if (cnt == 3) threeCnt++;
    }
    grid[x][y] = old;
    return threeCnt >= 2;
}

bool Board::isFourFour(int x, int y, int piece) {
    if (piece != PLAYER) return false;
    int old = grid[x][y];
    grid[x][y] = piece;
    int fourCnt = 0;
    const int dx[4] = {1, 0, 1, -1};
    const int dy[4] = {0, 1, 1, 1};
    for (int dir = 0; dir < 4; ++dir) {
        int cnt = countDirection(x, y, dx[dir], dy[dir], piece);
        if (cnt == 4) fourCnt++;
    }
    grid[x][y] = old;
    return fourCnt >= 2;
}

int Board::getForbiddenType(int x, int y, int piece) {
    if (piece != PLAYER) return NO_FORBIDDEN;
    if (grid[x][y] != EMPTY) return NO_FORBIDDEN;
    int type = NO_FORBIDDEN;
    if (isOverline(x, y, piece)) type |= FORBIDDEN_OVERLINE;
    if (isThreeThree(x, y, piece)) type |= FORBIDDEN_THREE_THREE;
    if (isFourFour(x, y, piece)) type |= FORBIDDEN_FOUR_FOUR;
    return type;
}

bool Board::isValidMove(int x, int y, int piece) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return false;
    if (grid[x][y] != EMPTY) return false;
    return getForbiddenType(x, y, piece) == NO_FORBIDDEN;
}

void Board::setPiece(int x, int y, int piece) {
    int old = grid[x][y];
    grid[x][y] = piece;
    updateZobrist(x, y, old, piece);
}

int Board::getPiece(int x, int y) const {
    return grid[x][y];
}

void Board::updateZobrist(int x, int y, int oldPiece, int newPiece) {
    if (oldPiece != EMPTY)
        zobristHash ^= zobristTable[x][y][oldPiece];
    if (newPiece != EMPTY)
        zobristHash ^= zobristTable[x][y][newPiece];
}

void Board::print(int lastPlayerX, int lastPlayerY, int lastAIX, int lastAIY,
                  const string& rightPanel) const {
    cout << "\n     ";
    for (int j = 0; j < BOARD_SIZE; ++j) {
        cout << (j + 1) % 10 << " ";
    }
    cout << "\n";

    for (int i = 0; i < BOARD_SIZE; ++i) {
        cout << "  ";
        if (i + 1 < 10) cout << " " << i + 1;
        else cout << i + 1;
        cout << " ";
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
        if (i == 0 && !rightPanel.empty()) {
            cout << "   " << rightPanel;
        }
        cout << "\n";
    }
    cout << endl;
}