#ifndef CHESS_BITS_H_
#define CHESS_BITS_H_

#include <bitset>
#include <iostream>
#include <cstdint>

namespace chess {
    /** 
     * Cardinal and ordinal directions on the board
     */
    enum Direction {
        DownRight = 0,
        Right     = 1,
        UpRight   = 2,
        Up        = 3,
        UpLeft    = 4,
        Left      = 5,
        DownLeft  = 6,
        Down      = 7
    };

    /** 
     * Values for calculating adjacent positions using bitwise operations
     */
    constexpr int direction_shift[8] = {9, 1, -7, -8, -9, -1, 7, 8};

    /**
     * Constant bitmasks to eliminate overflowing bits during shift operations
     */
    constexpr uint64_t wrap_bitmasks[8] = {
        0xfefefefefefefe00,
        0xfefefefefefefefe,
        0x00fefefefefefefe,
        0x00ffffffffffffff,
        0x007f7f7f7f7f7f7f,
        0x7f7f7f7f7f7f7f7f,
        0x7f7f7f7f7f7f7f00,
        0xffffffffffffff00,
    };

    /**
     * Bit scan table for fetching the index of the least significant bit
     */
    constexpr uint64_t debruijn64 = 0x07EDD5E59A4E28C2;
    constexpr int bitscan_table[64] = {
        63,  0, 58,  1, 59, 47, 53,  2,
        60, 39, 48, 27, 54, 33, 42,  3,
        61, 51, 37, 40, 49, 18, 28, 20,
        55, 30, 34, 11, 43, 14, 22,  4,
        62, 57, 46, 52, 38, 26, 32, 41,
        50, 36, 17, 19, 29, 10, 13, 21,
        56, 45, 25, 31, 35, 16,  9, 12,
        44, 24, 15,  8, 23,  7,  6,  5
    };

    /**
     * Horizontal bitboard flip constants
     */
    constexpr uint64_t horflip_k[3] = {
        0x5555555555555555,
        0x3333333333333333,
        0x0f0f0f0f0f0f0f0f
    };

    /**
     * Bitmask constants
     */
    constexpr uint64_t rank4 = 0x00000000FF000000;
    constexpr uint64_t end_ranks = 0xFF000000000000FF;
    constexpr uint64_t main_diagonal = 0x8040201008040201;
    constexpr uint64_t anti_diagnoal = 0x0102040810204080;

    /**
     * Print a bitboard (8 bits per row)
     */
    void print_bitboard(uint64_t bitboard);

    /**
     * Vertically flip the bitboard
     */
    inline uint64_t flip_vertical(uint64_t bitboard) {
        return _byteswap_uint64(bitboard);
    }

    /**
     * Horizontally flip the bitboard
     */
    inline uint64_t flip_horizontal(uint64_t bitboard) {
        bitboard = ((bitboard >> 1) & horflip_k[0]) +  2*(bitboard & horflip_k[0]);
        bitboard = ((bitboard >> 2) & horflip_k[1]) +  4*(bitboard & horflip_k[1]);
        bitboard = ((bitboard >> 4) & horflip_k[2]) + 16*(bitboard & horflip_k[2]);
        return bitboard;
    }

    /**
     * Calculates the index of the least significant bit in the binary string
     * Use this to iterate through all set bits on a bitboard (active pieces)
     */
    inline int find_lsb(uint64_t binary) {
        return bitscan_table[((binary & -binary) * debruijn64) >> 58];
    }

    /**
     * Get the diagonal from the current bit position
     */
    inline uint64_t get_diagonal_mask(int shift) {
        int diag = 8*(shift & 7) - (shift & 56);
        int nort = -diag & ( diag >> 31);
        int sout =  diag & (-diag >> 31);
        return (main_diagonal >> sout) << nort;
    }

    /**
     * Get the antidiagonal from the current bit position
     */
    inline uint64_t get_antidiag_mask(int shift) {
        int diag = 56 - 8*(shift & 7) - (shift & 56);
        int nort = -diag & ( diag >> 31);
        int sout =  diag & (-diag >> 31);
        return (anti_diagnoal >> sout) << nort;   
    }

    /**
     * Shift a binary string by an arbitrary amount, negative shifts are right shifts
     */ 
    inline uint64_t shift(uint64_t binary, int shift) {
        return shift < 0 ? binary >> -shift : binary << shift;
    }

    /**
     * Return a new bitboard with a point adjacent to the binary string in a specific direction
     */
    inline uint64_t get_adjacent(uint64_t bitboard, Direction dir) {
        return shift(bitboard, direction_shift[dir]) & wrap_bitmasks[dir];
    }

    /**
     * For sliding pieces and a set of blockers, find the valid movement bits
     * Only works for positive rays, must reverse the bits to work with negative rays
     */
    inline uint64_t get_ray_attack(uint64_t bitboard, uint64_t occupied) {
        return occupied ^ (occupied - 2 * bitboard);
    }

    /**
     * Test if pieces on bitboards A, B, and C are colinear on the 8 directions
     */
    inline bool is_aligned(uint64_t A, uint64_t B, uint64_t C) {
        const int shift = find_lsb(A);
        const uint64_t mask1 = get_diagonal_mask(shift);
        const uint64_t mask2 = get_antidiag_mask(shift);
        const uint64_t mask3 = 0xFF00000000000000ULL >> (56 - 8 * (shift/8));
        const uint64_t mask4 = 0x0101010101010101ULL << (7 & shift);
        return !((mask1 | C) ^ (mask1 | B)) ||
               !((mask2 | C) ^ (mask2 | B)) ||
               !((mask3 | C) ^ (mask3 | B)) ||
               !((mask4 | C) ^ (mask4 | B));
    }

    /**
     * Get the all possible directions the king can move to from its current position
     */
    inline uint64_t get_king_mask(uint64_t bitboard, uint64_t same_color) {
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

    /**
     * Get the all possible directions the knight can move to from its current position
     */
    inline uint64_t get_knight_mask(uint64_t bitboard, uint64_t same_color) {
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

    /**
     * Get the position of the pawn after advancing a single rank
     */
    inline uint64_t get_pawn_advance_mask(uint64_t bitboard, uint64_t all_pieces) {
        return get_adjacent(bitboard, Direction::Down) & ~all_pieces;
    }

    /**
     * Get the position of the pawn after advancing 2 ranks
     */
    inline uint64_t get_pawn_double_mask(uint64_t bitboard, uint64_t all_pieces) {
        // Move twice, assuming both cells are clear
        // Only move if target square is rank 4
        return get_pawn_advance_mask(get_pawn_advance_mask(bitboard, all_pieces), all_pieces) & rank4;
    }

    /**
     * Get the all possible positions of the pawn if capturing (either en passant or regular)
     */
    inline uint64_t get_pawn_capture_mask(uint64_t bitboard, uint64_t opposite_color) {
        uint64_t moves = get_adjacent(bitboard, Direction::DownLeft) |
                         get_adjacent(bitboard, Direction::DownRight);
        return moves & opposite_color;
    }

    /**
     * Get all possible en passant moves
     */
    inline uint64_t get_pawn_en_passant_mask(uint64_t bitboard, uint64_t en_passant) {
        uint64_t moves = get_adjacent(bitboard, Direction::DownLeft) |
                         get_adjacent(bitboard, Direction::DownRight);
        return moves & en_passant;
    }

    /**
     * Get all possible moves for the rook
     */
    inline uint64_t get_rook_mask(uint64_t bitboard, uint64_t same_color, uint64_t opposite_color) {
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

    /**
     * Get all possible moves for the bishop
     */
    inline uint64_t get_bishop_mask(uint64_t bitboard, uint64_t same_color, uint64_t opposite_color) {
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

    /**
     * Get all possible moves for the queen
     * Simply perform bitwise OR on the rook and bishop masks
     */
    inline uint64_t get_queen_mask(uint64_t bitboard, uint64_t same_color, uint64_t opposite_color) {
        return get_rook_mask(bitboard, same_color, opposite_color) | 
               get_bishop_mask(bitboard, same_color, opposite_color);
    }
}

#endif