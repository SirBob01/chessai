#CMakeLists.txt
cmake_minimum_required (VERSION 3.6)
project(example)

add_executable(chessai chessai.cpp vendor/Brainiac/engine/bits.cpp 
                                   vendor/Brainiac/engine/board.cpp 
                                   vendor/Brainiac/engine/brainiac.cpp 
                                   vendor/Brainiac/engine/move.cpp 
                                   vendor/Brainiac/engine/transpositions.cpp 
                                   vendor/Brainiac/engine/util.cpp 
                                   vendor/Brainiac/engine/zobrist.cpp)

add_subdirectory(vendor/sleepy-discord)
target_link_libraries(chessai sleepy-discord)