#include "bits.h"

namespace chess {
    /**
     * Move generation algorithm
     * Output: Array of bitboards representing possible movements for each piece of the current turn
     * This is from white's perspective. To do black, just flip the original bitboard, calculate movement bitboards, then flip each movement bitboard
     *
     * Directional shifts (for non-sliding pieces): -1, 1, -7, 7, 8, -8, 9, -9
     * Pawn: 2 steps up if on 2nd rank or 1 step forward, 1 diagonal up for capturing (either normal or en passant)
     * Bishop: Diagonal and anti-diagonal, (sliding)
     * Rook: Vertical and horizontal, (sliding)
     * Queen: Vertical, horizontal, diagonal, anti-diagonal (sliding)
     * Knight: 2 steps on one axis, 1 step on the other
     * King: 1 step in any direction
     */
    void print_bitboard(uint64_t bitboard) {
        std::bitset<64> bitarray(bitboard);
        for(int i = 0; i < 64; i++) {
            std::cout << bitarray[i];
            if((i+1)%8 == 0) std::cout << "\n";
        }
        std::cout << "\n";
    }

    uint64_t flip_vertical(uint64_t bitboard) {
        return _byteswap_uint64(bitboard);
    }

    uint64_t flip_horizontal(uint64_t bitboard) {
        const uint64_t k1 = 0x5555555555555555;
        const uint64_t k2 = 0x3333333333333333;
        const uint64_t k4 = 0x0f0f0f0f0f0f0f0f;
        bitboard = ((bitboard >> 1) & k1) +  2*(bitboard & k1);
        bitboard = ((bitboard >> 2) & k2) +  4*(bitboard & k2);
        bitboard = ((bitboard >> 4) & k4) + 16*(bitboard & k4);
        return bitboard;
    }
    
    int find_lsb(uint64_t binary) {
        int count = 0;
        while(binary ^ 1) {
            binary >>= 1;
            count++;
        }
        return count;
    }

    uint64_t shift(uint64_t binary, int shift) {
        if(shift < 0) return binary >> -shift;
        return binary << shift;
    }

    uint64_t get_adjacent(uint64_t bitboard, Direction dir) {
        return shift(bitboard, direction_shift[dir]) & wrap_bitmasks[dir];
    }

    uint64_t get_king_mask(uint64_t bitboard, uint64_t same_color) {
        uint64_t moves = get_adjacent(bitboard, Direction::Left)     |
                         get_adjacent(bitboard, Direction::Right)    |
                         get_adjacent(bitboard, Direction::Up)       |
                         get_adjacent(bitboard, Direction::Down)     |
                         get_adjacent(bitboard, Direction::UpLeft)   |
                         get_adjacent(bitboard, Direction::UpRight)  |
                         get_adjacent(bitboard, Direction::DownLeft) |
                         get_adjacent(bitboard, Direction::DownRight);
        return moves & ~same_color;
    }

    uint64_t get_knight_mask(uint64_t bitboard, uint64_t same_color) {
        uint64_t moves = get_adjacent(get_adjacent(bitboard, Direction::UpLeft), Direction::Left)     |
                         get_adjacent(get_adjacent(bitboard, Direction::DownLeft), Direction::Left)   |
                         get_adjacent(get_adjacent(bitboard, Direction::UpRight), Direction::Right)   |
                         get_adjacent(get_adjacent(bitboard, Direction::DownRight), Direction::Right) |
                         get_adjacent(get_adjacent(bitboard, Direction::UpLeft), Direction::Up)       |
                         get_adjacent(get_adjacent(bitboard, Direction::UpRight), Direction::Up)      |
                         get_adjacent(get_adjacent(bitboard, Direction::DownLeft), Direction::Down)   |
                         get_adjacent(get_adjacent(bitboard, Direction::DownRight), Direction::Down);
        return moves & ~same_color;
    }

    uint64_t get_pawn_advance_mask(uint64_t bitboard, uint64_t all_pieces) {
        return get_adjacent(bitboard, Direction::Down) & ~all_pieces;
    }

    uint64_t get_pawn_double_mask(uint64_t bitboard, uint64_t all_pieces) {
        // Move twice, assuming both cells are clear
        // Only move if target square is rank 4
        uint64_t rank4 = 0x00000000FF000000;
        return get_pawn_advance_mask(get_pawn_advance_mask(bitboard, all_pieces), all_pieces) & rank4;
    }

    uint64_t get_pawn_capture_mask(uint64_t bitboard, uint64_t opposite_color) {
        uint64_t moves = get_adjacent(bitboard, Direction::DownLeft) |
                         get_adjacent(bitboard, Direction::DownRight);
        return moves & opposite_color;
    }
    
    uint64_t get_pawn_en_passant_mask(uint64_t bitboard, uint64_t en_passant) {
        uint64_t moves = get_adjacent(bitboard, Direction::DownLeft) |
                         get_adjacent(bitboard, Direction::DownRight);
        return moves & en_passant;
    }

    uint64_t get_rook_mask(uint64_t bitboard, uint64_t same_color) {
        // TODO: Get file and rank of the rook
        return bitboard;
    }

    uint64_t get_bishop_mask(uint64_t bitboard, uint64_t same_color) {
        // TODO: Deal with diagonals
        return bitboard;
    }

    uint64_t get_queen_mask(uint64_t bitboard, uint64_t same_color) {
        // Queen has the mobility of the rook and bishop combined
        return get_rook_mask(bitboard, same_color) | get_bishop_mask(bitboard, same_color);
    }
}