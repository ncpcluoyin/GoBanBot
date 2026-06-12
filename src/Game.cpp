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
 * 1. 从 GoBanBot.cfg 读取配置（深度、线程、限时、高亮）
 * 2. 选择游戏模式（人机 / AI对弈）
 * 3. 选择人类颜色（人机模式）
 * 4. 应用配置文件中的参数到 AI 引擎
 */
void Game::setup() {
    clearScreen();
    std::cout << "=== Welcome to GoBanBot ===\n\n";

    // 从配置文件加载参数
    Config config = loadConfig();
    std::cout << "Loaded config: depth=" << config.depth
              << " threads=" << config.threads
              << " max_time_ms=" << config.maxTimeMs
              << " null_move_r=" << config.nullMoveR
              << " highlight=" << (config.highlight ? "on" : "off")
              << " swap=" << (config.swapRule ? "on" : "off")
              << " twoMove=" << (config.twoMoveRule ? "on" : "off") << "\n\n";

    // 选择游戏模式
    std::cout << "Select mode:\n";
    std::cout << "1. Human vs AI\n";
    std::cout << "2. AI vs AI\n";
    int choice = intPrompt("Enter choice", 1, 2);
    mode = (choice == 1) ? HUMAN_VS_AI : AI_VS_AI;

    // 从配置文件读取规则参数
    highlight = config.highlight;
    swapRule = config.swapRule;
    twoMoveRule = config.twoMoveRule;

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

        aiBlack.setDepth(config.depth);
        aiWhite.setDepth(config.depth);
    } else {
        aiBlack.setDepth(config.depth);
        aiWhite.setDepth(config.depth);
    }

    aiBlack.setThreads(config.threads);
    aiWhite.setThreads(config.threads);
    aiBlack.setMaxTime(config.maxTimeMs);
    aiWhite.setMaxTime(config.maxTimeMs);
    aiBlack.setNullMoveReduction(config.nullMoveR);
    aiWhite.setNullMoveReduction(config.nullMoveR);

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
 * @brief 检查着法是否在开局允许范围内
 *
 * 开局限制：
 * - 第 1 手（moveCount==0）：必须落在中心 H8
 * - 第 2 手（moveCount==1）：天元周围 3×3 区域 (G7-I9)
 * - 第 3 手（moveCount==2）：天元周围 5×5 区域 (F6-J10)
 */
bool Game::isValidOpening(int x, int y) const {
    if (moveCount >= 3) return true;
    int c = Board::SIZE / 2;
    if (moveCount == 0) return x == c && y == c;
    if (moveCount == 1) return std::abs(x - c) <= 1 && std::abs(y - c) <= 1;
    return std::abs(x - c) <= 2 && std::abs(y - c) <= 2;  // moveCount == 2
}

/**
 * @brief 三手交换下 AI 均衡着法选择
 *
 * 在 swapRule 开启且 AI 执黑走第 3 手时使用：
 * 1. 全深度搜索获取 Top-7 候选及其搜索评分
 * 2. 按评分升序排列（低分 = 黑方优势小）
 * 3. 选取评分 >= 0 且最低的着法（刚好不劣，但优势最小）
 *
 * 原理：黑方优势过大会触发白方交换，黑方反而变成劣势方。
 * 黑方最优策略是制造尽可能均衡的局面（评分 ≈ 0）。
 */
void Game::aiTurnBalanced(Search& ai, const std::string& name) {
    std::cout << name << " is thinking (balanced)...\n";
    *stopFlag = false;
    ai.setStopFlag(stopFlag);

    // 全深度搜索 Top-7，返回 (着法, 评分)
    auto scored = ai.getTopMovesScored(board, 7);
    if (scored.size() <= 1) {
        aiTurn(ai, name);
        return;
    }

    // 按评分升序（黑方优势最小的在前）
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    // 选择评分 >= 0 且最低的着法（刚好不劣，优势最小）
    Move bestMove = scored[0].first;
    for (const auto& [m, s] : scored) {
        if (s >= 0) {
            bestMove = m;
            break;
        }
    }

    board.makeMove(bestMove.x, bestMove.y, board.getSide());
    std::cout << name << " plays " << char('A'+bestMove.x) << bestMove.y+1 << "\n";
}

/**
 * @brief 处理人类玩家输入
 *
 * 输入格式：字母+数字，如 "H8" 表示第 H 列第 8 行。
 * 对非法输入（格式错误、越界、禁手、开局限制）会提示并重新要求输入。
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
        if (!isValidOpening(x, y)) {
            if (moveCount == 0)
                std::cout << "First move must be at H8 (center).\n";
            else if (moveCount == 1)
                std::cout << "Move must be within 3x3 around center (G7-I9).\n";
            else
                std::cout << "Move must be within 5x5 central area (F6-J10).\n";
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

// =========================================================================
//  三手交换
// =========================================================================

/**
 * @brief 三手交换规则处理
 *
 * 第 3 手结束后，白方（第二玩家）可选择是否交换颜色。
 * 交换后 humanSide 翻转，走棋方不变（仍为白方走第 4 手）。
 */
void Game::handleSwap() {
    // 确定白方是人类还是 AI
    // 注意：走棋方轮替为 side = -stone，所以 3 手后 getSide() 是 WHITE
    bool whiteIsHuman = (humanSide == Board::WHITE);

    bool doSwap = false;
    if (whiteIsHuman) {
        std::cout << "\n--- Swap Rule ---\n";
        std::cout << "You are White. You may swap colors with Black.\n";
        std::cout << "If you swap, you will play as Black from now on.\n";
        doSwap = yesNoPrompt("Do you want to swap?");
    } else {
        // AI 作为白方，基于局面评估决定是否交换
        int blackScore, whiteScore;
        board.evaluateBoth(blackScore, whiteScore);
        int adv = blackScore - whiteScore;       // 正值 = 黑方优势

        // 交换阈值：若黑方优势明显（先行利），AI 交换拿黑
        // 3 手时棋盘仅有 3 子，通常差值较小；阈值不宜过大
        doSwap = (adv > 800);
        std::cout << "\n--- Swap Rule ---\n";
        std::cout << "AI (White) evaluates: Black+=" << adv
                  << " | " << (doSwap ? "swaps!" : "does not swap.") << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(800));
    }

    if (doSwap) {
        humanSide = -humanSide;              // 翻转人类执棋颜色
        std::cout << "Colors swapped! You now play as "
                  << (humanSide == Board::BLACK ? "Black (X)" : "White (O)") << ".\n";
        std::cout << "Press Enter to continue...";
        std::cin.ignore();
        std::cin.get();
    }
}

// =========================================================================
//  五手两打
// =========================================================================

/**
 * @brief 五手两打规则处理
 *
 * 第 5 手时，黑方必须提出两个不同着法，白方选择其中一个实际落子。
 */
void Game::handleTwoMoves() {
    int blackSide = board.getSide();         // 此时应为 BLACK
    bool blackIsHuman = (humanSide == Board::BLACK);

    std::vector<Move> candidates;

    if (blackIsHuman) {
        // ── 人类是黑方：输入两个候选着法 ──
        std::cout << "\n--- Two-Move Rule (5th move) ---\n";
        std::cout << "As Black, you must propose TWO different moves.\n";
        std::cout << "White (AI) will choose one of them.\n\n";

        for (int i = 0; i < 2; ++i) {
            while (true) {
                std::cout << "Proposal " << (i+1) << " (e.g., H8): ";
                std::string input;
                std::cin >> input;
                if (input.length() < 2) {
                    std::cout << "Invalid format. Use letter+number (A-O, 1-15).\n";
                    continue;
                }
                char colChar = std::toupper(input[0]);
                int x = colChar - 'A';
                std::string rowStr = input.substr(1);
                int y;
                try { y = std::stoi(rowStr) - 1; }
                catch (...) { std::cout << "Invalid number.\n"; continue; }

                if (!board.inBoard(x, y) || !board.isEmpty(x, y)) {
                    std::cout << "Illegal move.\n";
                    continue;
                }
                if (board.isForbidden(x, y)) {
                    std::cout << "That move is forbidden for Black.\n";
                    continue;
                }
                // 确保不与已提交的候选重复
                bool dup = false;
                for (const Move& c : candidates) {
                    if (c.x == x && c.y == y) { dup = true; break; }
                }
                if (dup) {
                    std::cout << "Move already proposed. Choose a different one.\n";
                    continue;
                }
                candidates.emplace_back(x, y);
                break;
            }
        }

        // AI（白方）从两个候选中选择一个
        // 选择对白方最有利的着法（即黑方得分最低的）
        int bestForWhite = 1000000;          // 极大值
        Move chosen = candidates[0];
        for (const Move& m : candidates) {
            Board temp = board;
            temp.makeMove(m.x, m.y, Board::BLACK);
            int blackScore, whiteScore;
            temp.evaluateBoth(blackScore, whiteScore);
            int score = blackScore - whiteScore;   // 正值利于黑方，负值利于白方
            if (score < bestForWhite) {
                bestForWhite = score;
                chosen = m;
            }
        }
        std::cout << "AI (White) chooses: " << char('A'+chosen.x) << chosen.y+1 << "\n";
        board.makeMove(chosen.x, chosen.y, blackSide);

    } else {
        // ── AI 是黑方：AI 提出两个候选，人类选择 ──
        std::cout << "\n--- Two-Move Rule (5th move) ---\n";
        std::cout << "AI (Black) is thinking...\n";

        *stopFlag = false;
        aiBlack.setStopFlag(stopFlag);
        // 获取 Top-5 候选，用于对抗性配对
        std::vector<Move> topCandidates = aiBlack.getTopMoves(board, 5);

        if (topCandidates.size() < 2) {
            if (!topCandidates.empty()) {
                board.makeMove(topCandidates[0].x, topCandidates[0].y, blackSide);
                std::cout << "AI (Black) plays " << char('A'+topCandidates[0].x)
                          << topCandidates[0].y+1 << "\n";
            }
            return;
        }

        // 对抗性配对：人类（白方）会选择对黑方最不利的着法
        // AI 的目标：最大化 min(score1, score2)，即两个候选中最差的那个也要尽量好
        // 对每个候选，用迷你搜索评估 blackScore - whiteScore
        std::vector<std::pair<int, Move>> evaluated;
        for (const Move& m : topCandidates) {
            Board temp = board;
            temp.makeMove(m.x, m.y, Board::BLACK);
            int bs, ws;
            temp.evaluateBoth(bs, ws);
            int adv = bs - ws;             // 黑方优势值（越大黑方越有利）
            evaluated.emplace_back(adv, m);
        }

        // 遍历所有配对，选择 max-min 最优
        int bestPairMin = -1000000;
        int bestI = 0, bestJ = 1;
        for (size_t i = 0; i < evaluated.size(); ++i) {
            for (size_t j = i + 1; j < evaluated.size(); ++j) {
                int pairMin = std::min(evaluated[i].first, evaluated[j].first);
                if (pairMin > bestPairMin) {
                    bestPairMin = pairMin;
                    bestI = (int)i; bestJ = (int)j;
                }
            }
        }

        candidates = {evaluated[bestI].second, evaluated[bestJ].second};

        std::cout << "AI (Black) proposes two moves:\n";
        std::cout << "  1. " << char('A'+candidates[0].x) << candidates[0].y+1 << "\n";
        std::cout << "  2. " << char('A'+candidates[1].x) << candidates[1].y+1 << "\n";

        int choice = intPrompt("As White, choose one", 1, 2);
        Move chosen = candidates[choice - 1];
        board.makeMove(chosen.x, chosen.y, blackSide);
    }

    std::cout << "Press Enter to continue...";
    std::cin.ignore();
    std::cin.get();
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
    board.reset();
    moveCount = 0;
    swapDone = false;

    while (!gameOver) {
        clearScreen();
        printBoard();

        // ── 三手交换（人机模式，第 3 手后） ──
        if (mode == HUMAN_VS_AI && swapRule && moveCount == 3 && !swapDone) {
            handleSwap();
            swapDone = true;
        }

        // ── 五手两打（人机模式，第 5 手） ──
        if (mode == HUMAN_VS_AI && twoMoveRule && moveCount == 4) {
            handleTwoMoves();
            moveCount++;
            checkGameOver(gameOver);
            continue;
        }

        // ── 正常着法 ──
        // 开局前 3 手：仅人机模式下设置根着法边界（AI vs AI 不受限）
        if (moveCount < 3 && mode == HUMAN_VS_AI) {
            int c = Board::SIZE / 2;
            if (moveCount == 0) {
                aiBlack.setRootBounds(c, c, c, c);
                aiWhite.setRootBounds(c, c, c, c);
            } else if (moveCount == 1) {
                aiBlack.setRootBounds(c-1, c-1, c+1, c+1);
                aiWhite.setRootBounds(c-1, c-1, c+1, c+1);
            } else {
                aiBlack.setRootBounds(c-2, c-2, c+2, c+2);
                aiWhite.setRootBounds(c-2, c-2, c+2, c+2);
            }
        } else {
            aiBlack.clearRootBounds();
            aiWhite.clearRootBounds();
        }

        if (mode == HUMAN_VS_AI) {
            if (board.getSide() == humanSide) {
                humanTurn();
            } else {
                // 三手交换下 AI 执黑第 3 手：选均衡着法防止被交换
                if (swapRule && moveCount == 2 && board.getSide() == Board::BLACK)
                    aiTurnBalanced(aiBlack, "AI (Black)");
                else if (board.getSide() == Board::BLACK)
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
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        moveCount++;
        checkGameOver(gameOver);
    }

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
