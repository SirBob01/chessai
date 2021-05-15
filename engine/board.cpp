#include "board.h"

namespace chess {
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
                set_at(pos, static_cast<Piece>(PieceChars.find(c)));
                col++;
            }
        }

        _turn = fields[1][0];
        _castling_rights = 0;
        for(auto &c : fields[2]) {
            if(c == 'K')      _castling_rights |= Castle::KingWhite;
            else if(c == 'Q') _castling_rights |= Castle::QueenWhite;
            else if(c == 'k') _castling_rights |= Castle::KingBlack;
            else if(c == 'q') _castling_rights |= Castle::QueenBlack;
        }

        if(fields[3].length() == 2) {
            _en_passant_target = Position(fields[3][0], fields[3][1]);
        }
        _halfmoves = stoi(fields[4]);
        _fullmoves = stoi(fields[5]);
        
        _attackers = get_attackers();
        generate_moves();
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
                    fen += PieceChars[piece];
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

    void Board::generate_piece_moves(uint64_t bitboard, uint64_t(*mask_func)(uint64_t, uint64_t)) {
        uint64_t same_color, opposite_color;
        if(_turn == 'w') {
            same_color = _bitboards[Piece::White];
            opposite_color = _bitboards[Piece::Black];
        }
        else {
            same_color = _bitboards[Piece::Black];
            opposite_color = _bitboards[Piece::White];
        }
        uint64_t empty_mask = ~(opposite_color | same_color);

        while(bitboard) {
            uint64_t piece = bitboard & (-bitboard);
            Position from = Position(find_lsb(piece));
            
            uint64_t move_bits = mask_func(piece, same_color);
            uint64_t capture = move_bits & opposite_color;
            uint64_t advance = move_bits & empty_mask;

            while(advance) {
                uint64_t move = advance &(-advance);
                Position to = Position(find_lsb(move));
                register_move({from, to, MoveFlag::Quiet});
                advance &= (advance - 1);
            }

            while(capture) {
                uint64_t move = capture & (-capture);
                Position to = Position(find_lsb(move));
                register_move({from, to, MoveFlag::Capture});
                capture &= (capture - 1);
            }
            bitboard &= (bitboard - 1);
        }
    }

    void Board::generate_slider_moves(uint64_t bitboard, uint64_t(*mask_func)(uint64_t, uint64_t, uint64_t)) {
        uint64_t same_color, opposite_color;
        if(_turn == 'w') {
            same_color = _bitboards[Piece::White];
            opposite_color = _bitboards[Piece::Black];
        }
        else {
            same_color = _bitboards[Piece::Black];
            opposite_color = _bitboards[Piece::White];
        }
        uint64_t empty_mask = ~(opposite_color | same_color);

        while(bitboard) {
            uint64_t piece = bitboard & (-bitboard);
            Position from = Position(find_lsb(piece));

            uint64_t move_bits = mask_func(piece, same_color, opposite_color);
            uint64_t capture = move_bits & opposite_color;
            uint64_t advance = move_bits & empty_mask;

            while(advance) {
                uint64_t move = advance &(-advance);
                Position to = Position(find_lsb(move));
                register_move({from, to, MoveFlag::Quiet});
                advance &= (advance - 1);
            }

            while(capture) {
                uint64_t move = capture & (-capture);
                Position to = Position(find_lsb(move));
                register_move({from, to, MoveFlag::Capture});
                capture &= (capture - 1);
            }
            bitboard &= (bitboard - 1);
        }
    }

    void Board::generate_pawn_moves(uint64_t bitboard) {
        uint64_t all_pieces = _bitboards[Piece::White] | _bitboards[Piece::Black];
        uint64_t en_passant_mask = 0;
        if(_en_passant_target.shift != -1) {
            en_passant_mask = _en_passant_target.get_mask();
        }
        uint64_t opposite_color = (_turn == 'w') ? _bitboards[Piece::Black] : _bitboards[Piece::White];

        while(bitboard) {
            uint64_t piece = bitboard & (-bitboard);
            Position from(find_lsb(piece));

            uint64_t advance = get_pawn_advance_mask(piece, all_pieces, _turn);
            while(advance) {
                uint64_t move = advance & (-advance);
                unsigned flags = MoveFlag::Quiet | MoveFlag::PawnAdvance;
                Position to(find_lsb(move));
                if(move & end_ranks) {
                    register_move({from, to, flags | MoveFlag::QueenPromo});
                    register_move({from, to, flags | MoveFlag::KnightPromo});
                    register_move({from, to, flags | MoveFlag::RookPromo});
                    register_move({from, to, flags | MoveFlag::BishopPromo});
                }
                else {
                    register_move({from, to, flags});
                }
                advance &= (advance - 1);
            }

            uint64_t double_advance = get_pawn_double_mask(piece, all_pieces, _turn);
            while(double_advance) {
                uint64_t move = double_advance & (-double_advance);
                unsigned flags = MoveFlag::Quiet | MoveFlag::PawnDouble;
                Position to(find_lsb(move));
                if(move & end_ranks) {
                    register_move({from, to, flags | MoveFlag::QueenPromo});
                    register_move({from, to, flags | MoveFlag::KnightPromo});
                    register_move({from, to, flags | MoveFlag::RookPromo});
                    register_move({from, to, flags | MoveFlag::BishopPromo});
                }
                else {
                    register_move({from, to, flags});
                }
                double_advance &= (double_advance - 1);
            }
            
            uint64_t capture = get_pawn_capture_mask(piece, _turn) & opposite_color;
            while(capture) {
                uint64_t move = capture & (-capture);
                unsigned flags = MoveFlag::Capture;
                Position to(find_lsb(move));
                if(move & end_ranks) {
                    register_move({from, to, flags | MoveFlag::QueenPromo});
                    register_move({from, to, flags | MoveFlag::KnightPromo});
                    register_move({from, to, flags | MoveFlag::RookPromo});
                    register_move({from, to, flags | MoveFlag::BishopPromo});
                }
                else {
                    register_move({from, to, flags});
                }
                capture &= (capture - 1);
            }

            uint64_t en_passant = get_pawn_capture_mask(piece, _turn) & en_passant_mask;
            while(en_passant) {
                uint64_t move = en_passant & (-en_passant);
                unsigned flags = MoveFlag::Capture | MoveFlag::EnPassant;
                Position to(find_lsb(move));
                if(move & end_ranks) {
                    register_move({from, to, flags | MoveFlag::QueenPromo});
                    register_move({from, to, flags | MoveFlag::KnightPromo});
                    register_move({from, to, flags | MoveFlag::RookPromo});
                    register_move({from, to, flags | MoveFlag::BishopPromo});
                }
                else {
                    register_move({from, to, flags});
                }
                en_passant &= (en_passant - 1);
            }
            bitboard &= (bitboard - 1);
        }
    }

    void Board::generate_castling_moves(uint64_t bitboard) {
        uint64_t all_pieces = _bitboards[Piece::White] | _bitboards[Piece::Black];
        uint8_t rights;
        if(_turn == 'w') {
            rights = _castling_rights & (Castle::KingWhite | Castle::QueenWhite);
        }
        else {
            rights = _castling_rights & (Castle::KingBlack | Castle::QueenBlack);
        }
        Position from(find_lsb(bitboard));
        while(rights) {
            uint8_t side = rights & (-rights);
            
            uint64_t mask = get_castling_mask(all_pieces, side);
            if(mask) {
                int diff = find_lsb(mask) - from.shift;
                Position to(find_lsb(mask));
                register_move({from, to, MoveFlag::Quiet | MoveFlag::Castle});
            }
            rights &= (rights-1);
        }
    }

    bool Board::is_legal(Move move) {
        uint64_t king = (_turn == 'w') ? _bitboards[Piece::WhiteKing] : _bitboards[Piece::BlackKing];
        uint64_t from = move.from.get_mask();
        uint64_t to = move.to.get_mask();
        
        // Pawn cannot perform an En Passant if the king is in check
        if((move.flags & MoveFlag::EnPassant) && (_attackers & king)) {
            return false;
        }

        // King cannot castle if it is in check or if it has to pass through an attacked square
        if(move.flags & MoveFlag::Castle) {
            int rankd = move.to.shift - move.from.shift;
            int dir = (rankd > 0) - (rankd < 0);
            Position pass_through(move.to.shift - dir);
            if((_attackers & king) || (_attackers & pass_through.get_mask())) {
                return false;
            }
        }
        
        // If king is the moving piece, ensure destination is not an attacked square
        if((king & from) && (_attackers & to)) {
            return false;
        }

        // Non-king piece is moving, make sure it either isn't pinned
        // or it target destination still blocks king from attacker
        if(!is_aligned(from, to, king) &&
            is_king_pinned(move.from)) {
            return false;
        }
        return true;
    }

    void Board::register_move(Move move) {
        if(is_legal(move)) {
            _legal_moves.push_back(move);
        }
    }

    void Board::generate_moves() {
        int start = (_turn == 'w') ? 0 : 6;
        for(int i = start; i < start + 6; i++) {
            uint64_t bitboard = _bitboards[i];
            switch(i % 6) {
                // Relative index ordering in bitboard array is the same
                // for both white and black pieces
                case Piece::WhitePawn:
                    generate_pawn_moves(bitboard);
                    break;
                case Piece::WhiteKnight:
                    generate_piece_moves(bitboard, get_knight_mask);
                    break;
                case Piece::WhiteKing:
                    generate_piece_moves(bitboard, get_king_mask);
                    generate_castling_moves(bitboard);
                    break;
                case Piece::WhiteBishop:
                    generate_slider_moves(bitboard, get_bishop_mask);
                    break;
                case Piece::WhiteRook:
                    generate_slider_moves(bitboard, get_rook_mask);
                    break;
                case Piece::WhiteQueen:
                    generate_slider_moves(bitboard, get_queen_mask);
                    break;
                default:
                    break;
            }
        }
    }

    bool Board::is_king_pinned(Position pos) {
        uint64_t mask = ~pos.get_mask();
        uint64_t same_color, opposite_color, queens, bishops, rooks, king;
        // Test? Remove bit from the occluder list and test
        // if any of the opposing sliding pieces can attack the king
        // Test queen, bishop, and rook
        if(_turn == 'w') {
            opposite_color = _bitboards[Piece::White] & mask;
            king = _bitboards[Piece::WhiteKing];
            same_color = _bitboards[Piece::Black];
            queens = _bitboards[Piece::BlackQueen];
            rooks = _bitboards[Piece::BlackRook];
            bishops = _bitboards[Piece::BlackBishop];
        }
        else {
            opposite_color = _bitboards[Piece::Black] & mask;
            king = _bitboards[Piece::BlackKing];
            same_color = _bitboards[Piece::White];
            queens = _bitboards[Piece::WhiteQueen];
            rooks = _bitboards[Piece::WhiteRook];
            bishops = _bitboards[Piece::WhiteBishop];
        }

        uint64_t attacks = 0;
        while(queens) {
            uint64_t piece = queens & (-queens);
            attacks |= get_queen_mask(piece, same_color, opposite_color);
            queens &= (queens-1);
        }
        while(rooks) {
            uint64_t piece = rooks & (-rooks);
            attacks |= get_rook_mask(piece, same_color, opposite_color);
            rooks &= (rooks-1);
        }
        while(bishops) {
            uint64_t piece = bishops & (-bishops);
            attacks |= get_bishop_mask(piece, same_color, opposite_color);
            bishops &= (bishops-1);
        }
        return king & attacks;
    }

    uint64_t Board::get_attackers() {
        int start;
        uint64_t same_color, opposite_color;
        // Generate attack vectors for targets
        // Exclude the king from the target list for case when king is still aligned
        // with sliding piece, but further moves away from the attack vector
        if(_turn == 'w') {
            start = 6;
            same_color = _bitboards[Piece::Black];
            opposite_color = _bitboards[Piece::White] & ~_bitboards[Piece::WhiteKing];
        }
        else {
            start = 0;
            same_color = _bitboards[Piece::White];
            opposite_color = _bitboards[Piece::Black] & ~_bitboards[Piece::BlackKing];
        }
        uint64_t attack_board = 0;
        for(int i = start; i < start + 6; i++) {
            uint64_t bitboard = _bitboards[i];
            int piece = i % 6;
            // Single move pieces can be bit-parallelized
            if(piece == Piece::WhitePawn) {
                if(_turn == 'w') {
                    attack_board |= get_pawn_capture_mask(bitboard, 'b');
                }
                else {
                    attack_board |= get_pawn_capture_mask(bitboard, 'w');
                }
            }
            else if(piece == Piece::WhiteKnight) {
                attack_board |= get_knight_mask(bitboard, same_color);
            }
            else if(piece == Piece::WhiteKing) {
                attack_board |= get_king_mask(bitboard, same_color);
            }
            else {
                // Sliding pieces must be handled individually
                while(bitboard) {
                    uint64_t unit = bitboard & (-bitboard);
                    switch(piece) {
                        case Piece::WhiteBishop:
                            attack_board |= get_bishop_mask(unit, same_color, opposite_color);
                            break;
                        case Piece::WhiteRook:
                            attack_board |= get_rook_mask(unit, same_color, opposite_color);
                            break;
                        case Piece::WhiteQueen:
                            attack_board |= get_queen_mask(unit, same_color, opposite_color);
                            break;
                        default:
                            break;
                    }
                    bitboard &= (bitboard-1);
                }
            }
        }
        return attack_board;
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
        uint64_t mask = pos.get_mask();
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
        uint64_t mask = ~(pos.get_mask());
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
        _halfmoves++;
        Piece piece = get_at(move.from);
        Piece target = get_at(move.to);

        // Unset castling flags if relevant pieces were moved
        if(_castling_rights & (Castle::KingWhite | Castle::QueenWhite)) {
            if(piece == Piece::WhiteKing) {
                _castling_rights ^= (Castle::KingWhite | Castle::QueenWhite);
            }
            else if(piece == Piece::WhiteRook) {
                uint64_t mask = move.from.get_mask();
                if(mask & fileA) _castling_rights ^= Castle::QueenWhite;
                else if(mask & fileH) _castling_rights ^= Castle::KingWhite;
            }
        }
        if(_castling_rights & (Castle::KingBlack | Castle::QueenBlack)) {
            if(piece == Piece::BlackKing) {
                _castling_rights ^= (Castle::KingBlack | Castle::QueenBlack);
            }
            else if(piece == Piece::BlackRook) {
                uint64_t mask = move.from.get_mask();
                if(mask & fileA) _castling_rights ^= Castle::QueenBlack;
                else if(mask & fileH) _castling_rights ^= Castle::KingBlack;
            }
        }

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

        // Move rook if castling
        if(move.flags & MoveFlag::Castle) {
            int rankd = move.to.shift - move.from.shift;
            int dir = (rankd > 0) - (rankd < 0);
            Piece rook_type = (_turn == 'w') ? Piece::WhiteRook : Piece::BlackRook;
            uint64_t rook = _bitboards[rook_type];
            Position target(move.to.shift - dir);
            if(rankd < 0) {
                rook &= fileA;
            }
            else {
                rook &= fileH;
            }
            clear_at(Position(find_lsb(rook)));
            set_at(target, rook_type);
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

    int Board::get_halfmoves() {
        return _halfmoves;
    }

    char Board::get_turn() {
        return _turn;
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
                if(piece != Piece::Empty) {
                    icon = PieceDisplay[piece];
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