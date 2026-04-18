#ifndef UTILS_H
#define UTILS_H

#include <cstdint>

constexpr int BOARD_SIZE = 15;
constexpr int EMPTY = 0;
constexpr int PLAYER = 1;   // X
constexpr int AI_PIECE = 2; // O

// 禁手类型位掩码
constexpr int NO_FORBIDDEN = 0;
constexpr int FORBIDDEN_THREE_THREE = 1;
constexpr int FORBIDDEN_FOUR_FOUR = 2;
constexpr int FORBIDDEN_OVERLINE = 4;

using ZobristKey = uint64_t;

#endif