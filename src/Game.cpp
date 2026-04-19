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

Game::Game() : mode(HUMAN_VS_AI), highlight(false), humanSide(Board::BLACK) {
    stopFlag = std::make_shared<std::atomic<bool>>(false);
}

void Game::setup() {
    clearScreen();
    std::cout << "=== Welcome to GoBanBot ===\n\n";
    std::cout << "Select mode:\n";
    std::cout << "1. Human vs AI\n";
    std::cout << "2. AI vs AI\n";
    int choice = intPrompt("Enter choice", 1, 2);
    mode = (choice == 1) ? HUMAN_VS_AI : AI_VS_AI;

    highlight = yesNoPrompt("Enable last move highlight?");

    int depthBlack = intPrompt("Enter search depth for Black (AI)", 2, 12);
    aiBlack.setDepth(depthBlack);

    if (mode == AI_VS_AI) {
        int depthWhite = intPrompt("Enter search depth for White (AI)", 2, 12);
        aiWhite.setDepth(depthWhite);
    } else {
        aiWhite.setDepth(depthBlack);
        std::cout << "You play as Black (X). AI plays as White (O).\n";
        humanSide = Board::BLACK;
    }

    // 修改：不限制最大线程数，maxVal = -1 表示无上限
    int threads = intPrompt("Enter number of search threads", 1, -1);
    aiBlack.setThreads(threads);
    aiWhite.setThreads(threads);

    std::cout << "\nPress Enter to start game...";
    std::cin.ignore();
    std::cin.get();
    clearScreen();
}

void Game::printBoard() const {
    std::cout << "   ";
    for (int i = 0; i < Board::SIZE; ++i) std::cout << (char)('A' + i) << ' ';
    std::cout << '\n';

    Move last = board.getLastMove();
    int lastStone = (last.valid()) ? board.get(last.x, last.y) : Board::EMPTY;

    for (int y = 0; y < Board::SIZE; ++y) {
        std::cout << (y+1 < 10 ? " " : "") << y+1 << ' ';
        for (int x = 0; x < Board::SIZE; ++x) {
            char c;
            int stone = board.get(x, y);
            if (stone == Board::BLACK) c = 'X';
            else if (stone == Board::WHITE) c = 'O';
            else c = '.';

            bool isLast = (last.x == x && last.y == y);
            if (highlight && isLast) {
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
        int x = colChar - 'A';
        std::string rowStr = input.substr(1);
        int y;
        try { y = std::stoi(rowStr) - 1; }
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

void Game::checkGameOver(bool& over) {
    int winner = board.checkWinner();
    if (winner != Board::EMPTY) {
        over = true;
        announceResult(winner);
    } else if (board.isFull()) {
        over = true;
        announceResult(Board::EMPTY);
    }
}

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

void Game::run() {
    bool gameOver = false;
    board.reset();

    while (!gameOver) {
        clearScreen();
        printBoard();

        if (mode == HUMAN_VS_AI) {
            if (board.getSide() == humanSide) {
                humanTurn();
            } else {
                aiTurn(aiWhite, "AI (White)");
            }
        } else {
            if (board.getSide() == Board::BLACK) {
                aiTurn(aiBlack, "AI Black");
            } else {
                aiTurn(aiWhite, "AI White");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        checkGameOver(gameOver);
    }

    clearScreen();
    printBoard();
    announceResult(board.checkWinner());
}

void Game::waitExit() {
    std::cout << "\nPress any key to exit...";
#ifdef _WIN32
    _getch();
#else
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    std::cout << std::endl;
}