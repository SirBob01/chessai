#include "board.h"

namespace chess {
    ChessPosition::ChessPosition(char file, char rank) {
        int row = rank - '1';
        int col = file - 'a';
        shift = row * 8 + col;
    }

    std::string ChessPosition::standard_notation() {
        int row = shift / 8;
        int col = shift % 8;
        char rank = row + '1';
        char field = col + 'a';
        return std::string({field, rank});
    }

    ChessBoard::ChessBoard(std::string fen_string) {
        std::vector<std::string> fields = chess::util::tokenize(fen_string, ' ');
        
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
                ChessPosition pos = ChessPosition(row * 8 + col);
                if(c == 'p') set_at(pos, ChessPiece::BlackPawn);
                else if(c == 'n') set_at(pos, ChessPiece::BlackKnight);
                else if(c == 'b') set_at(pos, ChessPiece::BlackBishop);
                else if(c == 'r') set_at(pos, ChessPiece::BlackRook);
                else if(c == 'q') set_at(pos, ChessPiece::BlackQueen);
                else if(c == 'k') set_at(pos, ChessPiece::BlackKing);

                else if(c == 'P') set_at(pos, ChessPiece::WhitePawn);
                else if(c == 'N') set_at(pos, ChessPiece::WhiteKnight);
                else if(c == 'B') set_at(pos, ChessPiece::WhiteBishop);
                else if(c == 'R') set_at(pos, ChessPiece::WhiteRook);
                else if(c == 'Q') set_at(pos, ChessPiece::WhiteQueen);
                else if(c == 'K') set_at(pos, ChessPiece::WhiteKing);
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
            _en_passant_target = ChessPosition(fields[3][0], fields[3][1]);
        }
        _halfmoves = stoi(fields[4]);
        _fullmoves = stoi(fields[5]);
    }

    std::string ChessBoard::generate_fen() {
        std::string fen = "";
        for(int row = 7; row >= 0; row--) {
            int counter = 0;
            for(int col = 0; col < 8; col++) {
                uint8_t piece = get_at_coords(row, col);
                if(piece < 12) {
                    if(counter) {
                        fen += counter + '0';
                        counter = 0;
                    }
                    if(piece == ChessPiece::BlackPawn) fen += 'p';
                    else if(piece == ChessPiece::BlackKnight) fen += 'n';
                    else if(piece == ChessPiece::BlackBishop) fen += 'b';
                    else if(piece == ChessPiece::BlackRook) fen += 'r';
                    else if(piece == ChessPiece::BlackQueen) fen += 'q';
                    else if(piece == ChessPiece::BlackKing) fen += 'k';
                    
                    else if(piece == ChessPiece::WhitePawn) fen += 'P';
                    else if(piece == ChessPiece::WhiteKnight) fen += 'N';
                    else if(piece == ChessPiece::WhiteBishop) fen += 'B';
                    else if(piece == ChessPiece::WhiteRook) fen += 'R';
                    else if(piece == ChessPiece::WhiteQueen) fen += 'Q';
                    else if(piece == ChessPiece::WhiteKing) fen += 'K';
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
        
        if(_en_passant_target.shift == -1) {
            fen += " -";
        }
        else {
            fen += " ";
            fen += _en_passant_target.standard_notation();
        }

        fen += " ";
        fen += std::to_string(_halfmoves);
        fen += " ";
        fen += std::to_string(_fullmoves);
        return fen;
    }

    int ChessBoard::calculate_material() {
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
    
    ChessPiece ChessBoard::get_at(ChessPosition pos) {
        uint8_t piece = 0;
        for(piece; piece < 12; piece++) {
            if((_bitboards[piece] >> pos.shift) & 1ULL) return static_cast<ChessPiece>(piece);
        }
        return ChessPiece::Empty;
    }

    void ChessBoard::set_at(ChessPosition pos, ChessPiece piece) {
        clear_at(pos);
        uint64_t mask = 1ULL << pos.shift;
        if(piece < 6) {
            _bitboards[ChessPiece::White] |= mask;
        }
        else {
            _bitboards[ChessPiece::Black] |= mask;
        }
        _bitboards[piece] |= mask;
    }

    void ChessBoard::clear_at(ChessPosition pos) {
        // Clear both white and black bitboards
        uint64_t mask = ~(1ULL << pos.shift);
        _bitboards[12] &= mask;
        _bitboards[13] &= mask;

        uint8_t piece = 0;
        for(piece; piece < 14; piece++) {
            if((_bitboards[piece] >> pos.shift) & 1ULL) {
                _bitboards[piece] &= mask;
                break;
            }
        }
    }
    
    void ChessBoard::set_at_coords(int row, int col, ChessPiece piece) {
        set_at({row * 8 + col}, piece);
    }

    ChessPiece ChessBoard::get_at_coords(int row, int col) {
        return get_at({row * 8 + col});
    }

    void ChessBoard::execute_move(ChessMove move) {
        // TODO: Deal with castling
        _halfmoves++;
        ChessPiece piece = get_at(move.from);
        ChessPiece target = get_at(move.to);

        // Move to target square and handle promotions
        clear_at(move.from);
        if(move.flags & ChessMoveFlag::BishopPromo) {
            if(_turn == 'w') set_at(move.to, ChessPiece::WhiteBishop);
            else set_at(move.to, ChessPiece::BlackBishop);
        }
        else if(move.flags & ChessMoveFlag::RookPromo) {
            if(_turn == 'w') set_at(move.to, ChessPiece::WhiteRook);
            else set_at(move.to, ChessPiece::BlackRook);
        }
        else if(move.flags & ChessMoveFlag::KnightPromo) {
            if(_turn == 'w') set_at(move.to, ChessPiece::WhiteKnight);
            else set_at(move.to, ChessPiece::BlackKnight);
        }
        else if(move.flags & ChessMoveFlag::QueenPromo) {
            if(_turn == 'w') set_at(move.to, ChessPiece::WhiteQueen);
            else set_at(move.to, ChessPiece::BlackQueen);
        }
        else {
            set_at(move.to, piece);
        }

        // Check for en passant capture
        if(move.flags & ChessMoveFlag::EnPassant) {
            // Clear the square of the captured pawn
            int rankd = move.to.shift - move.from.shift;
            int dir = (rankd > 0) - (rankd < 0);
            clear_at(ChessPosition(_en_passant_target.shift - (dir * 8)));
            _en_passant_target = ChessPosition();
        }

        // Update en passant position if pawn advanced two ranks
        if(move.flags & ChessMoveFlag::PawnDouble) {
            _en_passant_target = ChessPosition(move.from.shift + (move.to.shift - move.from.shift)/2);
        }
        else {
            _en_passant_target = ChessPosition();
        }

        // Reset halfmove counter if piece was pawn advance or move was a capture
        if(move.flags & (ChessMoveFlag::PawnAdvance | 
                         ChessMoveFlag::PawnDouble  | 
                         ChessMoveFlag::EnPassant   | 
                         ChessMoveFlag::Capture)) {
            _halfmoves = 0;
        }

        // Update turn and fullmove counter
        if(_turn == 'b') {
            _fullmoves++;
            _turn = 'w';
        }
        else {
            _turn = 'b';
        }
    }

    void ChessBoard::generate_pawn_moves(uint64_t bitboard, std::vector<ChessMove> &moves) {
        uint64_t final_ranks = 0xFF000000000000FF;
        uint64_t all_pieces = _bitboards[ChessPiece::White] | _bitboards[ChessPiece::Black];
        uint64_t opposite_color, en_passant_mask = 0;
        if(_en_passant_target.shift != -1) {
            en_passant_mask = 1ULL << _en_passant_target.shift;
        }
        if(_turn == 'w') {
            opposite_color = _bitboards[ChessPiece::Black];
        }
        else {
            // If it's black's turn, flip all auxiliary bitboards
            opposite_color = flip_vertical(_bitboards[ChessPiece::White]);
            all_pieces = flip_vertical(all_pieces);
            en_passant_mask = flip_vertical(en_passant_mask);
        }

        while(bitboard) {
            uint64_t piece = bitboard & (-bitboard);
            ChessPosition from = ChessPosition(find_lsb(piece));
            if(_turn == 'b') piece = flip_vertical(piece);

            uint64_t advance = get_pawn_advance_mask(piece, all_pieces);
            if(_turn == 'b') advance = flip_vertical(advance); // Unflip final moves if it's black's turn
            while(advance) {
                uint64_t move = advance & (-advance);
                unsigned flags = ChessMoveFlag::Quiet | ChessMoveFlag::PawnAdvance;
                ChessPosition to(find_lsb(move));
                if(move & final_ranks) {
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::QueenPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::KnightPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::RookPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::BishopPromo
                    });
                }
                else {
                    moves.push_back({
                        from, to, flags
                    });
                }
                advance &= (advance - 1);
            }

            uint64_t double_advance = get_pawn_double_mask(piece, all_pieces);
            if(_turn == 'b') double_advance = flip_vertical(double_advance);
            while(double_advance) {
                uint64_t move = double_advance & (-double_advance);
                unsigned flags = ChessMoveFlag::Quiet | ChessMoveFlag::PawnDouble;
                ChessPosition to(find_lsb(move));
                if(move & final_ranks) {
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::QueenPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::KnightPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::RookPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::BishopPromo
                    });
                }
                else {
                    moves.push_back({
                        from, to, flags
                    });
                }
                double_advance &= (double_advance - 1);
            }
            
            uint64_t capture = get_pawn_capture_mask(piece, opposite_color);
            if(_turn == 'b') capture = flip_vertical(capture);
            while(capture) {
                uint64_t move = capture & (-capture);
                unsigned flags = ChessMoveFlag::Capture;
                ChessPosition to(find_lsb(move));
                if(move & final_ranks) {
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::QueenPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::KnightPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::RookPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::BishopPromo
                    });
                }
                else {
                    moves.push_back({
                        from, to, flags
                    });
                }
                capture &= (capture - 1);
            }

            uint64_t en_passant = get_pawn_en_passant_mask(piece, en_passant_mask);
            if(_turn == 'b') en_passant = flip_vertical(en_passant);
            while(en_passant) {
                uint64_t move = en_passant & (-en_passant);
                unsigned flags = ChessMoveFlag::Capture | ChessMoveFlag::EnPassant;
                ChessPosition to(find_lsb(move));
                if(move & final_ranks) {
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::QueenPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::KnightPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::RookPromo
                    });
                    moves.push_back({
                        from, to, flags | ChessMoveFlag::BishopPromo
                    });
                }
                else {
                    moves.push_back({
                        from, to, flags
                    });
                }
                en_passant &= (en_passant - 1);
            }
            bitboard &= (bitboard - 1);
        }
    }

    std::vector<ChessMove> ChessBoard::generate_move_list() {
        std::vector<ChessMove> moves;
        int start;
        if(_turn == 'w') {
            start = 0;
        }
        else {
            start = 6;
        }
        for(int i = start; i < start + 6; i++) {
            uint64_t bitboard = _bitboards[i];
            switch(i % 6) {
                case 0:
                    generate_pawn_moves(bitboard, moves);
                    break;
                default:
                    break;
            }
        }
        return moves;
    }


    ChessMove ChessBoard::create_move(ChessPosition from, ChessPosition to) {
        std::vector<ChessMove> valid_moves = generate_move_list();
        for(auto &move : valid_moves) {
            if(move.from.shift == from.shift && move.to.shift == to.shift) {
                return move;
            }
        }
        return {
            ChessPosition(), 
            ChessPosition(), 
            ChessMoveFlag::Invalid
        };
    }

    void ChessBoard::print() {
        if(_turn == 'w') std::cout << "White's turn.\n";
        if(_turn == 'b') std::cout << "Black's turn.\n";
        for(int rank = 0; rank < 8; rank++) {
            for(int file = 0; file < 8; file++) {
                unsigned piece = get_at_coords(rank, file);
                std::string icon = "-";
                if(piece == ChessPiece::WhitePawn) {
                    icon = "\u2659";
                }
                else if(piece == ChessPiece::WhiteRook) {
                    icon = "\u2656";
                }
                else if(piece == ChessPiece::WhiteKnight) {
                    icon = "\u2658";
                }
                else if(piece == ChessPiece::WhiteBishop) {
                    icon = "\u2657";
                }
                else if(piece == ChessPiece::WhiteQueen) {
                    icon = "\u2655";
                }
                else if(piece == ChessPiece::WhiteKing) {
                    icon = "\u2654";
                }
                
                else if(piece == ChessPiece::BlackPawn) {
                    icon = "\u265F";
                }
                else if(piece == ChessPiece::BlackRook) {
                    icon = "\u265C";
                }
                else if(piece == ChessPiece::BlackKnight) {
                    icon = "\u265E";
                }
                else if(piece == ChessPiece::BlackBishop) {
                    icon = "\u265D";
                }
                else if(piece == ChessPiece::BlackQueen) {
                    icon = "\u265B";
                }
                else if(piece == ChessPiece::BlackKing) {
                    icon = "\u265A";
                }
                std::cout << icon << " ";
            }
            std::cout << "\n";
        }
    }
}