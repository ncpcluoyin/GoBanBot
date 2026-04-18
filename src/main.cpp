#include <iostream>
#include <memory>
#include <limits>
#include "game.h"
#include "negamax_ai.h"
#include "mcts_ai.h"

using namespace std;

void clearCin() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int main() {
    cout << "========== GobanBot Gomoku AI ==========" << endl;
    cout << "Player: X, AI: O, Board: 15x15" << endl;

    int mode;
    cout << "Select mode (1: Human vs AI, 2: AI vs AI): ";
    cin >> mode;
    if (mode != 1 && mode != 2) mode = 1;

    // 创建两个AI的辅助函数
    auto createAI = [](const string& prompt) -> unique_ptr<GomokuAI> {
        int modelType;
        cout << prompt << " model (1: Negamax, 2: MCTS+DNN): ";
        cin >> modelType;
        if (modelType == 1) {
            int depth, threads;
            cout << "Search depth: ";
            cin >> depth;
            cout << "Threads: ";
            cin >> threads;
            return make_unique<NegamaxAI>(depth, threads);
        } else {
            int sims, threads;
            cout << "MCTS simulations: ";
            cin >> sims;
            cout << "Threads: ";
            cin >> threads;
            string weights;
            cout << "Weights file (empty for random): ";
            cin.ignore();
            getline(cin, weights);
            return make_unique<MCTSAI>(sims, threads, weights);
        }
    };

    unique_ptr<GomokuAI> ai1, ai2;
    bool humanFirst = false;
    if (mode == 1) {
        ai1 = createAI("Select AI");
        int first;
        cout << "Who goes first? (1: Player, 2: AI): ";
        cin >> first;
        humanFirst = (first == 1);
    } else {
        cout << "=== First AI (AI1) ===" << endl;
        ai1 = createAI("AI1");
        cout << "=== Second AI (AI2) ===" << endl;
        ai2 = createAI("AI2");
    }

    bool highlight;
    char ch;
    cout << "Highlight last move? (y/n): ";
    cin >> ch;
    highlight = (ch == 'y' || ch == 'Y');

    if (mode == 1) {
        Game game(move(ai1), nullptr, humanFirst, highlight);
        game.run();
    } else {
        int numGames;
        cout << "Number of games: ";
        cin >> numGames;
        char save;
        cout << "Save PGN logs? (y/n): ";
        cin >> save;
        bool savePGN = (save == 'y' || save == 'Y');
        char printBoard;
        cout << "Print board after each move? (y/n): ";
        cin >> printBoard;
        bool printBoardEachMove = (printBoard == 'y' || printBoard == 'Y');
        Game battle(move(ai1), move(ai2), false, highlight);
        battle.runBattle(numGames, savePGN, printBoardEachMove);
    }

    cout << "Press any key to exit..." << endl;
    cin.ignore();
    cin.get();
    return 0;
}