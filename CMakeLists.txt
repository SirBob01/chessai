cmake_minimum_required (VERSION 3.6)
project(chessai)

add_executable(chessai src/chessai.cpp src/id.cpp src/image.cpp)
add_subdirectory(src/vendor/DPP)
add_subdirectory(src/vendor/Brainiac)

target_link_libraries(chessai dpp brainiac)