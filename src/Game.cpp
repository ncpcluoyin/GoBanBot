/**
 * @file Game.cpp
 * @brief 游戏主控实现
 *
 * 管理游戏完整生命周期：初始化设置、主循环运行、结果判定与退出。
 * 支持人机对弈和 AI vs AI 两种模式，棋盘支持高亮最后落子。
 */

#include "Game.h"
#include "Utils.h"
#include <iostream>
#include <thread>
#include <sstream>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

// =========================================================================
//  构造与初始化
// =========================================================================

Game::Game() : mode(HUMAN_VS_AI), highlight(false), humanSide(Board::BLACK) {
    stopFlag = std::make_shared<std::atomic<bool>>(false);
}

// =========================================================================
//  游戏设置
// =========================================================================

/**
 * @brief 游戏初始化设置
 *
 * 设置流程：
 * 1. 选择游戏模式（人机 / AI对弈）
 * 2. 是否开启最后落子高亮
 * 3. 选择人类颜色（人机模式）
 * 4. 设置 AI 搜索深度（人机统一深度，AI对弈可分别设置）
 * 5. 设置搜索线程数
 */
void Game::setup() {
    clearScreen();
    std::cout << "=== Welcome to GoBanBot ===\n\n";

    // 选择游戏模式
    std::cout << "Select mode:\n";
    std::cout << "1. Human vs AI\n";
    std::cout << "2. AI vs AI\n";
    int choice = intPrompt("Enter choice", 1, 2);
    mode = (choice == 1) ? HUMAN_VS_AI : AI_VS_AI;

    // 是否高亮最后落子
    highlight = yesNoPrompt("Enable last move highlight?");

    if (mode == HUMAN_VS_AI) {
        // 人类选择执黑或执白
        std::cout << "Choose your color:\n";
        std::cout << "  B - Black (X, first)\n";
        std::cout << "  W - White (O, second)\n";
        char col;
        while (true) {
            std::cout << "Your choice (B/W): ";
            std::cin >> col;
            col = std::toupper(col);
            if (col == 'B') {
                humanSide = Board::BLACK;
                break;
            } else if (col == 'W') {
                humanSide = Board::WHITE;
                break;
            }
            std::cout << "Invalid choice. Please enter B or W.\n";
        }
        std::cout << "You play as " << (humanSide == Board::BLACK ? "Black (X)" : "White (O)") << ".\n";
        std::cout << "AI plays as " << (humanSide == Board::BLACK ? "White (O)" : "Black (X)") << ".\n";

        // 人机模式：统一 AI 深度
        int aiDepth = intPrompt("Enter AI search depth", 2, 12);
        aiBlack.setDepth(aiDepth);
        aiWhite.setDepth(aiDepth);
    } else {
        // AI vs AI 模式：黑白双方可分别设置深度
        int depthBlack = intPrompt("Enter search depth for Black (AI)", 2, 12);
        aiBlack.setDepth(depthBlack);
        int depthWhite = intPrompt("Enter search depth for White (AI)", 2, 12);
        aiWhite.setDepth(depthWhite);
    }

    // 设置搜索线程数（无上限，传入 -1 表示不检查上限）
    int threads = intPrompt("Enter number of search threads", 1, -1);
    aiBlack.setThreads(threads);
    aiWhite.setThreads(threads);

    std::cout << "\nPress Enter to start game...";
    std::cin.ignore();
    std::cin.get();
    clearScreen();
}

// =========================================================================
//  棋盘打印
// =========================================================================

/**
 * @brief 打印当前棋盘到控制台
 *
 * 格式说明：
 * - 列标签：A-O（列坐标 0-14）
 * - 行标签：1-15（行坐标 0-14）
 * - 棋子符号：X = 黑棋，O = 白棋，. = 空位
 * - 高亮模式：最后落子用红色（黑）或绿色（白）高亮显示
 */
void Game::printBoard() const {
    // 列坐标标签
    std::cout << "   ";
    for (int i = 0; i < Board::SIZE; ++i) std::cout << (char)('A' + i) << ' ';
    std::cout << '\n';

    Move last = board.getLastMove();
    int lastStone = (last.valid()) ? board.get(last.x, last.y) : Board::EMPTY;

    for (int y = 0; y < Board::SIZE; ++y) {
        std::cout << (y+1 < 10 ? " " : "") << y+1 << ' ';  // 行号对齐
        for (int x = 0; x < Board::SIZE; ++x) {
            char c;
            int stone = board.get(x, y);
            if (stone == Board::BLACK) c = 'X';
            else if (stone == Board::WHITE) c = 'O';
            else c = '.';

            bool isLast = (last.x == x && last.y == y);
            if (highlight && isLast) {
                // ANSI 颜色码：\033[1;31m=亮红色(黑子), \033[1;32m=亮绿色(白子)
                if (lastStone == Board::BLACK) std::cout << "\033[1;31m";
                else std::cout << "\033[1;32m";
                std::cout << c << "\033[0m ";
            } else {
                std::cout << c << ' ';
            }
        }
        std::cout << '\n';
    }
    std::cout << std::endl;
}

// =========================================================================
//  着法处理
// =========================================================================

/**
 * @brief 处理人类玩家输入
 *
 * 输入格式：字母+数字，如 "H8" 表示第 H 列第 8 行。
 * 对非法输入（格式错误、越界、禁手）会提示并重新要求输入。
 */
void Game::humanTurn() {
    while (true) {
        std::cout << "Your move (e.g., H8): ";
        std::string input;
        std::cin >> input;
        if (input.length() < 2) {
            std::cout << "Invalid format. Use letter+number (A-O, 1-15).\n";
            continue;
        }
        char colChar = std::toupper(input[0]);
        int x = colChar - 'A';                              // 列号 0-14
        std::string rowStr = input.substr(1);
        int y;
        try { y = std::stoi(rowStr) - 1; }                  // 行号 0-14
        catch (...) {
            std::cout << "Invalid number.\n";
            continue;
        }
        if (!board.inBoard(x, y) || !board.isEmpty(x, y)) {
            std::cout << "Illegal move.\n";
            continue;
        }
        if (board.isForbidden(x, y)) {
            std::cout << "That move is forbidden for Black.\n";
            continue;
        }
        board.makeMove(x, y, humanSide);
        break;
    }
}

/**
 * @brief AI 走棋
 *
 * 重置停止标志，调用搜索引擎获取最佳着法并执行。
 * 若 AI 无合法着法（极稀有情况），输出提示信息。
 */
void Game::aiTurn(Search& ai, const std::string& name) {
    std::cout << name << " is thinking...\n";
    *stopFlag = false;
    ai.setStopFlag(stopFlag);
    Move m = ai.getBestMove(board);
    if (!m.valid() || !board.isEmpty(m.x, m.y)) {
        std::cout << name << " has no legal move.\n";
        return;
    }
    board.makeMove(m.x, m.y, board.getSide());
    std::cout << name << " plays " << char('A'+m.x) << m.y+1 << "\n";
}

// =========================================================================
//  游戏结束判定
// =========================================================================

/**
 * @brief 检查游戏结束条件
 *
 * 结束条件（按优先级）：
 * 1. 有玩家形成五连 → 胜者
 * 2. 棋盘已满 → 平局
 * 3. 当前走棋方无合法着法（黑方全部禁手） → 黑负
 */
void Game::checkGameOver(bool& over) {
    int winner = board.checkWinner();
    if (winner != Board::EMPTY) {
        over = true;
        announceResult(winner);
        return;
    }

    if (board.isFull()) {
        over = true;
        announceResult(Board::EMPTY);                   // 棋盘满 → 平局
        return;
    }

    // 当前走棋方无合法着法（只有黑方可能因全盘禁手导致）
    if (!board.hasLegalMoves()) {
        over = true;
        if (board.getSide() == Board::BLACK) {
            std::cout << "Black has no legal move (all empty points are forbidden).\n";
            announceResult(Board::WHITE);               // 黑方无着法 → 白胜
        } else {
            // 白方无合法着法仅可能发生在棋盘满时，上面已处理
            announceResult(Board::EMPTY);
        }
    }
}

/**
 * @brief 清屏并宣布游戏结果
 */
void Game::announceResult(int winner) const {
    clearScreen();
    printBoard();
    if (winner == Board::BLACK)
        std::cout << "=== Black (X) wins! ===\n";
    else if (winner == Board::WHITE)
        std::cout << "=== White (O) wins! ===\n";
    else
        std::cout << "=== Draw! Board is full. ===\n";
}

// =========================================================================
//  主循环
// =========================================================================

/**
 * @brief 游戏主循环
 *
 * 在 gameOver 变为 true 前循环执行：
 * 1. 清屏并打印棋盘
 * 2. 根据当前模式和走棋方执行着法
 * 3. 检测游戏是否结束
 *
 * AI vs AI 模式下每步间有 500ms 延迟便于观察。
 */
void Game::run() {
    bool gameOver = false;
    board.reset();                                      // 重置棋盘到初始状态

    while (!gameOver) {
        clearScreen();
        printBoard();

        if (mode == HUMAN_VS_AI) {
            if (board.getSide() == humanSide) {
                humanTurn();                            // 人类回合
            } else {
                // AI 回合：根据当前颜色使用对应的 AI 引擎
                if (board.getSide() == Board::BLACK)
                    aiTurn(aiBlack, "AI (Black)");
                else
                    aiTurn(aiWhite, "AI (White)");
            }
        } else {
            // AI vs AI 模式
            if (board.getSide() == Board::BLACK) {
                aiTurn(aiBlack, "AI Black");
            } else {
                aiTurn(aiWhite, "AI White");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 观察延迟
        }

        checkGameOver(gameOver);
    }

    // 游戏结束：最终清屏并打印结果
    clearScreen();
    printBoard();
    announceResult(board.checkWinner());
}

// =========================================================================
//  退出
// =========================================================================

/**
 * @brief 等待用户按键退出（跨平台实现）
 *
 * Windows 使用 _getch()，Linux/macOS 使用 termios 设置终端为
 * 非规范模式以读取单字符（不回显）。
 */
void Game::waitExit() {
    std::cout << "\nPress any key to exit...";
#ifdef _WIN32
    _getch();
#else
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);                     // 获取当前终端属性
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);                   // 关闭规范模式和回显
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    getchar();                                          // 等待任意按键
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);            // 恢复终端属性
#endif
    std::cout << std::endl;
}
