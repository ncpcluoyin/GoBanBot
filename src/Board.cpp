#include "Board.h"
#include <random>
#include <cstring>

uint64_t Board::zobristTable[Board::SIZE][Board::SIZE][2];
bool Board::zobristInitialized = false;

Board::Board() {
    if (!zobristInitialized) {
        initZobrist();
        zobristInitialized = true;
    }
    reset();
}

void Board::initZobrist() {
    std::mt19937_64 rng(12345);
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            for (int k = 0; k < 2; ++k)
                zobristTable[i][j][k] = rng();
}

void Board::reset() {
    std::memset(board, 0, sizeof(board));
    side = BLACK;
    lastMove = Move(-1, -1);
    hash = 0;
}

bool Board::makeMove(int x, int y, int stone) {
    if (!inBoard(x, y) || board[x][y] != EMPTY) return false;
    board[x][y] = stone;
    lastMove = Move(x, y);
    side = -stone;
    int idx = (stone == BLACK) ? 0 : 1;
    hash ^= zobristTable[x][y][idx];
    return true;
}

void Board::undoMove(int x, int y) {
    if (!inBoard(x, y)) return;
    int stone = board[x][y];
    if (stone == EMPTY) return;
    board[x][y] = EMPTY;
    side = stone;
    int idx = (stone == BLACK) ? 0 : 1;
    hash ^= zobristTable[x][y][idx];
}

int Board::checkWinner() const {
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j) {
            int stone = board[i][j];
            if (stone != EMPTY && isFive(i, j, stone))
                return stone;
        }
    return EMPTY;
}

bool Board::isFull() const {
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (board[i][j] == EMPTY) return false;
    return true;
}

int Board::countDirection(int x, int y, int dx, int dy, int stone) const {
    int cnt = 0;
    int nx = x + dx, ny = y + dy;
    while (inBoard(nx, ny) && board[nx][ny] == stone) {
        ++cnt;
        nx += dx;
        ny += dy;
    }
    return cnt;
}

bool Board::isFive(int x, int y, int stone) const {
    if (board[x][y] != stone) return false;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (auto& d : dirs) {
        int cnt = 1 + countDirection(x, y, d[0], d[1], stone)
                    + countDirection(x, y, -d[0], -d[1], stone);
        if (cnt >= 5) return true;
    }
    return false;
}

bool Board::isOverline(int x, int y) const {
    if (board[x][y] != BLACK) return false;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (auto& d : dirs) {
        int cnt = 1 + countDirection(x, y, d[0], d[1], BLACK)
                    + countDirection(x, y, -d[0], -d[1], BLACK);
        if (cnt > 5) return true;
    }
    return false;
}

bool Board::isForbidden(int x, int y) const {
    if (!inBoard(x, y) || board[x][y] != EMPTY) return true;
    if (side != BLACK) return false;

    Board temp = *this;
    temp.board[x][y] = BLACK;

    if (temp.isOverline(x, y)) return true;

    // 简化双三双四检测（保持与之前一致）
    int threeCount = 0, fourCount = 0;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (auto& d : dirs) {
        int cnt1 = temp.countDirection(x, y, d[0], d[1], BLACK);
        int cnt2 = temp.countDirection(x, y, -d[0], -d[1], BLACK);
        int total = 1 + cnt1 + cnt2;

        int nx1 = x + d[0] * (cnt1 + 1);
        int ny1 = y + d[1] * (cnt1 + 1);
        int nx2 = x - d[0] * (cnt2 + 1);
        int ny2 = y - d[1] * (cnt2 + 1);
        bool open1 = temp.inBoard(nx1, ny1) && temp.board[nx1][ny1] == EMPTY;
        bool open2 = temp.inBoard(nx2, ny2) && temp.board[nx2][ny2] == EMPTY;

        if (total == 4 && open1 && open2) fourCount++;
        else if (total == 4 && (open1 || open2)) fourCount++;
        if (total == 3 && open1 && open2) threeCount++;
    }
    return (fourCount >= 2 || threeCount >= 2);
}

std::vector<Move> Board::generateLegalMoves(bool includeForbidden) const {
    std::vector<Move> moves;
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j) {
            if (board[i][j] != EMPTY) continue;
            if (!includeForbidden && isForbidden(i, j)) continue;
            moves.emplace_back(i, j);
        }
    return moves;
}

// 棋型分值常量
namespace {
    const int SCORE_FIVE      = 1000000;  // 实际在搜索中捕获，这里不用
    const int SCORE_OPEN_FOUR = 100000;
    const int SCORE_FOUR      = 10000;
    const int SCORE_OPEN_THREE= 5000;
    const int SCORE_THREE     = 800;
    const int SCORE_OPEN_TWO  = 500;
    const int SCORE_TWO       = 50;
}

int Board::evaluateColor(int stone) const {
    if (stone == EMPTY) return 0;
    int totalScore = 0;

    // 四个方向向量
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};

    // 为避免重复计算同一连线，使用一个标记数组
    bool visited[SIZE][SIZE][4] = {false};

    for (int x = 0; x < SIZE; ++x) {
        for (int y = 0; y < SIZE; ++y) {
            if (board[x][y] != stone) continue;

            for (int d = 0; d < 4; ++d) {
                if (visited[x][y][d]) continue;

                int dx = dirs[d][0], dy = dirs[d][1];

                // 正向计数连续同色棋子
                int count = 1;
                int nx = x + dx, ny = y + dy;
                while (inBoard(nx, ny) && board[nx][ny] == stone) {
                    ++count;
                    nx += dx;
                    ny += dy;
                }
                bool openEnd1 = inBoard(nx, ny) && board[nx][ny] == EMPTY;

                // 反向计数
                nx = x - dx; ny = y - dy;
                while (inBoard(nx, ny) && board[nx][ny] == stone) {
                    ++count;
                    nx -= dx;
                    ny -= dy;
                }
                bool openEnd2 = inBoard(nx, ny) && board[nx][ny] == EMPTY;

                // 标记该线上所有同色棋子已访问
                int tx = x, ty = y;
                while (inBoard(tx, ty) && board[tx][ty] == stone) {
                    visited[tx][ty][d] = true;
                    tx += dx; ty += dy;
                }
                tx = x - dx; ty = y - dy;
                while (inBoard(tx, ty) && board[tx][ty] == stone) {
                    visited[tx][ty][d] = true;
                    tx -= dx; ty -= dy;
                }

                // 根据连子数和开放情况评分
                if (count >= 5) {
                    // 连五已在胜负判定中处理，这里不重复加分
                    continue;
                }

                bool leftOpen = openEnd1;
                bool rightOpen = openEnd2;
                int openCount = (leftOpen ? 1 : 0) + (rightOpen ? 1 : 0);

                if (count == 4) {
                    if (openCount == 2) totalScore += SCORE_OPEN_FOUR;
                    else if (openCount >= 1) totalScore += SCORE_FOUR;
                } else if (count == 3) {
                    if (openCount == 2) totalScore += SCORE_OPEN_THREE;
                    else if (openCount == 1) totalScore += SCORE_THREE;
                } else if (count == 2) {
                    if (openCount == 2) totalScore += SCORE_OPEN_TWO;
                    else if (openCount == 1) totalScore += SCORE_TWO;
                } else if (count == 1) {
                    // 单子不单独给分，避免噪声
                }
                // 对于更复杂棋型（如跳活三、跳冲四）可在此扩展，当前版本已具备基本强度
            }
        }
    }
    return totalScore;
}