#include "move.h"

namespace chess {
    Square::Square(char file, char rank) {
        int row = rank - '1';
        int col = file - 'a';
        shift = row * 8 + col;
    }

    Square::Square(std::string notation) {
        int row = notation[1] - '1';
        int col = notation[0] - 'a';
        shift = row * 8 + col;
    }
}