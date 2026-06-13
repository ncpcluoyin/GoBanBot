/**
 * @file Board.cpp
 * @brief 五子棋棋盘功能实现
 *
 * 包含走子/悔棋、Zobrist 哈希、禁手检测（长连、双三、双四）、
 * 合法着法生成、局面评估等核心逻辑。
 */

#include "Board.h"
#include <random>
#include <cstring>
#include <unordered_set>

/** @brief 为 unordered_set<Move> 提供哈希支持 */
struct MoveHash {
    size_t operator()(const Move& m) const {
        return m.x * 31 + m.y;
    }
};

// 静态成员定义
uint64_t Board::zobristTable[Board::SIZE][Board::SIZE][2];
uint64_t Board::zobristSideKey = 0;
bool Board::zobristInitialized = false;

// =========================================================================
//  构造与初始化
// =========================================================================

Board::Board() {
    if (!zobristInitialized) {
        initZobrist();              // 首次构造时初始化全局随机数表
        zobristInitialized = true;
    }
    reset();
}

/**
 * @brief 初始化 Zobrist 随机数表
 *
 * 使用固定种子的 mt19937_64 生成伪随机数，确保每次运行
 * 哈希值一致。表结构为 zobristTable[x][y][colorIndex]，
 * 其中 colorIndex=0 表示黑子，colorIndex=1 表示白子。
 */
void Board::initZobrist() {
    std::mt19937_64 rng(12345);     // 固定种子，保证哈希一致性
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            for (int k = 0; k < 2; ++k)
                zobristTable[i][j][k] = rng();
    zobristSideKey = rng();
}

void Board::reset() {
    std::memset(board, 0, sizeof(board));
    side = BLACK;
    lastMove = Move(-1, -1);
    hash = zobristSideKey;
    cachedBlackScore = 0;
    cachedWhiteScore = 0;
}

// =========================================================================
//  着法操作
// =========================================================================

bool Board::makeMove(int x, int y, int stone) {
    if (!inBoard(x, y) || board[x][y] != EMPTY) return false;

    // 增量计算评分变化（落子前，board[x][y] == EMPTY）
    int deltaBlack, deltaWhite;
    computeDelta(x, y, stone, deltaBlack, deltaWhite);

    board[x][y] = stone;
    lastMove = Move(x, y);
    side = -stone;                          // 切换走棋方
    int idx = (stone == BLACK) ? 0 : 1;
    hash ^= zobristTable[x][y][idx]         // 异或棋子
         ^ zobristSideKey;                  // 异或走棋方切换

    cachedBlackScore += deltaBlack;
    cachedWhiteScore += deltaWhite;
    return true;
}

void Board::undoMove(int x, int y) {
    if (!inBoard(x, y)) return;
    int stone = board[x][y];
    if (stone == EMPTY) return;
    board[x][y] = EMPTY;
    side = stone;                           // 恢复走棋方
    lastMove = Move(-1, -1);                // 撤销后上次着法失效
    int idx = (stone == BLACK) ? 0 : 1;
    hash ^= zobristTable[x][y][idx]         // 撤销棋子
         ^ zobristSideKey;                  // 撤销走棋方切换

    // 增量撤销评分变化（撤子后 board[x][y] == EMPTY，与落子前的状态一致）
    int deltaBlack, deltaWhite;
    computeDelta(x, y, stone, deltaBlack, deltaWhite);
    cachedBlackScore -= deltaBlack;
    cachedWhiteScore -= deltaWhite;
}

// =========================================================================
//  胜负与状态判定
// =========================================================================

int Board::checkWinner() const {
    if (!lastMove.valid()) return EMPTY;
    int stone = board[lastMove.x][lastMove.y];
    if (stone != EMPTY && isFive(lastMove.x, lastMove.y, stone))
        return stone;
    return EMPTY;
}

bool Board::isFull() const {
    for (int i = 0; i < SIZE; ++i)
        for (int j = 0; j < SIZE; ++j)
            if (board[i][j] == EMPTY) return false;
    return true;
}

// =========================================================================
//  方向计数与连子检测
// =========================================================================

/**
 * @brief 从 (x,y) 出发沿方向 (dx,dy) 统计连续同色棋子数
 *
 * 统计结果不包含起点本身，只计相邻同色棋子。
 */
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

/**
 * @brief 检查 (x,y) 处是否形成五连（4 个方向遍历）
 */
bool Board::isFive(int x, int y, int stone) const {
    if (board[x][y] != stone) return false;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};  // 水平 垂直 对角线 反对角线
    for (auto& d : dirs) {
        int cnt = 1 + countDirection(x, y, d[0], d[1], stone)
                    + countDirection(x, y, -d[0], -d[1], stone);  // 双向计数
        if (cnt >= 5) return true;
    }
    return false;
}

/**
 * @brief 检查黑棋在 (x,y) 是否形成长连（>5 子）
 *
 * 长连是黑方禁手之一，仅对黑棋检测。
 */
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

// =========================================================================
//  禁手检测
// =========================================================================

/**
 * @brief 黑棋活三检测（含跳活三）
 *
 * 在方向 (dx, dy) 上，以 (x,y) 为中心遍历 5 子窗口，
 * 判断是否构成活三：窗口内恰好 3 黑子 + 2 空位，无白子，
 * 且窗口外侧两端均为空（真正两活端）。
 */
bool Board::isLiveThree(int x, int y, int dx, int dy) const {
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

        // 窗口外侧两端都必须为空（且在棋盘内）才构成真正的活三
        int beforeX = sx - dx, beforeY = sy - dy;
        int afterX = endx + dx, afterY = endy + dy;
        if (inBoard(beforeX, beforeY) && board[beforeX][beforeY] == EMPTY &&
            inBoard(afterX, afterY)  && board[afterX][afterY] == EMPTY)
            return true;
    }
    return false;
}

/**
 * @brief 统计 (x,y) 处某方棋子的冲四/活四数量
 *
 * 遍历四个方向，对每个方向上所有包含 (x,y) 的 5 连线段，
 * 判断是否由 4 颗同色子 + 1 空位组成（含跳四模式）。
 * 每个方向最多计 1 个四，避免重复计数。
 */
int Board::countFoursAt(int x, int y, int stone) const {
    int fourCount = 0;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    for (auto& d : dirs) {
        int dx = d[0], dy = d[1];
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
                    if (hasEmpty) { hasOpponent = true; break; }  // 多于1个空位 → 不是四
                    hasEmpty = true;
                } else { hasOpponent = true; break; }  // 有对手棋 → 不是四
            }
            if (!hasOpponent && stoneCount == 4 && hasEmpty) {
                fourCount++;
                break;  // 该方向已找到一个四，跳过该方向其余窗口
            }
        }
    }
    return fourCount;
}

/**
 * @brief 禁手综合检测
 *
 * 检测规则：
 * 1. 如果该着能形成五连，则不禁手（五连优先）
 * 2. 长连禁手：>5 子连珠
 * 3. 双三禁手：两个方向的活三同时存在（含跳活三）
 * 4. 双四禁手：两个方向的冲四/活四同时存在（含跳四）
 *
 * 注意：禁手仅对黑方生效。
 */
bool Board::isForbidden(int x, int y) const {
    if (!inBoard(x, y) || board[x][y] != EMPTY) return true;
    if (side != BLACK) return false;        // 白方无禁手

    Board temp = *this;                     // 拷贝棋盘模拟落子
    temp.board[x][y] = BLACK;

    // 形成五连时不禁手（五连 > 禁手）
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

    // 双四检测（包含冲四和跳四）
    int fourCount = temp.countFoursAt(x, y, BLACK);
    if (fourCount >= 2) return true;

    return false;
}

// =========================================================================
//  合法着法生成
// =========================================================================

/**
 * @brief 生成合法着法列表
 *
 * 两种模式：
 * 1. 全盘模式 (onlyNearby=false)：遍历全部 225 个位置
 * 2. 邻近模式 (onlyNearby=true)：只在已有棋子周围 radius 范围内生成
 *
 * 默认排除禁手着法。若邻近模式结果为空且不含禁手，回退到全盘模式。
 */
std::vector<Move> Board::generateLegalMoves(bool includeForbidden, bool onlyNearby, int radius) const {
    if (!onlyNearby) {
        // 全盘生成模式
        std::vector<Move> moves;
        for (int i = 0; i < SIZE; ++i)
            for (int j = 0; j < SIZE; ++j) {
                if (board[i][j] != EMPTY) continue;
                if (!includeForbidden && isForbidden(i, j)) continue;
                moves.emplace_back(i, j);
            }
        return moves;
    }

    // 邻近生成模式：以每个已有棋子为中心，搜索 radius 范围内的空位
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

    // 若棋盘为空（无任何棋子），生成中心 3x3 区域
    if (candidates.empty()) {
        int center = SIZE / 2;                  // 中心点 (7, 7)
        for (int dx = -1; dx <= 1; ++dx)
            for (int dy = -1; dy <= 1; ++dy) {
                int nx = center + dx, ny = center + dy;
                if (inBoard(nx, ny)) candidates.insert(Move(nx, ny));
            }
    }

    // 过滤禁手，构建返回列表
    std::vector<Move> moves;
    for (const Move& m : candidates) {
        if (!includeForbidden && isForbidden(m.x, m.y)) continue;
        moves.push_back(m);
    }

    // 若邻近生成后无合法着法且排除禁手，回退为全盘生成
    if (moves.empty() && !includeForbidden && onlyNearby) {
        return generateLegalMoves(false, false);
    }

    return moves;
}

// =========================================================================
//  局面评估 — 增量维护
// =========================================================================

namespace {
    const int SCORE_OPEN_FOUR  = 100000;
    const int SCORE_FOUR       = 10000;
    const int SCORE_OPEN_THREE = 5000;
    const int SCORE_THREE      = 800;
    const int SCORE_OPEN_TWO   = 500;
    const int SCORE_TWO        = 50;
}

int Board::scoreRun(int len, bool openBefore, bool openAfter) {
    if (len >= 5 || len < 2) return 0;
    int openCount = (openBefore ? 1 : 0) + (openAfter ? 1 : 0);
    if (len == 4) return openCount == 2 ? SCORE_OPEN_FOUR : (openCount >= 1 ? SCORE_FOUR : 0);
    if (len == 3) return openCount == 2 ? SCORE_OPEN_THREE : (openCount == 1 ? SCORE_THREE : 0);
    if (len == 2) return openCount == 2 ? SCORE_OPEN_TWO : (openCount == 1 ? SCORE_TWO : 0);
    return 0;
}

/**
 * @brief 计算在 (x,y) 落一子对黑/白评分的增量
 *
 * 仅在 makeMove 前调用（此时 (x,y) 为空）。
 * 对 4 个方向分别计算：
 * - 己方连子扩展/合并导致的加分
 * - 对方连子的开放端被阻挡导致的扣分
 *
 * @param deltaBlack [out] 黑方评分变化量
 * @param deltaWhite [out] 白方评分变化量
 */
void Board::computeDelta(int x, int y, int stone, int& deltaBlack, int& deltaWhite) const {
    deltaBlack = 0;
    deltaWhite = 0;

    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    int oppColor = -stone;

    for (auto& d : dirs) {
        int dx = d[0], dy = d[1];

        // ── 己方：左/右侧旧连子 + 新子合并 ──
        int leftLen = 0, nx = x - dx, ny = y - dy;
        while (inBoard(nx, ny) && board[nx][ny] == stone) { ++leftLen; nx -= dx; ny -= dy; }
        bool leftOpen = inBoard(nx, ny) && board[nx][ny] == EMPTY;

        int rightLen = 0;
        nx = x + dx; ny = y + dy;
        while (inBoard(nx, ny) && board[nx][ny] == stone) { ++rightLen; nx += dx; ny += dy; }
        bool rightOpen = inBoard(nx, ny) && board[nx][ny] == EMPTY;

        // 旧评分：左右两侧是分离的两个连子（内侧均以空位结束）
        int oldSelfScore = 0;
        if (leftLen >= 2)  oldSelfScore += scoreRun(leftLen,  leftOpen, true);
        if (rightLen >= 2) oldSelfScore += scoreRun(rightLen, true, rightOpen);

        // 新评分：合并后的一条连子
        int newLen = leftLen + 1 + rightLen;
        int newSelfScore = (newLen >= 2 && newLen < 5) ? scoreRun(newLen, leftOpen, rightOpen) : 0;

        int selfDelta = newSelfScore - oldSelfScore;
        if (stone == BLACK) deltaBlack += selfDelta;
        else                deltaWhite += selfDelta;

        // ── 对方：左右相邻的对方连子，其内侧开放端被阻挡 ──
        auto handleOppRun = [&](int sideLen, bool outerOpen, bool isLeft) {
            if (sideLen < 2 || sideLen >= 5) return 0;
            // 旧评分：内侧开放端指向 (x,y) 空位
            int oldScore = scoreRun(sideLen, isLeft ? outerOpen : true, isLeft ? true : outerOpen);
            // 新评分：内侧被 stone 阻挡
            int newScore = scoreRun(sideLen, isLeft ? outerOpen : false, isLeft ? false : outerOpen);
            return newScore - oldScore;
        };

        // 左侧对方连子
        int oppLeftLen = 0;
        nx = x - dx; ny = y - dy;
        while (inBoard(nx, ny) && board[nx][ny] == oppColor) { ++oppLeftLen; nx -= dx; ny -= dy; }
        bool oppLeftOuterOpen = inBoard(nx, ny) && board[nx][ny] == EMPTY;
        int oppLeftDelta = handleOppRun(oppLeftLen, oppLeftOuterOpen, true);
        if (oppColor == BLACK) deltaBlack += oppLeftDelta;
        else                   deltaWhite += oppLeftDelta;

        // 右侧对方连子
        int oppRightLen = 0;
        nx = x + dx; ny = y + dy;
        while (inBoard(nx, ny) && board[nx][ny] == oppColor) { ++oppRightLen; nx += dx; ny += dy; }
        bool oppRightOuterOpen = inBoard(nx, ny) && board[nx][ny] == EMPTY;
        int oppRightDelta = handleOppRun(oppRightLen, oppRightOuterOpen, false);
        if (oppColor == BLACK) deltaBlack += oppRightDelta;
        else                   deltaWhite += oppRightDelta;
    }
}

/**
 * @brief 全盘重新计算评分（全量，用于初始化及验证）
 *
 * 与旧 evaluateBoth 逻辑一致，遍历全部 225×4 个方向。
 * 仅在 reset() 和调试时调用。
 */
void Board::fullRecalcScores() {
    cachedBlackScore = 0;
    cachedWhiteScore = 0;
    const int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    bool visited[SIZE][SIZE][4] = {false};

    for (int x = 0; x < SIZE; ++x) {
        for (int y = 0; y < SIZE; ++y) {
            int stone = board[x][y];
            if (stone == EMPTY) continue;
            for (int d = 0; d < 4; ++d) {
                if (visited[x][y][d]) continue;
                int dx = dirs[d][0], dy = dirs[d][1];

                int count = 1;
                int nx = x + dx, ny = y + dy;
                while (inBoard(nx, ny) && board[nx][ny] == stone) { ++count; nx += dx; ny += dy; }
                bool openEnd1 = inBoard(nx, ny) && board[nx][ny] == EMPTY;

                nx = x - dx; ny = y - dy;
                while (inBoard(nx, ny) && board[nx][ny] == stone) { ++count; nx -= dx; ny -= dy; }
                bool openEnd2 = inBoard(nx, ny) && board[nx][ny] == EMPTY;

                int tx = x, ty = y;
                while (inBoard(tx, ty) && board[tx][ty] == stone) { visited[tx][ty][d] = true; tx += dx; ty += dy; }
                tx = x - dx; ty = y - dy;
                while (inBoard(tx, ty) && board[tx][ty] == stone) { visited[tx][ty][d] = true; tx -= dx; ty -= dy; }

                if (count >= 5) continue;
                int sc = scoreRun(count, openEnd1, openEnd2);
                if (stone == BLACK) cachedBlackScore += sc;
                else                cachedWhiteScore += sc;
            }
        }
    }
}

bool Board::hasLegalMoves() const {
    return !generateLegalMoves(false, false).empty();
}
