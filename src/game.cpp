#include "game.h"
#include "logger.h"
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

Game::Game(unique_ptr<GomokuAI> ai1, unique_ptr<GomokuAI> ai2, bool humanFirst, bool highlight)
    : playerAI(move(ai1)), opponentAI(move(ai2)), humanTurn(humanFirst),
      highlightLast(highlight), lastPlayerX(-1), lastPlayerY(-1), lastAIX(-1), lastAIY(-1) {}

void Game::printStatus(const string& info) {
    string panel = info;
    board.print(lastPlayerX, lastPlayerY, lastAIX, lastAIY, panel);
}

bool Game::handleGameEnd(int x, int y, int piece, const string& winner) {
    if (board.checkWin(x, y, piece)) {
        printStatus(winner + " wins!");
        cout << winner << " wins!" << endl;
        return true;
    }
    if (board.isFull()) {
        printStatus("Draw!");
        cout << "Draw!" << endl;
        return true;
    }
    return false;
}

void Game::playerMove() {
    int x, y;
    while (true) {
        cout << "Enter move (row col, e.g., 7 7): ";
        string in;
        cin >> in;
        size_t pos = in.find(',');
        if (pos != string::npos) {
            x = stoi(in.substr(0, pos)) - 1;
            y = stoi(in.substr(pos + 1)) - 1;
        } else {
            int ty;
            cin >> ty;
            x = stoi(in) - 1;
            y = ty - 1;
        }
        if (!board.isValidMove(x, y, PLAYER)) {
            int ft = board.getForbiddenType(x, y, PLAYER);
            if (ft != NO_FORBIDDEN) {
                cout << "Forbidden move! ";
                if (ft & FORBIDDEN_OVERLINE) cout << "Overline ";
                if (ft & FORBIDDEN_THREE_THREE) cout << "Three-three ";
                if (ft & FORBIDDEN_FOUR_FOUR) cout << "Four-four ";
                cout << endl;
            } else {
                cout << "Invalid move. Try again." << endl;
            }
            continue;
        }
        break;
    }
    board.setPiece(x, y, PLAYER);
    lastPlayerX = x; lastPlayerY = y;
    printStatus("Player moved at (" + to_string(x+1) + "," + to_string(y+1) + ")");
    if (handleGameEnd(x, y, PLAYER, "Player")) exit(0);
}

void Game::aiMove(unique_ptr<GomokuAI>& ai, int piece, const string& name, int& lastX, int& lastY, bool isFirst) {
    cout << name << " thinking..." << endl;
    AIResult res = ai->getMove(board, piece, (piece == PLAYER) ? AI_PIECE : PLAYER);
    if (res.x == -1) {
        cout << name << " cannot move!" << endl;
        return;
    }
    board.setPiece(res.x, res.y, piece);
    lastX = res.x; lastY = res.y;
    string info = name + " move at (" + to_string(res.x+1) + "," + to_string(res.y+1) + ")";
    info += " eval=" + to_string(res.evaluation) + " time=" + to_string(res.timeMs) + "ms";
    if (res.cacheHits > 0) info += " cacheHits=" + to_string(res.cacheHits);
    printStatus(info);
    Logger::instance().logMove(name, res.x, res.y, res.evaluation, res.simulations, res.cacheHits, res.timeMs);
    if (handleGameEnd(res.x, res.y, piece, name)) exit(0);
}

void Game::run() {
    board.init();
    printStatus("Game started. You are X, AI is O.");
    while (true) {
        if (humanTurn) {
            playerMove();
            humanTurn = false;
        } else {
            aiMove(playerAI, AI_PIECE, "AI", lastAIX, lastAIY, false);
            humanTurn = true;
        }
    }
}

void Game::writePGN(const vector<MoveRecord>& moves, const string& filename) const {
    ofstream ofs(filename);
    if (!ofs) return;
    ofs << "[Event \"AI Battle\"]\n[Site \"GobanBot\"]\n[GameType \"Gomoku\"]\n\n";
    for (size_t i = 0; i < moves.size(); ++i) {
        char col = 'a' + moves[i].y;
        int row = moves[i].x + 1;
        ofs << col << row;
        if ((i+1) % 10 == 0) ofs << "\n";
        else ofs << " ";
    }
    ofs << "\n";
}

void Game::runBattle(int numGames, bool savePGN, bool printBoardEachMove) {
    int win1 = 0, win2 = 0, draw = 0;
    for (int g = 0; g < numGames; ++g) {
        board.init();
        bool turn = true; // true: playerAI (AI1) 先手
        vector<MoveRecord> moves;
        // 打印初始棋盘
        if (printBoardEachMove) {
            board.print(lastPlayerX, lastPlayerY, lastAIX, lastAIY, "Battle started");
        }
        while (true) {
            AIResult res;
            string name;
            int piece;
            if (turn) {
                name = "AI1 (Negamax/MCTS)";
                piece = AI_PIECE;
                res = playerAI->getMove(board, AI_PIECE, PLAYER);
            } else {
                name = "AI2 (Negamax/MCTS)";
                piece = PLAYER;
                res = opponentAI->getMove(board, PLAYER, AI_PIECE);
            }
            if (res.x == -1) break;
            board.setPiece(res.x, res.y, piece);
            moves.push_back({turn ? 0 : 1, res.x, res.y, res.evaluation, res.simulations, res.cacheHits, res.timeMs});

            if (printBoardEachMove) {
                string info = name + " move at (" + to_string(res.x+1) + "," + to_string(res.y+1) + ")";
                info += " eval=" + to_string(res.evaluation) + " time=" + to_string(res.timeMs) + "ms";
                board.print(lastPlayerX, lastPlayerY, lastAIX, lastAIY, info);
            }

            if (board.checkWin(res.x, res.y, piece)) {
                if (turn) win1++; else win2++;
                if (printBoardEachMove) cout << name << " wins game " << g+1 << "!" << endl;
                break;
            }
            if (board.isFull()) { draw++; break; }
            turn = !turn;
        }
        cout << "Game " << g+1 << " finished. Score: " << win1 << " - " << win2 << " - " << draw << endl;
        if (savePGN) {
            string fname = "logs/battle_" + to_string(g+1) + ".pgn";
            writePGN(moves, fname);
        }
    }
    cout << "Final: AI1 wins " << win1 << ", AI2 wins " << win2 << ", draws " << draw << endl;
}