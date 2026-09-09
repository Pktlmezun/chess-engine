cmake_minimum_required(VERSION 3.16)
project(chess_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g -fsanitize=address,undefined")

set(ENGINE_SOURCES
    src/bitboard.cpp
    src/board.cpp
)

# Main engine (grows as milestones complete)
add_executable(chess_engine src/main.cpp ${ENGINE_SOURCES})
target_include_directories(chess_engine PRIVATE src)
