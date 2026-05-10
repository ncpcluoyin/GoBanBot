cmake_minimum_required(VERSION 3.10)
project(GoBanBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启最高优化
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(Threads REQUIRED)

add_executable(GoBanBot
    src/main.cpp
    src/Board.cpp
    src/TransTable.cpp
    src/Search.cpp
    src/Game.cpp
)

target_link_libraries(GoBanBot Threads::Threads)