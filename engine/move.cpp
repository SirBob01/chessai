#include "move.h"

namespace chess {
    Position::Position(char file, char rank) {
        int row = rank - '1';
        int col = file - 'a';
        shift = row * 8 + col;
    }

    Position::Position(std::string notation) {
        int row = notation[1] - '1';
        int col = notation[0] - 'a';
        shift = row * 8 + col;
    }

    std::string Position::standard_notation() {
        int row = shift / 8;
        int col = shift % 8;
        char rank = row + '1';
        char field = col + 'a';
        return std::string({field, rank});
    }

    uint64_t Position::get_mask() {
        return 1ULL << shift;
    }

    bool Move::is_invalid() {
        return flags & MoveFlag::Invalid;
    }
}