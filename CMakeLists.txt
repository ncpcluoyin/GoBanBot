# =============================================================================
#  GoBanBot — 五子棋 AI 程序
#  C++17 标准，最高优化级别，多线程并行搜索
# =============================================================================

cmake_minimum_required(VERSION 3.10)
project(GoBanBot)

# 使用 C++17 标准，强制启用（不满足则编译错误）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release 模式开启 -O3 最高优化
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# 多线程依赖（pthread，跨平台）
find_package(Threads REQUIRED)

# 可执行文件目标
add_executable(GoBanBot
    src/main.cpp
    src/Board.cpp
    src/TransTable.cpp
    src/Search.cpp
    src/Game.cpp
)

# 链接线程库
target_link_libraries(GoBanBot Threads::Threads)