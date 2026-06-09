/**
 * @file Game.h
 * @brief 游戏主控类定义
 *
 * 管理游戏生命周期：设置、运行、退出。
 * 支持人机对弈和 AI vs AI 两种模式。
 */

#ifndef GAME_H
#define GAME_H

#include "Board.h"
#include "Search.h"
#include <atomic>
#include <memory>

class Game {
public:
    Game();

    /** @brief 游戏设置：模式选择、难度、线程数等 */
    void setup();

    /** @brief 游戏主循环 */
    void run();

    /** @brief 等待用户按键后退出（跨平台实现） */
    void waitExit();

private:
    Board board;                                   /**< 棋盘状态 */
    Search aiBlack, aiWhite;                       /**< 黑白双方的 AI 引擎 */
    std::shared_ptr<std::atomic<bool>> stopFlag;    /**< AI 停止标志（共享指针） */

    enum Mode { HUMAN_VS_AI, AI_VS_AI };           /**< 游戏模式 */
    Mode mode;                                     /**< 当前游戏模式 */
    bool highlight;                                /**< 是否高亮最后落子 */
    int humanSide;                                 /**< 人类执棋颜色（人机模式） */

    /** @brief 打印当前棋盘到控制台（含坐标和最后落子高亮） */
    void printBoard() const;

    /** @brief 处理人类玩家输入 */
    void humanTurn();

    /** @brief 让 AI 走一步棋 */
    void aiTurn(Search& ai, const std::string& name);

    /** @brief 检查游戏是否结束（胜负/平局/无合法着法） */
    void checkGameOver(bool& over);

    /** @brief 宣布游戏结果 */
    void announceResult(int winner) const;
};

#endif