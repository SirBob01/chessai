#include <iostream>
#include "board.h"

int main() {
    std::cout << "Chess Engine C++ v.1.0\n";

    std::string initial_state = "rnbqkbnr/pp1ppppP/8/2pP4/8/8/PPP1PPPP/RNBQKBNR w KQkq c6 0 2";
    chess::ChessBoard b(initial_state);
    std::cout << sizeof(chess::ChessBoard) << "\n";
    std::cout << initial_state << "\n";
    std::cout << b.generate_fen() << "\n";
    auto moves = b.generate_move_list();
    b.print();
    for(auto move : moves) {
        chess::ChessBoard a = b;
        a.execute_move(move);
        a.print();
        std::cout << a.calculate_material() << "\n";
        std::cout << a.generate_fen() << "\n";
    }
    
    return 0;
}