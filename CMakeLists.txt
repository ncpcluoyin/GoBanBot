cmake_minimum_required(VERSION 3.12)
project(gobanbot)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -O3 -pthread")

# 查找 Threads
find_package(Threads REQUIRED)

# 查找 Eigen3 (多种方式)
find_package(Eigen3 QUIET)
if(NOT Eigen3_FOUND)
    # 尝试使用 pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(Eigen3 REQUIRED eigen3)
    set(EIGEN3_INCLUDE_DIRS ${Eigen3_INCLUDE_DIRS})
    set(EIGEN3_LIBRARIES ${Eigen3_LIBRARIES})
else()
    set(EIGEN3_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIRS})
    set(EIGEN3_LIBRARIES ${EIGEN3_LIBRARIES})
endif()

# 如果仍然找不到，手动指定常见路径（可选）
if(NOT EIGEN3_INCLUDE_DIRS)
    set(EIGEN3_INCLUDE_DIRS "/usr/include/eigen3" "/usr/local/include/eigen3" "/mingw64/include/eigen3")
    message(STATUS "Using manual Eigen3 include path: ${EIGEN3_INCLUDE_DIRS}")
endif()

add_executable(gobanbot
    src/main.cpp
    src/board.cpp
    src/game.cpp
    src/negamax_ai.cpp
    src/mcts_ai.cpp
    src/neural_net.cpp
    src/logger.cpp
)

target_link_libraries(gobanbot Threads::Threads ${EIGEN3_LIBRARIES})
target_include_directories(gobanbot PRIVATE ${EIGEN3_INCLUDE_DIRS})