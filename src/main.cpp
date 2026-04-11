#include <iostream>
#include <memory>
#include "game.h"
#include "negamax_ai.h"

using namespace std;

int main() {
    cout << "========== 五子棋 AI：GobanBot (模块化版) ==========" << endl;
    cout << "玩家执 X，AI 执 O。棋盘大小 15x15。" << endl;

    // 询问AI模型
    cout << "请选择AI模型 (1: Negamax): ";
    int modelChoice;
    cin >> modelChoice;
    if (modelChoice != 1) {
        cout << "目前仅支持 Negamax 模型，将使用默认。" << endl;
        modelChoice = 1;
    }

    // 询问搜索深度
    int depth;
    cout << "请输入搜索深度 (推荐3或4): ";
    cin >> depth;

    // 询问线程数量
    int threads;
    cout << "请输入线程数量 (建议1~8): ";
    cin >> threads;
    if (threads < 1) threads = 1;

    // 创建AI实例
    unique_ptr<GomokuAI> ai;
    if (modelChoice == 1) {
        ai = make_unique<NegamaxAI>(depth, threads);
    }

    // 询问先手
    int first;
    cout << "请选择谁先手 (1: 玩家先手, 2: AI 先手): ";
    cin >> first;
    bool playerFirst = (first == 1);

    Game game(move(ai), playerFirst);
    game.run();

    return 0;
}