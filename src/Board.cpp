#include "Board.h"
#include <random>
#include <cstring>
#include <unordered_set>

struct MoveHash {
    size_t operator()(const Move& m) const {
        return m.x * 31 + m.y;
    }
};

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

bool Board::isLiveThree(int x, int y, int dx, int dy) const {
    // 在方向 (dx, dy) 上检查 (x, y) 是否参与构成活三（含跳活三）
    for (int start = -4; start <= 0; ++start) {
        int sx = x + dx * start;
        int sy = y + dy * start;
        int endx = sx + dx * 4;
        int endy = sy + dy * 4;
        if (!inBoard(sx, sy) || !inBoard(endx, endy)) continue;

        int blackCount = 0, emptyCount = 0;
        bool hasWhite = false, containsCenter = false;
        for (int step = 0; step <= 4; ++step) {
            int tx = sx + dx * step;
            int ty = sy + dy * step;
            int stone = board[tx][ty];
            if (stone == BLACK) {
                ++blackCount;
                if (tx == x && ty == y) containsCenter = true;
            } else if (stone == EMPTY) {
                ++emptyCount;
            } else {
                hasWhite = true;
                break;
            }
        }
        if (!containsCenter || hasWhite) continue;
        if (blackCount != 3 || emptyCount != 2) continue;

        // 检查窗口外侧两端是否都为空（且在棋盘内）
        int beforeX = sx - dx, beforeY = sy - dy;
        int afterX = endx + dx, afterY = endy + dy;
        if (inBoard(beforeX, beforeY) && board[beforeX][beforeY] == EMPTY &&
            inBoard(afterX, afterY)  && board[afterX][afterY] == EMPTY)
            return true;
    }
    return false;
}

int Board::countFoursAt(int x, int y, int stone) const {
    int fourCount = 0;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (auto& d : dirs) {
        int dx = d[0], dy = d[1];
        // 枚举包含 (x,y) 的所有可能 5 连线段
        for (int start = 0; start < 5; ++start) {
            int sx = x - dx * start;
            int sy = y - dy * start;
            int ex = sx + dx * 4;
            int ey = sy + dy * 4;
            if (!inBoard(sx, sy) || !inBoard(ex, ey)) continue;

            int stoneCount = 0;
            bool hasEmpty = false;
            bool hasOpponent = false;
            for (int step = 0; step < 5; ++step) {
                int tx = sx + dx * step;
                int ty = sy + dy * step;
                int s = board[tx][ty];
                if (s == stone) stoneCount++;
                else if (s == EMPTY) {
                    if (hasEmpty) { hasOpponent = true; break; }
                    hasEmpty = true;
                } else { hasOpponent = true; break; }
            }
            if (!hasOpponent && stoneCount == 4 && hasEmpty) {
                fourCount++;
                break;  // 该方向已找到一个四，不再重复计数
            }
        }
    }
    return fourCount;
}

bool Board::isForbidden(int x, int y) const {
    if (!inBoard(x, y) || board[x][y] != EMPTY) return true;
    if (side != BLACK) return false;

    Board temp = *this;
    temp.board[x][y] = BLACK;

    // 形成五连时不禁手
    if (temp.isFive(x, y, BLACK)) return false;

    // 长连禁手
    if (temp.isOverline(x, y)) return true;

    // 双三检测（含跳活三）
    int threeCount = 0;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (auto& d : dirs) {
        if (temp.isLiveThree(x, y, d[0], d[1])) {
            threeCount++;
        }
    }
    if (threeCount >= 2) return true;

    // 双四检测（包含跳四）
    int fourCount = temp.countFoursAt(x, y, BLACK);
    if (fourCount >= 2) return true;

    return false;
}

std::vector<Move> Board::generateLegalMoves(bool includeForbidden, bool onlyNearby, int radius) const {
    if (!onlyNearby) {
        std::vector<Move> moves;
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j) {
                if (board[i][j] != EMPTY) continue;
                if (!includeForbidden && isForbidden(i, j)) continue;
                moves.emplace_back(i, j);
            }
        return moves;
    }

    // 邻近生成（原逻辑不变）
    std::unordered_set<Move, MoveHash> candidates;
    for (int i = 0; i < SIZE; ++i) {
        for (int j = 0; j < SIZE; ++j) {
            if (board[i][j] == EMPTY) continue;
            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dy = -radius; dy <= radius; ++dy) {
                    int nx = i + dx, ny = j + dy;
                    if (inBoard(nx, ny) && board[nx][ny] == EMPTY)
                        candidates.insert(Move(nx, ny));
                }
            }
        }
    }

    // 若棋盘为空，生成中心附近
    if (candidates.empty()) {
        int center = SIZE / 2;
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy) {
                int nx = center + dx, ny = center + dy;
                if (inBoard(nx, ny)) candidates.insert(Move(nx, ny));
            }
    }

    std::vector<Move> moves;
    for (const Move& m : candidates) {
        if (!includeForbidden && isForbidden(m.x, m.y)) continue;
        moves.push_back(m);
    }

    // 回退：邻近生成为空且未要求禁手时，用全盘生成
    if (moves.empty() && !includeForbidden && onlyNearby) {
        return generateLegalMoves(false, false);
    }

    return moves;
}

// 评估函数实现
namespace {
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
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    bool visited[SIZE][SIZE][4] = {false};

    for (int x = 0; x < SIZE; ++x) {
        for (int y = 0; y < SIZE; ++y) {
            if (board[x][y] != stone) continue;
            for (int d = 0; d < 4; ++d) {
                if (visited[x][y][d]) continue;
                int dx = dirs[d][0], dy = dirs[d][1];

                int count = 1;
                int nx = x + dx, ny = y + dy;
                while (inBoard(nx, ny) && board[nx][ny] == stone) {
                    ++count;
                    nx += dx; ny += dy;
                }
                bool openEnd1 = inBoard(nx, ny) && board[nx][ny] == EMPTY;

                nx = x - dx; ny = y - dy;
                while (inBoard(nx, ny) && board[nx][ny] == stone) {
                    ++count;
                    nx -= dx; ny -= dy;
                }
                bool openEnd2 = inBoard(nx, ny) && board[nx][ny] == EMPTY;

                // 标记已访问
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

                if (count >= 5) continue;

                int openCount = (openEnd1 ? 1 : 0) + (openEnd2 ? 1 : 0);
                if (count == 4) {
                    if (openCount == 2) totalScore += SCORE_OPEN_FOUR;
                    else if (openCount >= 1) totalScore += SCORE_FOUR;
                } else if (count == 3) {
                    if (openCount == 2) totalScore += SCORE_OPEN_THREE;
                    else if (openCount == 1) totalScore += SCORE_THREE;
                } else if (count == 2) {
                    if (openCount == 2) totalScore += SCORE_OPEN_TWO;
                    else if (openCount == 1) totalScore += SCORE_TWO;
                }
            }
        }
    }
    return totalScore;
}

bool Board::hasLegalMoves() const {
    return !generateLegalMoves(false, false).empty(); // 全盘生成，不包含禁手
}