#include "board.h"

namespace chess {
    Position::Position(char file, char rank) {
        int row = rank - '1';
        int col = file - 'a';
        shift = row * 8 + col;
    }

    std::string Position::standard_notation() {
        int row = shift / 8;
        int col = shift % 8;
        char rank = row + '1';
        char field = col + 'a';
        return std::string({field, rank});
    }

    Board::Board(std::string fen_string) {
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
                Position pos = Position(row * 8 + col);
                if(c == 'p') set_at(pos, Piece::BlackPawn);
                else if(c == 'n') set_at(pos, Piece::BlackKnight);
                else if(c == 'b') set_at(pos, Piece::BlackBishop);
                else if(c == 'r') set_at(pos, Piece::BlackRook);
                else if(c == 'q') set_at(pos, Piece::BlackQueen);
                else if(c == 'k') set_at(pos, Piece::BlackKing);

                else if(c == 'P') set_at(pos, Piece::WhitePawn);
                else if(c == 'N') set_at(pos, Piece::WhiteKnight);
                else if(c == 'B') set_at(pos, Piece::WhiteBishop);
                else if(c == 'R') set_at(pos, Piece::WhiteRook);
                else if(c == 'Q') set_at(pos, Piece::WhiteQueen);
                else if(c == 'K') set_at(pos, Piece::WhiteKing);
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
            _en_passant_target = Position(fields[3][0], fields[3][1]);
        }
        _halfmoves = stoi(fields[4]);
        _fullmoves = stoi(fields[5]);
    }

    std::string Board::generate_fen() {
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
                    if(piece == Piece::BlackPawn) fen += 'p';
                    else if(piece == Piece::BlackKnight) fen += 'n';
                    else if(piece == Piece::BlackBishop) fen += 'b';
                    else if(piece == Piece::BlackRook) fen += 'r';
                    else if(piece == Piece::BlackQueen) fen += 'q';
                    else if(piece == Piece::BlackKing) fen += 'k';
                    
                    else if(piece == Piece::WhitePawn) fen += 'P';
                    else if(piece == Piece::WhiteKnight) fen += 'N';
                    else if(piece == Piece::WhiteBishop) fen += 'B';
                    else if(piece == Piece::WhiteRook) fen += 'R';
                    else if(piece == Piece::WhiteQueen) fen += 'Q';
                    else if(piece == Piece::WhiteKing) fen += 'K';
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

    void Board::generate_pawn_moves(uint64_t bitboard, std::vector<Move> &moves) {
        uint64_t final_ranks = 0xFF000000000000FF;
        uint64_t all_pieces = _bitboards[Piece::White] | _bitboards[Piece::Black];
        uint64_t opposite_color, en_passant_mask = 0;
        if(_en_passant_target.shift != -1) {
            en_passant_mask = 1ULL << _en_passant_target.shift;
        }
        if(_turn == 'w') {
            opposite_color = _bitboards[Piece::Black];
        }
        else {
            // If it's black's turn, flip all auxiliary bitboards
            opposite_color = flip_vertical(_bitboards[Piece::White]);
            all_pieces = flip_vertical(all_pieces);
            en_passant_mask = flip_vertical(en_passant_mask);
        }

        while(bitboard) {
            uint64_t piece = bitboard & (-bitboard);
            Position from = Position(find_lsb(piece));
            if(_turn == 'b') piece = flip_vertical(piece);

            uint64_t advance = get_pawn_advance_mask(piece, all_pieces);
            if(_turn == 'b') advance = flip_vertical(advance); // Unflip final moves if it's black's turn
            while(advance) {
                uint64_t move = advance & (-advance);
                unsigned flags = MoveFlag::Quiet | MoveFlag::PawnAdvance;
                Position to(find_lsb(move));
                if(move & final_ranks) {
                    moves.push_back({
                        from, to, flags | MoveFlag::QueenPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::KnightPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::RookPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::BishopPromo
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
                unsigned flags = MoveFlag::Quiet | MoveFlag::PawnDouble;
                Position to(find_lsb(move));
                if(move & final_ranks) {
                    moves.push_back({
                        from, to, flags | MoveFlag::QueenPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::KnightPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::RookPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::BishopPromo
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
                unsigned flags = MoveFlag::Capture;
                Position to(find_lsb(move));
                if(move & final_ranks) {
                    moves.push_back({
                        from, to, flags | MoveFlag::QueenPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::KnightPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::RookPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::BishopPromo
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
                unsigned flags = MoveFlag::Capture | MoveFlag::EnPassant;
                Position to(find_lsb(move));
                if(move & final_ranks) {
                    moves.push_back({
                        from, to, flags | MoveFlag::QueenPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::KnightPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::RookPromo
                    });
                    moves.push_back({
                        from, to, flags | MoveFlag::BishopPromo
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

    void Board::generate_knight_moves(uint64_t bitboard, std::vector<Move> &moves) {
        uint64_t same_color, opposite_color;
        if(_turn == 'w') {
            same_color = _bitboards[Piece::White];
            opposite_color = _bitboards[Piece::Black];
        }
        else {
            same_color = flip_vertical(_bitboards[Piece::Black]);
            opposite_color = flip_vertical(_bitboards[Piece::White]);
        }

        while(bitboard) {
            uint64_t piece = bitboard & (-bitboard);
            Position from = Position(find_lsb(piece));
            if(_turn == 'b') piece = flip_vertical(piece);
            
            uint64_t move_bits = get_knight_mask(piece, same_color);
            uint64_t capture = move_bits & opposite_color;
            uint64_t advance = move_bits & ~(opposite_color | same_color);

            if(_turn == 'b') {
                capture = flip_vertical(capture);
                advance = flip_vertical(advance);
            }

            while(advance) {
                uint64_t move = advance &(-advance);
                Position to = Position(find_lsb(move));
                moves.push_back({from, to, MoveFlag::Quiet});
                advance &= (advance - 1);
            }

            while(capture) {
                uint64_t move = capture & (-capture);
                Position to = Position(find_lsb(move));
                moves.push_back({from, to, MoveFlag::Capture});
                capture &= (capture - 1);
            }
            bitboard &= (bitboard - 1);
        }
    }

    void Board::generate_king_moves(uint64_t bitboard, std::vector<Move> &moves) {
        uint64_t same_color, opposite_color;
        if(_turn == 'w') {
            same_color = _bitboards[Piece::White];
            opposite_color = _bitboards[Piece::Black];
        }
        else {
            same_color = flip_vertical(_bitboards[Piece::Black]);
            opposite_color = flip_vertical(_bitboards[Piece::White]);
        }

        while(bitboard) {
            uint64_t piece = bitboard & (-bitboard);
            Position from = Position(find_lsb(piece));
            if(_turn == 'b') piece = flip_vertical(piece);
            
            uint64_t move_bits = get_king_mask(piece, same_color);
            uint64_t capture = move_bits & opposite_color;
            uint64_t advance = move_bits & ~(opposite_color | same_color);

            if(_turn == 'b') {
                capture = flip_vertical(capture);
                advance = flip_vertical(advance);
            }

            while(advance) {
                uint64_t move = advance &(-advance);
                Position to = Position(find_lsb(move));
                moves.push_back({from, to, MoveFlag::Quiet});
                advance &= (advance - 1);
            }

            while(capture) {
                uint64_t move = capture & (-capture);
                Position to = Position(find_lsb(move));
                moves.push_back({from, to, MoveFlag::Capture});
                capture &= (capture - 1);
            }
            bitboard &= (bitboard - 1);
        }
    }

    std::vector<Move> Board::generate_pseudo_legal_moves() {
        int start;
        if(_turn == 'w') {
            start = 0;
        }
        else {
            start = 6;
        }
        std::vector<Move> pseudo_legal;
        for(int i = start; i < start + 6; i++) {
            uint64_t bitboard = _bitboards[i];
            switch(i % 6) {
                // Relative index ordering in bitboard array is the same
                // for both white and black pieces
                case Piece::WhitePawn:
                    generate_pawn_moves(bitboard, pseudo_legal);
                    break;
                case Piece::WhiteKnight:
                    generate_knight_moves(bitboard, pseudo_legal);
                    break;
                case Piece::WhiteKing:
                    generate_king_moves(bitboard, pseudo_legal);
                    break;
                default:
                    break;
            }
        }
        return pseudo_legal;
    }

    bool Board::is_in_check() {
        // Did the opposing turn (previous move) leave itself in check?
        // Generate pseudo legal moves and test against opposing king
        std::vector<Move> pseudo_legal = generate_pseudo_legal_moves();
        uint64_t target_king;
        if(_turn == 'b') target_king = _bitboards[Piece::WhiteKing];
        else target_king = _bitboards[Piece::BlackKing];

        for(auto &move : pseudo_legal) {
            uint64_t move_mask = 1ULL << move.to.shift;
            if((move_mask ^ target_king) == 0) return false;
        }
        return true;
    }

    void Board::generate() {
        // Filter out illegal moves by executing move in board copy
        // Move is invalid if turn puts its own king in check
        std::vector<Move> pseudo_legal = generate_pseudo_legal_moves();
        for(auto &move : pseudo_legal) {
            Board next_state = *this;
            next_state.execute_move(move);
            if(!next_state.is_in_check()) {
                continue;
            }
            _legal_moves.push_back(move);
        }
    }

    int Board::calculate_material() {
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
    
    Piece Board::get_at(Position pos) {
        uint8_t piece = 0;
        for(piece; piece < 12; piece++) {
            if((_bitboards[piece] >> pos.shift) & 1ULL) return static_cast<Piece>(piece);
        }
        return Piece::Empty;
    }

    void Board::set_at(Position pos, Piece piece) {
        clear_at(pos);
        uint64_t mask = 1ULL << pos.shift;
        if(piece < 6) {
            _bitboards[Piece::White] |= mask;
        }
        else {
            _bitboards[Piece::Black] |= mask;
        }
        _bitboards[piece] |= mask;
    }

    void Board::clear_at(Position pos) {
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
    
    void Board::set_at_coords(int row, int col, Piece piece) {
        set_at({row * 8 + col}, piece);
    }

    Piece Board::get_at_coords(int row, int col) {
        return get_at({row * 8 + col});
    }

    void Board::execute_move(Move move) {
        // TODO: Deal with castling
        _halfmoves++;
        Piece piece = get_at(move.from);
        Piece target = get_at(move.to);

        // Move to target square and handle promotions
        clear_at(move.from);
        if(move.flags & MoveFlag::BishopPromo) {
            if(_turn == 'w') set_at(move.to, Piece::WhiteBishop);
            else set_at(move.to, Piece::BlackBishop);
        }
        else if(move.flags & MoveFlag::RookPromo) {
            if(_turn == 'w') set_at(move.to, Piece::WhiteRook);
            else set_at(move.to, Piece::BlackRook);
        }
        else if(move.flags & MoveFlag::KnightPromo) {
            if(_turn == 'w') set_at(move.to, Piece::WhiteKnight);
            else set_at(move.to, Piece::BlackKnight);
        }
        else if(move.flags & MoveFlag::QueenPromo) {
            if(_turn == 'w') set_at(move.to, Piece::WhiteQueen);
            else set_at(move.to, Piece::BlackQueen);
        }
        else {
            set_at(move.to, piece);
        }

        // Check for en passant capture
        if(move.flags & MoveFlag::EnPassant) {
            // Clear the square of the captured pawn
            int rankd = move.to.shift - move.from.shift;
            
            // One rank up or one rank down depending on current player
            int dir = (rankd > 0) - (rankd < 0); 
            clear_at(Position(_en_passant_target.shift - (dir * 8)));
            _en_passant_target = Position();
        }

        // Update en passant position if pawn advanced two ranks
        if(move.flags & MoveFlag::PawnDouble) {
            _en_passant_target = Position(move.from.shift + (move.to.shift - move.from.shift)/2);
        }
        else {
            _en_passant_target = Position();
        }

        // Reset halfmove counter if piece was pawn advance or move was a capture
        if(move.flags & (MoveFlag::PawnAdvance | 
                         MoveFlag::PawnDouble  | 
                         MoveFlag::EnPassant   | 
                         MoveFlag::Capture)) {
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

    Move Board::create_move(Position from, Position to) {
        for(auto &move : _legal_moves) {
            if(move.from.shift == from.shift && move.to.shift == to.shift) {
                return move;
            }
        }
        return {
            Position(), 
            Position(), 
            MoveFlag::Invalid
        };
    }

    std::vector<Move> Board::get_legal_moves() {
        return _legal_moves;
    }

    void Board::print() {
        // Set code page to allow UTF16 characters to show (chcp 65001 on powershell)
        if(_turn == 'w') std::cout << "White's turn.\n";
        if(_turn == 'b') std::cout << "Black's turn.\n";
        std::string files = "ABCDEFGH";
        for(int rank = 7; rank >= 0; rank--) {
            std::cout << rank+1 << " ";
            for(int file = 0; file < 8; file++) {
                unsigned piece = get_at_coords(rank, file);
                std::string icon = "-";
                if(piece == Piece::WhitePawn) {
                    icon = "\u2659";
                }
                else if(piece == Piece::WhiteRook) {
                    icon = "\u2656";
                }
                else if(piece == Piece::WhiteKnight) {
                    icon = "\u2658";
                }
                else if(piece == Piece::WhiteBishop) {
                    icon = "\u2657";
                }
                else if(piece == Piece::WhiteQueen) {
                    icon = "\u2655";
                }
                else if(piece == Piece::WhiteKing) {
                    icon = "\u2654";
                }
                
                else if(piece == Piece::BlackPawn) {
                    icon = "\u265F";
                }
                else if(piece == Piece::BlackRook) {
                    icon = "\u265C";
                }
                else if(piece == Piece::BlackKnight) {
                    icon = "\u265E";
                }
                else if(piece == Piece::BlackBishop) {
                    icon = "\u265D";
                }
                else if(piece == Piece::BlackQueen) {
                    icon = "\u265B";
                }
                else if(piece == Piece::BlackKing) {
                    icon = "\u265A";
                }
                std::cout << icon << " ";
            }
            std::cout << "\n";
        }
        std::cout << "  ";
        for(auto &f : files) {
            std::cout << f << " ";
        }
        std::cout << "\n";
    }
}