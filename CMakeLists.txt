cmake_minimum_required(VERSION 3.10)
project(gobanbot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(gobanbot
    src/main.cpp
    src/board.cpp
    src/game.cpp
    src/negamax_ai.cpp
)

target_link_libraries(gobanbot Threads::Threads)