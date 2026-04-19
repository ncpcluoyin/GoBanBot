cmake_minimum_required(VERSION 3.10)
project(GoBanBot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 多线程支持
find_package(Threads REQUIRED)

add_executable(GoBanBot
    src/main.cpp
    src/Board.cpp
    src/TransTable.cpp
    src/Search.cpp
    src/Game.cpp
)

target_link_libraries(GoBanBot Threads::Threads)