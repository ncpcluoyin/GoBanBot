/**
 * @file Utils.h
 * @brief 工具函数集合
 *
 * 提供跨平台的控制台清屏、字符/字符串大小写转换、以及用户交互式输入提示
 * (yes/no 确认、整数输入) 等通用辅助功能。
 */

#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>

/**
 * @brief 清除控制台屏幕
 *
 * Windows 使用 cls 命令，Linux/macOS 使用 clear 命令。
 */
inline void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/** @brief 将单个字符转为小写 */
inline char toLower(char c) { return std::tolower(static_cast<unsigned char>(c)); }

/** @brief 将字符串转为全小写 */
inline std::string toLower(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return res;
}

/**
 * @brief 向用户询问 yes/no 问题
 * @param question 显示的问句
 * @return true 表示回答 "y"（不区分大小写），false 表示回答 "n" 或其他
 */
inline bool yesNoPrompt(const std::string& question) {
    std::cout << question << " (y/n): ";
    std::string ans;
    std::cin >> ans;
    ans = toLower(ans);
    return (!ans.empty() && ans[0] == 'y');
}

/**
 * @brief 向用户询问一个范围内的整数
 * @param question 显示的问句
 * @param minVal  最小值
 * @param maxVal  最大值；传入负数表示无上限（只约束最小值）
 * @return 用户输入的合法整数
 */
inline int intPrompt(const std::string& question, int minVal, int maxVal) {
    int val;
    while (true) {
        std::cout << question;
        if (maxVal < 0) std::cout << " [" << minVal << "+]";
        else std::cout << " [" << minVal << "-" << maxVal << "]";
        std::cout << ": ";
        std::cin >> val;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            std::cout << "Invalid input. Please enter a number.\n";
            continue;
        }
        if (val >= minVal && (maxVal < 0 || val <= maxVal)) break;
        std::cout << "Value out of range. Try again.\n";
    }
    return val;
}

#endif