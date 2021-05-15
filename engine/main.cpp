#include <iostream>
#include <chrono>
#include "board.h"

uint64_t perft(chess::Board b, int depth, int max_depth, bool verbose) {
    uint64_t nodes = 0;
    auto moves = b.get_legal_moves();
    if(depth == 1) {
        return moves.size();
    }
    for(auto &move : moves) {
        chess::Board c = b;
        c.execute_move(move);
        uint64_t children = perft(c, depth-1, max_depth, verbose);
        if(depth == max_depth && verbose) {
            std::cout << move.from.standard_notation() << move.to.standard_notation() << ": " << children << "\n";
        }
        nodes += children;
    }
    return nodes;
}

void perft_command() {
    int depth;
    std::cout << "Enter perft depth: ";
    std::cin >> depth;

    chess::Board b;
    b.print();
    std::cout << b.generate_fen() << "\n";

    for(int i = 1; i <= depth; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        std::cout << "Perft(" << i << ") = ";
        uint64_t nodes = perft(b, i, i, false);
        std::cout << nodes << " (";
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << duration.count()/1000000.0 << " s)\n";
    }
}

void debug_command() {
    int depth;
    // std::string fen_string;
    // std::cout << "Enter FEN string: ";
    // std::cin >> fen_string;
    std::cout << "Enter perft depth: ";
    std::cin >> depth;

    std::string castle = "4k2r/8/8/8/8/8/8/r3K2R w - - 0 1";
    std::string double_pin = "4k3/6N1/5b2/4R3/8/8/8/4K3 b - - 0 1";
    chess::Board b;
    b.print();
    std::cout << b.generate_fen() << "\n";

    std::string move_input;
    chess::Move move = {chess::Position(), chess::Position(), chess::MoveFlag::Invalid};
    while(true) {
        uint64_t nodes = perft(b, depth, depth, true);
        std::cout << "Evaluated " << nodes << " nodes\n";

        while(move.is_invalid()) {
            std::cout << "Enter move to visit subtree> ";
            std::cin >> move_input;
            std::string from = move_input.substr(0, 2);
            std::string to = move_input.substr(2, 2);
            move = b.create_move(chess::Position(from[0], from[1]), chess::Position(to[0], to[1]));
        }
        b.execute_move(move);
        b.print();
        std::cout << b.generate_fen() << "\n";
        move = {chess::Position(), chess::Position(), chess::MoveFlag::Invalid};
    }
}

void test_castling_command() {
    std::string fen = "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1";
    chess::Board b("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
    std::cout << fen << "\n" << b.generate_fen() <<"\n";
    auto moves = b.get_legal_moves();
    for(auto &move : moves) {
        chess::Board c = b;
        c.execute_move(move);
        if(move.flags & chess::MoveFlag::Castle) c.print();
        // std::cout << c.generate_fen() << "\n";
    }
}

int main() {
    std::cout << "Chess Engine C++ v.1.0\n";
    std::string command;
    while(true) {
        std::cout << "Enter command> ";
        std::cin >> command;
        if(command == "perft") {
            perft_command();
        }
        else if(command == "debug") {
            debug_command();
        }
        else if(command == "castle") {
            test_castling_command();
        }
        else if(command == "quit") {
            break;
        }
    }
    return 0;
}