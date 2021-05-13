#include <iostream>
#include "board.h"

int main() {
    std::cout << "Chess Engine C++ v.1.0\n";

    std::string initial_state = "k7/2PN4/1K6/8/8/8/8/8 b KQkq - 0 2";
    chess::Board b(initial_state);
    b.generate();
    std::cout << sizeof(chess::Board) << "\n";
    std::cout << initial_state << "\n";
    std::cout << b.generate_fen() << "\n";
    auto moves = b.get_legal_moves();
    b.print();
    if(moves.size() == 0) {
        std::cout << "Checkmate!\n";
    }
    for(auto move : moves) {
        chess::Board a = b;
        std::cout << move.from.standard_notation() << " " << move.to.standard_notation() << "\n";
        a.execute_move(move);
        a.print();
        std::cout << a.calculate_material() << "\n";
        std::cout << a.generate_fen() << "\n";
    }
    
    return 0;
}