#CMakeLists.txt
cmake_minimum_required (VERSION 3.6)
project(example)

add_executable(chessai chessai.cpp engine/board.cpp engine/bits.cpp engine/util.cpp engine/move.cpp)

add_subdirectory(deps/sleepy-discord)
target_link_libraries(chessai sleepy-discord)