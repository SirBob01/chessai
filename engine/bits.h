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
    static const int direction_shift[8] = {9, 1, -7, -8, -9, -1, 7, 8};

    /**
     * Constant bitmasks to eliminate overflowing bits during shift operations
     */
    static const uint64_t wrap_bitmasks[8] = {
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
    static const int bitscan_table[64] = {
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
     * Print a bitboard (8 bits per row)
     */
    void print_bitboard(uint64_t bitboard);

    /**
     * Vertically flip the bitboard
     */
    uint64_t flip_vertical(uint64_t bitboard);

    /**
     * Horizontally flip the bitboard
     */
    uint64_t flip_horizontal(uint64_t bitboard);

    /**
     * Calculates the index of the least significant bit in the binary string
     * Use this to iterate through all set bits on a bitboard (active pieces)
     */
    int find_lsb(uint64_t binary);

    /**
     * Get the diagonal from the current bit position
     */
    uint64_t get_diagonal_mask(int shift);

    /**
     * Get the antidiagonal from the current bit position
     */
    uint64_t get_antidiag_mask(int shift);

    /**
     * Shift a binary string by an arbitrary amount, negative shifts are right shifts
     */ 
    uint64_t shift(uint64_t binary, int shift);

    /**
     * Return a new bitboard with a point adjacent to the binary string in a specific direction
     */
    uint64_t get_adjacent(uint64_t bitboard, Direction dir);

    /**
     * Get the all possible directions the king can move to from its current position
     */
    uint64_t get_king_mask(uint64_t bitboard, uint64_t same_color);

    /**
     * Get the all possible directions the knight can move to from its current position
     */
    uint64_t get_knight_mask(uint64_t bitboard, uint64_t same_color);

    /**
     * Get the position of the pawn after advancing a single rank
     */
    uint64_t get_pawn_advance_mask(uint64_t bitboard, uint64_t all_pieces);

    /**
     * Get the position of the pawn after advancing 2 ranks
     */
    uint64_t get_pawn_double_mask(uint64_t bitboard, uint64_t all_pieces);

    /**
     * Get the all possible positions of the pawn if capturing (either en passant or regular)
     */
    uint64_t get_pawn_capture_mask(uint64_t bitboard, uint64_t opposite_color);

    /**
     * Get all possible en passant moves
     */
    uint64_t get_pawn_en_passant_mask(uint64_t bitboard, uint64_t en_passant);

    /**
     * For sliding pieces and a set of blockers, find the valid movement bits
     * Only works for positive rays, must reverse the bits to work with negative rays
     */
    uint64_t get_ray_attack(uint64_t bitboard, uint64_t occupied);

    /**
     * Get all possible moves for the rook
     */
    uint64_t get_rook_mask(uint64_t bitboard, uint64_t same_color, uint64_t opposite_color);

    /**
     * Get all possible moves for the bishop
     */
    uint64_t get_bishop_mask(uint64_t bitboard, uint64_t same_color, uint64_t opposite_color);

    /**
     * Get all possible moves for the queen
     * Simply perform bitwise OR on the rook and bishop masks
     */
    uint64_t get_queen_mask(uint64_t bitboard, uint64_t same_color, uint64_t opposite_color);
}

#endif