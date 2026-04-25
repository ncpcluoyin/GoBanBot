# GoBanBot - 五子棋AI

使用 Negamax + Alpha-Beta 剪枝的五子棋对战程序，支持人机/机机对弈、禁手规则、多线程搜索。

## 快速开始

### 编译
```bash
mkdir build && cd build
cmake ..
make
```

### 运行
```bash
./GoBanBot
```

## 主要功能

- 人机对弈（可选执黑或执白）或 AI vs AI
- 禁手自动处理（黑方禁手点不能下）
- 多线程搜索，可自定义线程数
- 最后落子高亮（红/绿，可选）

## 对弈操作

1. 根据英文提示选择模式、深度、线程数等。
2. 人类落子输入例如 `H8`（列字母 A-O，行数字 1-15）。
3. 程序结束时按任意键退出。

## 项目结构

```
GoBanBot/
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp
    ├── Board.h / Board.cpp
    ├── Search.h / Search.cpp
    ├── Game.h / Game.cpp
    └── ...
```

## 联系方式

欢迎提 Issue / Pull Request。