#include "game.h"
#include <iostream>
#include <string>
using namespace std;

#include "game.h"
#include <iostream>
#include <string>

Game::Game(std::unique_ptr<GomokuAI> ai, bool playerFirst)
    : ai(std::move(ai)),          // 转移所有权
      playerTurnFlag(playerFirst),
      lastPlayerX(-1), lastPlayerY(-1),
      lastAIX(-1), lastAIY(-1) {}

bool Game::handleGameEnd(int x, int y, int piece) {
    if (board.checkWin(x, y, piece)) {
        board.print(lastPlayerX, lastPlayerY, lastAIX, lastAIY);
        if (piece == PLAYER) cout << "恭喜！您赢了！" << endl;
        else cout << "AI 获胜！您输了。" << endl;
        return true;
    }
    if (board.isFull()) {
        board.print(lastPlayerX, lastPlayerY, lastAIX, lastAIY);
        cout << "平局！棋盘已满。" << endl;
        return true;
    }
    return false;
}

void Game::playerMove() {
    int x, y;
    while (true) {
        cout << "请输入落子坐标（行 列，如：7 7）：";
        string input;
        cin >> input;
        size_t pos = input.find(',');
        if (pos != string::npos) {
            x = stoi(input.substr(0, pos)) - 1;
            y = stoi(input.substr(pos + 1)) - 1;
        } else {
            int ty;
            cin >> ty;
            x = stoi(input) - 1;
            y = ty - 1;
        }
        if (!board.isValidMove(x, y)) {
            cout << "无效落子，请重新输入。" << endl;
            continue;
        }
        break;
    }
    board.setPiece(x, y, PLAYER);
    lastPlayerX = x; lastPlayerY = y;
    cout << "玩家落子于 (" << x+1 << "," << y+1 << ")" << endl;
    board.print(lastPlayerX, lastPlayerY, lastAIX, lastAIY);
    if (handleGameEnd(x, y, PLAYER)) exit(0);
}

void Game::aiMove() {
    cout << "AI 思考中..." << endl;
    auto [x, y] = ai->getMove(board, AI_PIECE, PLAYER);
    if (x == -1) return;
    board.setPiece(x, y, AI_PIECE);
    lastAIX = x; lastAIY = y;
    cout << "AI 落子于 (" << x+1 << "," << y+1 << ")" << endl;
    board.print(lastPlayerX, lastPlayerY, lastAIX, lastAIY);
    if (handleGameEnd(x, y, AI_PIECE)) exit(0);
}

void Game::run() {
    board.init();
    board.print(lastPlayerX, lastPlayerY, lastAIX, lastAIY);
    while (true) {
        if (playerTurnFlag) playerMove();
        else aiMove();
        playerTurnFlag = !playerTurnFlag;
    }
}