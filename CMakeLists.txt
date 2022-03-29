cmake_minimum_required (VERSION 3.6)
project(chessai)

add_executable(chessai chessai.cpp)
add_subdirectory(vendor/DPP)
add_subdirectory(vendor/Brainiac)

target_link_libraries(chessai dpp brainiac)