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
                int char_idx = 0;
                while(PieceChars[char_idx] != c) {
                    char_idx++;
                }
                Square sq = Square(row * 8 + col);
                PieceType type = static_cast<PieceType>(char_idx % PieceType::NPieces);
                Color color = static_cast<Color>(char_idx / PieceType::NPieces);
                set_at(sq, {type, color});
                col++;
            }
        }

        _turn = (fields[1][0] == 'w') ? Color::White : Color::Black;
        _castling_rights = 0;
        for(auto &c : fields[2]) {
            if(c == 'K')      _castling_rights |= Castle::WK;
            else if(c == 'Q') _castling_rights |= Castle::WQ;
            else if(c == 'k') _castling_rights |= Castle::BK;
            else if(c == 'q') _castling_rights |= Castle::BQ;
        }

        if(fields[3].length() == 2) {
            _en_passant_target = Square(fields[3]);
        }
        _halfmoves = stoi(fields[4]);
        _fullmoves = stoi(fields[5]);
        
        _attackers = 0;

        get_attackers();
        generate_moves();
    }

    bool Board::is_legal(Move move) {
        // TODO: Implement check logic
        Piece king = {PieceType::King, _turn};
        uint64_t kingbit = _bitboards[king.get_piece_index()];
        return true;
    }

    void Board::register_move(Move move) {
        if(is_legal(move)) {
            _legal_moves.push_back(move);
        }
    }

    void Board::generate_pawn_moves(uint64_t bitboard) {
        uint64_t allies = _bitboards[PieceType::NPieces * 2 +  _turn];
        uint64_t enemies = _bitboards[PieceType::NPieces * 2 + !_turn];
        uint64_t all_pieces = allies | enemies;
        
        uint64_t en_passant_mask = 0;
        if(!_en_passant_target.is_invalid()) {
            en_passant_mask = _en_passant_target.get_mask();
        }
        
        uint64_t advance_board = get_pawn_advance_mask(bitboard, all_pieces, _turn);
        while(advance_board) {
            uint64_t move = advance_board & (-advance_board);
            Move moveobj = {find_lsb(bitboard), find_lsb(move), MoveFlag::Quiet | MoveFlag::PawnAdvance};
            register_move(moveobj);
            advance_board &= (advance_board - 1);
        }

        uint64_t double_board = get_pawn_double_mask(bitboard, all_pieces, _turn);
        while(double_board) {
            uint64_t move = double_board & (-double_board);
            Move moveobj = {find_lsb(bitboard), find_lsb(move), MoveFlag::Quiet | MoveFlag::PawnAdvance | MoveFlag::PawnDouble};
            register_move(moveobj);
            double_board &= (double_board - 1);
        }

        uint64_t capture_board = get_pawn_capture_mask(bitboard, _turn) & enemies;
        while(capture_board) {
            uint64_t move = capture_board & (-capture_board);
            Move moveobj = {find_lsb(bitboard), find_lsb(move), MoveFlag::Capture};
            register_move(moveobj);
            capture_board &= (capture_board - 1);
        }

        uint64_t ep_board = get_pawn_capture_mask(bitboard, _turn) & en_passant_mask;
        while(ep_board) {
            uint64_t move = ep_board & (-ep_board);
            Move moveobj = {find_lsb(bitboard), find_lsb(move), MoveFlag::Capture | MoveFlag::EnPassant};
            register_move(moveobj);
            ep_board &= (ep_board - 1);
        }
    }

    void Board::generate_step_moves(uint64_t bitboard, bool is_king, uint64_t(*mask_func)(uint64_t)) {
        // Generate king moves and exclude any attack squares
        uint64_t allies = _bitboards[PieceType::NPieces * 2 +  _turn];
        uint64_t enemies = _bitboards[PieceType::NPieces * 2 + !_turn];

        uint64_t moves = mask_func(bitboard) & ~allies;
        if(is_king) {
            moves &= ~_attackers; // King cannot move in range of his attackers
        }

        uint64_t capture_board = moves & enemies;
        uint64_t quiet_board = moves & ~enemies;
        while(quiet_board) {
            uint64_t move = quiet_board & (-quiet_board);
            Move moveobj = {find_lsb(bitboard), find_lsb(move), MoveFlag::Quiet};
            register_move(moveobj);
            quiet_board &= (quiet_board - 1);
        }
        while(capture_board) {
            uint64_t move = capture_board & (-capture_board);
            Move moveobj = {find_lsb(bitboard), find_lsb(move), MoveFlag::Capture};
            register_move(moveobj);
            capture_board &= (capture_board - 1);
        }
    }
    
    void Board::generate_slider_moves(uint64_t bitboard, uint64_t(*mask_func)(uint64_t, uint64_t, uint64_t)) {
        uint64_t allies = _bitboards[PieceType::NPieces * 2 +  _turn];
        uint64_t enemies = _bitboards[PieceType::NPieces * 2 + !_turn];

        uint64_t moves = mask_func(bitboard, allies, enemies);

        uint64_t capture_board = moves & enemies;
        uint64_t quiet_board = moves & ~enemies;
        while(quiet_board) {
            uint64_t move = quiet_board & (-quiet_board);
            Move moveobj = {find_lsb(bitboard), find_lsb(move), MoveFlag::Quiet};
            register_move(moveobj);
            quiet_board &= (quiet_board - 1);
        }
        while(capture_board) {
            uint64_t move = capture_board & (-capture_board);
            Move moveobj = {find_lsb(bitboard), find_lsb(move), MoveFlag::Capture};
            register_move(moveobj);
            capture_board &= (capture_board - 1);
        }
    }

    void Board::generate_castling_moves(uint64_t bitboard) {
        uint64_t allies = _bitboards[PieceType::NPieces * 2 +  _turn];
        uint64_t enemies = _bitboards[PieceType::NPieces * 2 + !_turn];
        uint64_t all_pieces = allies | enemies;

        uint8_t rights = 0;
        if(_turn == Color::White) {
            rights = _castling_rights & (Castle::WK | Castle::WQ);
        }
        else {
            rights = _castling_rights & (Castle::BK | Castle::BQ);
        }

        Square from(find_lsb(bitboard));
        while(rights) {
            Castle side = static_cast<Castle>(rights & (-rights));
            
            uint64_t mask = get_castling_mask(all_pieces, side);
            if(mask) {
                int diff = find_lsb(mask) - from.shift;
                Square to(find_lsb(mask));
                register_move({from, to, MoveFlag::Quiet | MoveFlag::Castling});
            }
            rights &= (rights - 1);
        }
    }

    void Board::get_attackers() { 
        // Generate attack vectors for targets
        // Exclude the king from the target list for case when king is still aligned
        // with sliding piece, but further moves away from the attack vector
        Color opponent = static_cast<Color>(!_turn);

        Piece king = {PieceType::King, _turn};
        uint64_t source_squares = _bitboards[PieceType::NPieces * 2 + opponent];
        uint64_t target_squares = _bitboards[PieceType::NPieces * 2 + _turn] & ~_bitboards[king.get_piece_index()];

        for(int type = 0; type < PieceType::NPieces; type++) {
            Piece piece = {static_cast<PieceType>(type), opponent};
            uint64_t bitboard = _bitboards[piece.get_piece_index()];
            if(type == PieceType::Pawn) {
                _attackers |= get_pawn_capture_mask(bitboard, opponent);
            }
            else if(type == PieceType::Knight) {
                _attackers |= get_knight_mask(bitboard) & ~source_squares;
            }
            else if(type == PieceType::King) {
                _attackers |= get_king_mask(bitboard) & ~source_squares;
            }
            else {
                while(bitboard) {
                    uint64_t unit = bitboard & (-bitboard);
                    switch(type) {
                        case PieceType::Bishop:
                            _attackers |= get_bishop_mask(unit, source_squares, target_squares);
                            break;
                        case PieceType::Rook:
                            _attackers |= get_rook_mask(unit, source_squares, target_squares);
                            break;
                        case PieceType::Queen:
                            _attackers |= get_queen_mask(unit, source_squares, target_squares);
                            break;
                        default:
                            break;
                    }
                    bitboard &= (bitboard - 1);
                }
            }   
        }
    }

    void Board::generate_moves() {
        for(int type = 0; type < PieceType::NPieces; type++) {
            Piece piece = {
                static_cast<PieceType>(type), 
                _turn
            };
            uint64_t bitboard = _bitboards[piece.get_piece_index()];
            while(bitboard) {
                uint64_t unit = bitboard & (-bitboard);
                switch(type) {
                    case PieceType::Pawn:
                        generate_pawn_moves(unit);
                        break;
                    case PieceType::King:
                        generate_step_moves(unit, true, get_king_mask);
                        generate_castling_moves(unit);
                        break;
                    case PieceType::Knight:
                        generate_step_moves(unit, false, get_knight_mask);
                        break;
                    case PieceType::Bishop:
                        generate_slider_moves(unit, get_bishop_mask);
                        break;
                    case PieceType::Rook:
                        generate_slider_moves(unit, get_rook_mask);
                        break;
                    case PieceType::Queen:
                        generate_slider_moves(unit, get_queen_mask);
                        break;
                    default:
                        break;
                }
                bitboard &= (bitboard - 1);
            }
        }
    }

    std::string Board::generate_fen() {
        std::string fen = "";
        for(int row = 7; row >= 0; row--) {
            int counter = 0;
            for(int col = 0; col < 8; col++) {
                Piece piece = get_at_coords(row, col);
                if(!piece.is_empty()) {
                    if(counter) {
                        fen += counter + '0';
                        counter = 0;
                    }
                    fen += piece.get_char();
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
        
        if(_castling_rights & Castle::WK)  castling_rights += 'K';
        if(_castling_rights & Castle::WQ) castling_rights += 'Q';
        if(_castling_rights & Castle::BK)  castling_rights += 'k';
        if(_castling_rights & Castle::BQ) castling_rights += 'q';
        if(castling_rights.length() == 0) castling_rights = "-";
        fen += " " + castling_rights;
        
        if(_en_passant_target.is_invalid()) {
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
    
    Piece Board::get_at(Square sq) {
        uint64_t mask = sq.get_mask();
        for(int idx = 0; idx < 12; idx++) {
            if(_bitboards[idx] & mask) {
                PieceType type = static_cast<PieceType>(idx % PieceType::NPieces);
                Color color = static_cast<Color>(idx / PieceType::NPieces);
                return {type, color};
            }
        }
        return {};
    }

    void Board::set_at(Square sq, Piece piece) {
        clear_at(sq);
        uint64_t mask = sq.get_mask();
        _bitboards[piece.get_color_index()] |= mask;
        _bitboards[piece.get_piece_index()] |= mask;
    }

    Piece Board::get_at_coords(int row, int col) {
        return get_at(Square(row * 8 + col));
    }

    void Board::set_at_coords(int row, int col, Piece piece) {
        set_at(Square(row * 8 + col), piece);
    }

    void Board::clear_at(Square sq) {
        // Clear all bitboards at this Square
        uint64_t mask = ~(sq.get_mask());
        _bitboards[12] &= mask;
        _bitboards[13] &= mask;

        uint8_t piece = 0;
        for(piece; piece < 14; piece++) {
            if((_bitboards[piece] >> sq.shift) & 1ULL) {
                _bitboards[piece] &= mask;
                break;
            }
        }
    }

    void Board::execute_move(Move move) {
        _halfmoves++;
        Piece piece = get_at(move.from);
        Piece target = get_at(move.to);

        // Unset castling flags if relevant pieces were moved
        Castle queen_side = (_turn == Color::White) ? (Castle::WQ) : (Castle::BQ);
        Castle king_side = (_turn == Color::White) ? (Castle::WK) : (Castle::BK);

        if(_castling_rights & (king_side | queen_side)) {
            if(piece.type == PieceType::King) {
                _castling_rights ^= (king_side | queen_side);
            }
            else if(piece.type == PieceType::Rook) {
                uint64_t mask = move.from.get_mask();
                if(mask & fileA) _castling_rights ^= queen_side;
                else if(mask & fileH) _castling_rights ^= king_side;
            }
        }
        
        // Move to target square and handle promotions
        clear_at(move.from);
        if(move.flags & MoveFlag::BishopPromo) {
            set_at(move.to, {PieceType::Bishop, _turn});
        }
        else if(move.flags & MoveFlag::RookPromo) {
            set_at(move.to, {PieceType::Rook, _turn});
        }
        else if(move.flags & MoveFlag::KnightPromo) {
            set_at(move.to, {PieceType::Knight, _turn});
        }
        else if(move.flags & MoveFlag::QueenPromo) {
            set_at(move.to, {PieceType::Queen, _turn});
        }
        else {
            set_at(move.to, piece);
        }

        // Move rook if castling
        if(move.flags & MoveFlag::Castling) {
            int rankd = move.to.shift - move.from.shift;
            int dir = (rankd > 0) - (rankd < 0);
            Piece rook = {PieceType::Rook, _turn};
            uint64_t rook_board = _bitboards[rook.get_piece_index()];
            Square target(move.to.shift - dir);
            if(rankd < 0) {
                rook_board &= fileA;
            }
            else {
                rook_board &= fileH;
            }
            clear_at(Square(find_lsb(rook_board)));
            set_at(target, rook);
        } 

        // Check for en passant capture
        if(move.flags & MoveFlag::EnPassant) {
            // Clear the square of the captured pawn
            int rankd = move.to.shift - move.from.shift;
            
            // One rank up or one rank down depending on current player
            int dir = (rankd > 0) - (rankd < 0); 
            clear_at(Square(_en_passant_target.shift - (dir * 8)));
            _en_passant_target = Square();
        }

        // Update en passant position if pawn advanced two ranks
        if(move.flags & MoveFlag::PawnDouble) {
            _en_passant_target = Square(move.from.shift + (move.to.shift - move.from.shift)/2);
        }
        else {
            _en_passant_target = Square();
        }

        // Reset halfmove counter if piece was pawn advance or move was a capture
        if(move.flags & (MoveFlag::PawnAdvance | 
                         MoveFlag::PawnDouble  | 
                         MoveFlag::EnPassant   | 
                         MoveFlag::Capture)) {
            _halfmoves = 0;
        }

        // Update turn and fullmove counter
        if(_turn == Color::Black) {
            _fullmoves++;
        }
        _turn = static_cast<Color>(!_turn);

        get_attackers();
        generate_moves();
    }

    Move Board::create_move(Square from, Square to) {
        for(auto &move : _legal_moves) {
            if(move.from == from && move.to == to) {
                return move;
            }
        }
        return {};
    }

    std::vector<Move> Board::get_moves() {
        return _legal_moves;
    }

    int Board::get_halfmoves() {
        return _halfmoves;
    }

    Color Board::get_turn() {
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
                Piece piece = get_at_coords(rank, file);
                if(!piece.is_empty()) {
                    std::cout << piece.get_display() << " ";
                }
                else {
                    std::cout << "- ";
                }
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