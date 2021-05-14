#include "bits.h"

namespace chess {
    void print_bitboard(uint64_t bitboard) {
        std::bitset<64> bitarray(bitboard);
        for(int i = 0; i < 64; i++) {
            std::cout << (bitarray[i] ? "o " : ". ");
            if((i+1)%8 == 0) std::cout << "\n";
        }
        std::cout << "\n";
    }

    uint64_t flip_vertical(uint64_t bitboard) {
        return _byteswap_uint64(bitboard);
    }

    uint64_t flip_horizontal(uint64_t bitboard) {
        bitboard = ((bitboard >> 1) & horflip_k[0]) +  2*(bitboard & horflip_k[0]);
        bitboard = ((bitboard >> 2) & horflip_k[1]) +  4*(bitboard & horflip_k[1]);
        bitboard = ((bitboard >> 4) & horflip_k[2]) + 16*(bitboard & horflip_k[2]);
        return bitboard;
    }
    
    int find_lsb(uint64_t binary) {
        return bitscan_table[((binary & -binary) * debruijn64) >> 58];
    }

    uint64_t get_diagonal_mask(int shift) {
        int diag = 8*(shift & 7) - (shift & 56);
        int nort = -diag & ( diag >> 31);
        int sout =  diag & (-diag >> 31);
        return (main_diagonal >> sout) << nort;
    }

    uint64_t get_antidiag_mask(int shift) {
        int diag = 56 - 8*(shift & 7) - (shift & 56);
        int nort = -diag & ( diag >> 31);
        int sout =  diag & (-diag >> 31);
        return (anti_diagnoal >> sout) << nort;
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

    uint64_t get_ray_attack(uint64_t bitboard, uint64_t occupied) {
        return occupied ^ (occupied - 2 * bitboard);
    }

    uint64_t get_rook_mask(uint64_t bitboard, uint64_t same_color, uint64_t opposite_color) {
        const int shift = find_lsb(bitboard);
        const uint64_t rank_mask = 0xFF00000000000000 >> (56 - 8 * (shift/8));
        const uint64_t file_mask = 0x0101010101010101 << (7 & shift);
        
        const uint64_t occupied = same_color | opposite_color;
        uint64_t rank_positive = get_ray_attack(bitboard, occupied & rank_mask) & rank_mask;
        uint64_t rank_negative = get_ray_attack(flip_horizontal(bitboard), 
                                                flip_horizontal(occupied & rank_mask)) & rank_mask;

        uint64_t file_positive = get_ray_attack(bitboard, occupied & file_mask) & file_mask;
        uint64_t file_negative = get_ray_attack(flip_vertical(bitboard), 
                                                flip_vertical(occupied & file_mask)) & file_mask;
        return (rank_positive | flip_horizontal(rank_negative) | 
                file_positive | flip_vertical(file_negative)) & ~same_color;
    }
    
    uint64_t get_bishop_mask(uint64_t bitboard, uint64_t same_color, uint64_t opposite_color) {
        int shift = find_lsb(bitboard);
        uint64_t diagonal_mask = get_diagonal_mask(shift);
        uint64_t antidiag_mask = get_antidiag_mask(shift);

        const uint64_t occupied = same_color | opposite_color;
        uint64_t diagonal_positive = get_ray_attack(bitboard, occupied & diagonal_mask) & diagonal_mask;
        uint64_t diagonal_negative = get_ray_attack(flip_vertical(bitboard), 
                                                    flip_vertical(occupied & diagonal_mask)) & flip_vertical(diagonal_mask);

        uint64_t antidiag_positive = get_ray_attack(bitboard, occupied & antidiag_mask) & antidiag_mask;
        uint64_t antidiag_negative = get_ray_attack(flip_vertical(bitboard), 
                                                    flip_vertical(occupied & antidiag_mask)) & flip_vertical(antidiag_mask);
        return (diagonal_positive | flip_vertical(diagonal_negative) | 
                antidiag_positive | flip_vertical(antidiag_negative)) & ~same_color;
    }

    uint64_t get_queen_mask(uint64_t bitboard, uint64_t same_color, uint64_t opposite_color) {
        // Queen has the mobility of the rook and bishop combined
        return get_rook_mask(bitboard, same_color, opposite_color) | 
               get_bishop_mask(bitboard, same_color, opposite_color);
    }
}