#ifndef CHESS_MOVE_H_
#define CHESS_MOVE_H_

#include <string>

namespace chess {
    /**
     * A position on the board represented by a bitshift value (0 - 63)
     */
    struct Position {
        int shift;

        Position() : shift(-1) {}
        Position(int shift) : shift(shift) {}
        Position(char file, char rank);
        Position(std::string notation);

        std::string standard_notation();
        uint64_t get_mask();
    };

    /**
     * Flags that describe a chess move
     */
    enum MoveFlag {
        Quiet       = 0,
        Capture     = 1 << 0,
        EnPassant   = 1 << 1,
        PawnAdvance = 1 << 2,
        PawnDouble  = 1 << 3,
        Castle      = 1 << 4,
        KnightPromo = 1 << 5,
        QueenPromo  = 1 << 6,
        BishopPromo = 1 << 7,
        RookPromo   = 1 << 8,
        Invalid     = 1 << 9
    };

    /**
     * The different castling types
     */
    enum Castle {
        KingWhite  = 1,
        QueenWhite = 1 << 1,
        KingBlack  = 1 << 2,
        QueenBlack = 1 << 3,
    };

    /**
     * Container of a chess move
     */
    struct Move {
        Position from;
        Position to;
        unsigned flags = MoveFlag::Quiet;

        bool is_invalid();
    };
}

#endif