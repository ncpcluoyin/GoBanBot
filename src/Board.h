/**
 * @file Board.h
 * @brief 五子棋棋盘类定义
 *
 * 封装 15x15 棋盘的状态表示、着法合法性判定、禁手检测、
 * Zobrist 哈希维护及局面评估功能。
 */

#ifndef BOARD_H
#define BOARD_H

#include <cstdint>
#include <vector>
#include <string>

/**
 * @brief 棋步结构体
 *
 * 棋盘坐标范围 [0, 14]，无效着法用 (-1, -1) 表示。
 */
struct Move {
    int x, y;
    Move() : x(-1), y(-1) {}
    Move(int _x, int _y) : x(_x), y(_y) {}
    bool valid() const { return x >= 0 && x < 15 && y >= 0 && y < 15; }
    bool operator==(const Move& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Move& other) const { return !(*this == other); }
};

/**
 * @brief 五子棋棋盘类
 *
 * 使用二维数组存储 15x15 棋盘状态，支持走子/悔棋、胜负判定、
 * 禁手检测、合法着法生成、Zobrist 哈希和局面评估。
 */
class Board {
public:
    static const int SIZE = 15;      /**< 棋盘尺寸 */
    static const int EMPTY = 0;      /**< 空位置 */
    static const int BLACK = 1;      /**< 黑棋 */
    static const int WHITE = -1;     /**< 白棋 */

    Board();
    Board(const Board& other) = default;
    Board& operator=(const Board& other) = default;

    /** @brief 重置棋盘为初始状态（全空，黑方先行） */
    void reset();

    /** @brief 获取 (x, y) 位置的棋子 */
    int get(int x, int y) const { return board[x][y]; }
    /** @brief 设置 (x, y) 位置的棋子（不更新哈希和走棋方） */
    void set(int x, int y, int stone) { board[x][y] = stone; }
    /** @brief 检查 (x, y) 是否为空 */
    bool isEmpty(int x, int y) const { return board[x][y] == EMPTY; }
    /** @brief 检查 (x, y) 是否在棋盘范围内 */
    bool inBoard(int x, int y) const { return x>=0 && x<SIZE && y>=0 && y<SIZE; }

    /**
     * @brief 执行着法
     * @return true 表示着法合法且已执行
     */
    bool makeMove(int x, int y, int stone);
    /** @brief 撤销着法，恢复棋盘到之前状态 */
    void undoMove(int x, int y);

    /** @brief 获取最后一步着法 */
    Move getLastMove() const { return lastMove; }
    void setLastMove(Move m) { lastMove = m; }

    /** @brief 获取当前走棋方 */
    int getSide() const { return side; }
    /** @brief 设置当前走棋方（同步更新 Zobrist 哈希） */
    void setSide(int s) { if (s != side) { side = s; hash ^= zobristSideKey; } }

    /** @brief 检查是否已有胜者，返回获胜方（BLACK/WHITE），无胜者返回 EMPTY */
    int checkWinner() const;

    /** @brief 棋盘是否已满（平局） */
    bool isFull() const;

    /**
     * @brief 检测 (x, y) 位置是否为禁手
     *
     * 仅对黑方生效：长连、双三、双四均为禁手。
     * 形成五连时不禁手。
     */
    bool isForbidden(int x, int y) const;

    /**
     * @brief 生成当前局面的合法着法列表
     * @param includeForbidden 是否包含禁手着法（默认不包含）
     * @param onlyNearby       是否仅在有棋子的邻近位置生成（默认是）
     * @param radius           邻近搜索半径（默认 2）
     * @return 合法着法向量
     */
    std::vector<Move> generateLegalMoves(bool includeForbidden = false,
                                          bool onlyNearby = true,
                                          int radius = 2) const;

    /** @brief 获取当前局面的 Zobrist 哈希值 */
    uint64_t zobrist() const { return hash; }

    /**
     * @brief 评估某方在当前局面的得分
     * @param stone 要评估的棋子颜色 (BLACK / WHITE)
     * @return 该方的局面评分
     */
    int evaluateColor(int stone) const;

    /**
     * @brief 一次遍历同时评估双方局面得分
     * @param blackScore [out] 黑方评分
     * @param whiteScore [out] 白方评分
     */
    void evaluateBoth(int& blackScore, int& whiteScore) const;

    /** @brief 检查当前走棋方是否存在合法着法（不含禁手） */
    bool hasLegalMoves() const;

private:
    int board[SIZE][SIZE];      /**< 棋盘数据，board[x][y] */
    Move lastMove;              /**< 最后一步着法记录 */
    int side;                   /**< 当前走棋方 (BLACK / WHITE) */
    uint64_t hash;              /**< Zobrist 哈希值 */

    /** Zobrist 哈希随机数表，全局共享，首次构造时初始化 */
    static uint64_t zobristTable[SIZE][SIZE][2];
    static uint64_t zobristSideKey;            /**< 走棋方哈希常数 */
    static bool zobristInitialized;
    static void initZobrist();

    /** @brief 统计 (x,y) 沿方向 (dx,dy) 连续同色棋子数（不含起点） */
    int countDirection(int x, int y, int dx, int dy, int stone) const;

    /** @brief 检查 (x,y) 处 stone 颜色是否形成五连 */
    bool isFive(int x, int y, int stone) const;

    /** @brief 检查 (x,y) 处黑棋是否形成长连（>5 子） */
    bool isOverline(int x, int y) const;

    /** @brief 检查 (x,y) 处黑棋在方向 (dx,dy) 上是否参与活三（含跳活三） */
    bool isLiveThree(int x, int y, int dx, int dy) const;

    /** @brief 统计 (x,y) 处某方棋子的冲四/活四数量（含跳四） */
    int countFoursAt(int x, int y, int stone) const;
};

#endif