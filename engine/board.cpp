#include <bitset>
#include <iostream>

// Board has 8*8 = 64 cells
// Each cell can be (pawn, rook, bishop, knight, queen, king) * (black, white) + (empty) = 13 possible states
// Hence, each cell state can be represented by a 4 bit number (0 - 16)
// Each row is a 32 bit integer
// The entire board can be represented by a 256 bit number
enum ChessPieces {
    Empty       = 0,
    WhitePawn   = 1,
    WhiteRook   = 2,
    WhiteKnight = 3,
    WhiteBishop = 4,
    WhiteQueen  = 5,
    WhiteKing   = 6,
    
    BlackPawn   = 1 | (1 << 3),
    BlackRook   = 2 | (1 << 3),
    BlackKnight = 3 | (1 << 3),
    BlackBishop = 4 | (1 << 3),
    BlackQueen  = 5 | (1 << 3),
    BlackKing   = 6 | (1 << 3)
};

class ChessBoard {
    uint32_t state[8] = {0};

public:
    ChessBoard(bool initial=true) {
        if(initial) {
            set_at('a', '8', ChessPieces::BlackRook);
            set_at('b', '8', ChessPieces::BlackKnight);
            set_at('c', '8', ChessPieces::BlackBishop);
            set_at('d', '8', ChessPieces::BlackQueen);
            set_at('e', '8', ChessPieces::BlackKing);
            set_at('f', '8', ChessPieces::BlackBishop);
            set_at('g', '8', ChessPieces::BlackKnight);
            set_at('h', '8', ChessPieces::BlackRook);

            set_at('a', '7', ChessPieces::BlackPawn);
            set_at('b', '7', ChessPieces::BlackPawn);
            set_at('c', '7', ChessPieces::BlackPawn);
            set_at('d', '7', ChessPieces::BlackPawn);
            set_at('e', '7', ChessPieces::BlackPawn);
            set_at('f', '7', ChessPieces::BlackPawn);
            set_at('g', '7', ChessPieces::BlackPawn);
            set_at('h', '7', ChessPieces::BlackPawn);


            set_at('a', '1', ChessPieces::WhiteRook);
            set_at('b', '1', ChessPieces::WhiteKnight);
            set_at('c', '1', ChessPieces::WhiteBishop);
            set_at('d', '1', ChessPieces::WhiteQueen);
            set_at('e', '1', ChessPieces::WhiteKing);
            set_at('f', '1', ChessPieces::WhiteBishop);
            set_at('g', '1', ChessPieces::WhiteKnight);
            set_at('h', '1', ChessPieces::WhiteRook);

            set_at('a', '2', ChessPieces::WhitePawn);
            set_at('b', '2', ChessPieces::WhitePawn);
            set_at('c', '2', ChessPieces::WhitePawn);
            set_at('d', '2', ChessPieces::WhitePawn);
            set_at('e', '2', ChessPieces::WhitePawn);
            set_at('f', '2', ChessPieces::WhitePawn);
            set_at('g', '2', ChessPieces::WhitePawn);
            set_at('h', '2', ChessPieces::WhitePawn);
        }
    }

    inline unsigned get_at(char file, char rank) {
        unsigned row = rank - '1';
        unsigned col = 4 * (file - 'a');

        return (state[row] >> col) & 15;
    }

    inline void set_at(char file, char rank, uint8_t piece) {
        unsigned row = rank - '1';
        unsigned col = 4 * (file - 'a');

        state[row] = (state[row] & ~(15 << col)) | (piece << col);
    }

    void print() {
        for(auto &row : state) {
            std::cout << std::bitset<32>(row) << "\n";
        }
    }
};
