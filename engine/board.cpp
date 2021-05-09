#include <iostream>
#include <vector>

// Uniquely indexed on bitboard and piece weights
enum ChessPieces {
    WhitePawn   = 0,
    WhiteRook   = 1,
    WhiteKnight = 2,
    WhiteBishop = 3,
    WhiteQueen  = 4,
    WhiteKing   = 5,
    
    BlackPawn   = 6,
    BlackRook   = 7,
    BlackKnight = 8,
    BlackBishop = 9,
    BlackQueen  = 10,
    BlackKing   = 11,

    // Representing all white or all black pieces
    White = 12,
    Black = 13
};

// Used for calculating material score
const int piece_weights[] = {
     1,  5,  3,  3,  9,  4, // White pieces
    -1, -5, -3, -3, -9, -4  // Black pieces
};

// Standard notation
struct ChessPosition {
    char file = 0;
    char rank = 0;
};

// Utility function for tokenizing a string
std::vector<std::string> tokenize(std::string base, char delimiter) {
    std::vector<std::string> tokens;
    std::string current = "";
    for(auto &c : base) {
        if(c == delimiter) {
            if(current.length()) tokens.push_back(current);
            current = "";
        }
        else {
            current += c;
        }
    }
    if(current.length()) tokens.push_back(current);
    return tokens;
}

class ChessBoard {
    // Represent the positions of each of the 12 pieces on the board
    // Extra 2 bitboards represent white/black pieces in general
    uint64_t _bitboards[14] = {0};
    uint8_t _turn;            // w or b
    uint8_t _castling_rights; // qkQK (from most to least significant bit)
    ChessPosition _en_passant_target;
    int _halfmoves;
    int _fullmoves;

public:
    ChessBoard(std::string fen_string="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") {
        std::vector<std::string> fields = tokenize(fen_string, ' ');
        
        int row = 7;
        int col = 0;
        for(auto &c : fields[0]) {
            if(c == '/') {
                row--;
                col = 0;
            }
            else if(c <= '9' && c >= '0') {
                col += c - '0';
            }
            else {
                if(c == 'p') set_at(row, col, ChessPieces::BlackPawn);
                else if(c == 'n') set_at(row, col, ChessPieces::BlackKnight);
                else if(c == 'b') set_at(row, col, ChessPieces::BlackBishop);
                else if(c == 'r') set_at(row, col, ChessPieces::BlackRook);
                else if(c == 'q') set_at(row, col, ChessPieces::BlackQueen);
                else if(c == 'k') set_at(row, col, ChessPieces::BlackKing);

                else if(c == 'P') set_at(row, col, ChessPieces::WhitePawn);
                else if(c == 'N') set_at(row, col, ChessPieces::WhiteKnight);
                else if(c == 'B') set_at(row, col, ChessPieces::WhiteBishop);
                else if(c == 'R') set_at(row, col, ChessPieces::WhiteRook);
                else if(c == 'Q') set_at(row, col, ChessPieces::WhiteQueen);
                else if(c == 'K') set_at(row, col, ChessPieces::WhiteKing);
                col++;
            }
        }

        _turn = fields[1][0];
        _castling_rights = 0;
        for(auto &c : fields[2]) {
            int shift;
            if(c == 'K') shift = 0;
            else if(c == 'Q') shift = 1;
            else if(c == 'k') shift = 2;
            else if(c == 'q') shift = 3;
            _castling_rights |= (1 << shift);
        }

        if(fields[3].length() == 2) {
            _en_passant_target = {fields[3][0], fields[3][1]};
        }
        _halfmoves = stoi(fields[4]);
        _fullmoves = stoi(fields[5]);
    }

    int calculate_material() {
        int total = 0;
        for(int i = 0; i < 12; i++) {
            int bitcount = 0; // Count the number of bits in the bitboard
            uint64_t bitboard = _bitboards[i];
            while(bitboard) {
                bitboard &= (bitboard-1);
                bitcount++;
            }
            total += piece_weights[i] * bitcount;
        }
        return total;
    }

    std::string generate_fen() {
        std::string fen = "";
        for(int row = 7; row >= 0; row--) {
            int counter = 0;
            for(int col = 0; col < 8; col++) {
                uint8_t piece = get_at(row, col);
                if(piece < 12) {
                    if(counter) {
                        fen += counter + '0';
                        counter = 0;
                    }
                    if(piece == ChessPieces::BlackPawn) fen += 'p';
                    else if(piece == ChessPieces::BlackKnight) fen += 'n';
                    else if(piece == ChessPieces::BlackBishop) fen += 'b';
                    else if(piece == ChessPieces::BlackRook) fen += 'r';
                    else if(piece == ChessPieces::BlackQueen) fen += 'q';
                    else if(piece == ChessPieces::BlackKing) fen += 'k';
                    
                    else if(piece == ChessPieces::WhitePawn) fen += 'P';
                    else if(piece == ChessPieces::WhiteKnight) fen += 'N';
                    else if(piece == ChessPieces::WhiteBishop) fen += 'B';
                    else if(piece == ChessPieces::WhiteRook) fen += 'R';
                    else if(piece == ChessPieces::WhiteQueen) fen += 'Q';
                    else if(piece == ChessPieces::WhiteKing) fen += 'K';
                }
                else {
                    counter += 1;
                }
            }
            if(counter) fen += counter + '0';
            if(row) fen += '/';
        }
        fen += " ";
        fen += static_cast<char>(_turn);

        std::string castling_rights = "";
        
        if(_castling_rights & 1) castling_rights += 'K';
        if((_castling_rights >> 1) & 1) castling_rights += 'Q';
        if((_castling_rights >> 2) & 1) castling_rights += 'k';
        if((_castling_rights >> 3) & 1) castling_rights += 'q';
        if(castling_rights.length() == 0) castling_rights = "-";
        fen += " " + castling_rights;
        
        if(_en_passant_target.file && _en_passant_target.rank) {
            fen += " ";
            fen += _en_passant_target.file;
            fen += _en_passant_target.rank;
        }
        else {
            fen += " -";
        }

        fen += " ";
        fen += std::to_string(_halfmoves);
        fen += " ";
        fen += std::to_string(_fullmoves);
        return fen;
    }

    uint8_t get_at(int row, int col) {
        uint8_t piece = 0;
        for(piece; piece < 12; piece++) {
            if((_bitboards[piece] >> (row * 8 + col)) & 1ULL) return piece;
        }
        return piece;
    }

    inline void set_at(int row, int col, int piece) {
        if(piece < 6) {
            _bitboards[ChessPieces::White] |= (1ULL << (row * 8 + col));
        }
        else {
            _bitboards[ChessPieces::Black] |= (1ULL << (row * 8 + col));
        }
        _bitboards[piece] |= (1ULL << (row * 8 + col));
    }

    uint8_t get_at_position(ChessPosition pos) {
        return get_at(pos.rank-'1', pos.file-'a');
    }

    inline void set_at_position(ChessPosition pos, int piece) {
        set_at(pos.rank-'1', pos.file-'a', piece);
    }

    void print() {
        if(_turn == 'w') std::cout << "White's turn.\n";
        if(_turn == 'b') std::cout << "Black's turn.\n";
        std::string ranks = "87654321";
        std::string files = "abcdefgh";
        for(auto &r : ranks) {
            for(auto &f : files) {
                unsigned piece = get_at_position({f, r});
                std::string icon = " ";
                if(piece == ChessPieces::WhitePawn) {
                    icon = "\u2659";
                }
                else if(piece == ChessPieces::WhiteRook) {
                    icon = "\u2656";
                }
                else if(piece == ChessPieces::WhiteKnight) {
                    icon = "\u2658";
                }
                else if(piece == ChessPieces::WhiteBishop) {
                    icon = "\u2657";
                }
                else if(piece == ChessPieces::WhiteQueen) {
                    icon = "\u2655";
                }
                else if(piece == ChessPieces::WhiteKing) {
                    icon = "\u2654";
                }
                
                else if(piece == ChessPieces::BlackPawn) {
                    icon = "\u265F";
                }
                else if(piece == ChessPieces::BlackRook) {
                    icon = "\u265C";
                }
                else if(piece == ChessPieces::BlackKnight) {
                    icon = "\u265E";
                }
                else if(piece == ChessPieces::BlackBishop) {
                    icon = "\u265D";
                }
                else if(piece == ChessPieces::BlackQueen) {
                    icon = "\u265B";
                }
                else if(piece == ChessPieces::BlackKing) {
                    icon = "\u265A";
                }
                std::cout << icon << " ";
            }
            std::cout << "\n";
        }
    }
};
