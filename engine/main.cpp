#include <iostream>
#include "board.h"

int main() {
    std::cout << "Chess Engine C++ v.1.0\n";

    std::string initial_state = "rnbqkbnr/pp1p1ppP/8/2pP4/8/8/PPP1PPPP/RNBQKBNR b KQkq - 0 2";
    chess::ChessBoard b(initial_state);
    std::cout << sizeof(chess::ChessBoard) << "\n";
    std::cout << initial_state << "\n";
    std::cout << b.generate_fen() << "\n";
    auto moves = b.get_legal_moves();
    b.print();
    for(auto move : moves) {
        chess::ChessBoard a = b;
        std::cout << move.from.standard_notation() << " " << move.to.standard_notation() << "\n";
        a.execute_move(move);
        a.print();
        std::cout << a.calculate_material() << "\n";
        std::cout << a.generate_fen() << "\n";
    }
    
    return 0;
}