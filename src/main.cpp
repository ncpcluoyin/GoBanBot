/**
 * @file main.cpp
 * @brief GoBanBot 五子棋 AI 程序入口
 *
 * 初始化 Game 对象，依次执行游戏设置、游戏主循环和退出等待。
 */

#include "Game.h"

int main() {
    Game game;           // 创建游戏主控对象
    game.setup();        // 游戏模式与参数设置（人机/AI对弈、难度、线程数等）
    game.run();          // 启动游戏主循环
    game.waitExit();     // 等待用户按键后退出
    return 0;
}