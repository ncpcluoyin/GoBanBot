# GobanBot - 五子棋AI

**GobanBot** 是一个高性能的五子棋人工智能程序，支持两种先进的AI模型：**Negamax**（Alpha-Beta 剪枝 + 置换表）和 **MCTS+DNN**（蒙特卡洛树搜索 + 深度神经网络）。程序提供人机对弈和AI自对弈（模型对战）模式，具备禁手规则、彩色终端、日志记录、棋谱保存等完整功能。

---

## ✨ 主要特性

| 特性 | 说明 |
|------|------|
| 🧠 **双AI引擎** | Negamax（传统搜索）与 MCTS+DNN（机器学习）可任意切换 |
| ⚡ **高性能搜索** | Negamax 支持多线程、置换表缓存、启发式移动排序 |
| 🤖 **神经网络** | MCTS 使用 Eigen 库（AVX2 加速），可保存/加载模型权重 |
| 🚫 **禁手规则** | 完整实现黑棋（先手）三三、四四、长连禁手 |
| 🎨 **视觉反馈** | 最后一次落子高亮（玩家绿色 / AI 红色），棋盘紧凑显示 |
| 📊 **对战模式** | AI vs AI 自动对弈，支持不同模型对战（如 Negamax vs MCTS） |
| 📝 **日志系统** | 异步记录每步棋的思考时间、评估值、模拟次数、缓存命中率 |
| ♟️ **棋谱保存** | 对局自动保存为 PGN 格式，便于后续分析 |
| 🔧 **可配置** | 搜索深度、线程数、MCTS 模拟次数、神经网络结构等均可运行时设置 |

---

## 🖥️ 系统要求

- **操作系统**：Linux / macOS / Windows (MSYS2 / WSL2)
- **编译器**：支持 C++17 的编译器（GCC 7+, Clang 5+, MSVC 2019+）
- **构建工具**：CMake 3.12+
- **依赖库**：
  - [Eigen3](https://eigen.tuxfamily.org/) (>=3.3) – 神经网络计算
  - pthread (Linux/macOS 自带，Windows 需 MSYS2 环境)

---

## 📦 安装与编译

### 1. 安装依赖

#### Ubuntu / Debian
```bash
sudo apt update
sudo apt install build-essential cmake libeigen3-dev
```

#### macOS (Homebrew)
```bash
brew install cmake eigen
```

#### Windows (MSYS2 Clang64)
```bash
pacman -S mingw-w64-clang-x86_64-cmake mingw-w64-clang-x86_64-eigen3 mingw-w64-clang-x86_64-toolchain
```
启动 `MSYS2 CLANG64` 终端进行后续操作。

### 2. 克隆代码
```bash
git clone https://github.com/ncpcluoyin/GoBanBot
cd gobanbot
```

### 3. 编译
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

编译完成后，可执行文件为 `gobanbot`（或 `gobanbot.exe`）。

---

## 🎮 使用方法

### 启动程序
```bash
./gobanbot
```

程序会依次提示选择模式、AI模型、参数等。

### 模式选择

| 模式 | 说明 |
|------|------|
| **1. Human vs AI** | 玩家执 X，AI 执 O，轮流落子 |
| **2. AI vs AI** | 两个AI自动对战，可统计胜率并保存棋谱 |

### AI 模型参数

#### Negamax 模型
- **搜索深度**：推荐 3~4（深度 4 棋力较强但耗时增加）
- **线程数**：建议等于 CPU 核心数

#### MCTS+DNN 模型
- **模拟次数**：每步棋的 MCTS 模拟次数，推荐 1000~5000
- **线程数**：并行模拟线程数，建议等于 CPU 核心数
- **神经网络结构**：格式 `输入维度 隐藏层1 隐藏层2`（默认 `225 128 64`）
- **权重文件**：预训练模型路径（留空则随机初始化）

### 对战模式额外选项（AI vs AI）
- **对局数量**：进行多少盘对弈
- **保存PGN**：是否将棋谱保存到 `logs/` 目录
- **每步打印棋盘**：是否实时显示棋盘（便于观察）

### 棋盘显示说明
- **列号**：仅显示个位数（第10列显示 `0`，第11列显示 `1`，…，第15列显示 `5`）
- **行号**：完整显示 1~15
- **棋子**：`X` = 玩家 / AI1，`O` = AI / AI2
- **高亮**：最后一次落子用颜色标记（玩家绿色，AI 红色）

### 玩家落子格式
输入 `行 列` 或 `行,列`，例如：
```
7 7
7,7
```
行列范围均为 1~15。

---

## 📂 输出文件

程序运行时会自动创建 `logs/` 目录，包含以下文件：

| 文件 | 内容 |
|------|------|
| `gobanbot.log` | 全局日志，记录每步棋的详细信息（时间、评估值、耗时等） |
| `battle_N.pgn` | 第 N 局 AI 对战的棋谱（PGN 格式） |

### 日志示例
```
2025-04-18 10:30:01 AI move at (8,8) eval=0.62 sims=2000 cacheHits=0 time=1523ms
```

---

## ⚙️ 高级配置

### 修改默认参数
部分硬编码常量可在源码中调整：

- **`negamax_ai.cpp`**：`WIN_SCORE`, `LIVE_FOUR_SCORE`, `DEFENSE_WEIGHT`, `CANDIDATE_RADIUS`
- **`mcts_ai.cpp`**：MCTS 探索常数（`uctValue` 中的 `exploration`，默认 1.414）
- **`board.cpp`**：棋盘大小 `BOARD_SIZE`（默认 15）

### 启用/禁用颜色输出
若终端不支持 ANSI 颜色，可在 `board.cpp` 的 `print()` 函数中删除 `\033[31m` 等转义序列。

---

## 🧪 模型对战示例

### Negamax (深度3) vs MCTS (2000次模拟)
```bash
Select mode (1: Human vs AI, 2: AI vs AI): 2
=== First AI (AI1) ===
AI1 model (1: Negamax, 2: MCTS+DNN): 1
Search depth: 3
Threads: 4
=== Second AI (AI2) ===
AI2 model (1: Negamax, 2: MCTS+DNN): 2
MCTS simulations: 2000
Threads: 4
Weights file (empty for random): 
Number of games: 10
Save PGN logs? (y/n): y
Print board after each move? (y/n): n
...
Final: AI1 wins 4, AI2 wins 5, draws 1
```

---

## ❓ 常见问题

### Q1: 编译时找不到 Eigen3？
**A**: 确保已安装 Eigen3 库。若 CMake 仍报错，可手动指定路径：
```bash
cmake .. -DEigen3_DIR=/usr/share/eigen3/cmake
```
或在 `CMakeLists.txt` 中设置 `set(Eigen3_DIR "/path/to/eigen3")`。

### Q2: 棋盘列号为什么显示 0 1 2 ... 5 而不是 10 11 15？
**A**: 为保持紧凑显示，列号仅显示个位数。实际落子仍按行列号输入（如第10列输入 `10`）。

### Q3: MCTS 模型思考时间过长？
**A**: 可减少 `simulations` 参数（如 500 或 1000），或增加线程数。对于实时对弈，建议模拟次数 ≤ 2000。

### Q4: 如何训练自己的神经网络权重？
**A**: 可使用外部 Python 脚本（如 TensorFlow/PyTorch）训练一个输入 225 维、输出胜率的网络，然后将权重保存为二进制文件（按 `neural_net.cpp` 中的格式：先 `w1`, `b1`, `w2`, `b2`, `w3`, `b3` 的顺序连续写入 float）。本程序只加载权重，不包含训练逻辑。

### Q5: 为什么 AI 有时会不落子？
**A**: 可能所有合法落子点均为禁手点（黑棋无步可走）。五子棋规则中，若黑棋无合法步数则判负。程序会输出 "cannot move" 并退出。

### Q6: 在 Windows 下如何运行？
**A**: 推荐使用 MSYS2 Clang64 环境编译（见上文）。编译后的 `.exe` 可直接在 MSYS2 终端运行，或移至其他位置（需确保依赖的 DLL 在 PATH 中）。

### Q7: 如何查看实时日志？
**A**: 在另一个终端执行：
```bash
tail -f logs/gobanbot.log
```

---

## 📜 许可证

本项目仅供学习交流使用，未经许可不得用于商业用途。

---

## 🤝 贡献

欢迎提交 Issue 或 Pull Request。如有任何问题，请通过邮件或 GitHub 联系。

---

**Enjoy the game!** 🎉