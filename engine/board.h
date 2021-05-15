#ifndef CHESS_BOARD_H_
#define CHESS_BOARD_H_

#include <iostream>
#include <string>

#include "move.h"
#include "bits.h"
#include "util.h"

namespace chess {
    /**
     *   pieces are uniquely indexed on bitboard and piece weight arrays
     */
    enum Piece {
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
        Black = 13,
        Empty = 15
    };
    static const std::string PieceChars = "PRNBQKprnbqk";
    static const std::string PieceDisplay[] = {
        "\u2659", "\u2656", "\u2658", "\u2657", "\u2655", "\u2654",
        "\u265F", "\u265C", "\u265E", "\u265D", "\u265B", "\u265A"
    };

    /**
     * Calculate the material score of the board state
     */
    const int piece_weights[] = {
        1,  5,  3,  3,  9,  4, // White pieces
        -1, -5, -3, -3, -9, -4  // Black pieces
    };

    /**
     * Represents a chess board's state
     */
    class Board {
        // Represent the positions of each of the 12 pieces on the board
        // Extra 2 bitboards represent white/black pieces in general
        uint64_t _bitboards[14] = {0};
        uint8_t _turn;            // w or b
        uint8_t _castling_rights; // qkQK (from most to least significant bit)
        Position _en_passant_target;
        int _halfmoves;
        int _fullmoves;

        uint64_t _attackers;
        std::vector<Move> _legal_moves;

        /**
         * Generate all pseudo-legal moves for each piece and add them to a move vector
         */
        void generate_piece_moves(uint64_t bitboard, uint64_t(*mask_func)(uint64_t, uint64_t));

        // Slider moves need more information about the board
        void generate_slider_moves(uint64_t bitboard, uint64_t(*mask_func)(uint64_t, uint64_t, uint64_t));

        // Pawn function has special cases (ugh.)
        void generate_pawn_moves(uint64_t bitboard);

        // Castling
        void generate_castling_moves(uint64_t bitboard);

        /**
         * Test if a pseudo-legal move is legal
         * Algorithm for generating legal moves from pseudo legal?
         * - If king is the moving piece, make sure destination square is not an attack target
         * - If move is an en passant, king must not currently be in check
         * - If non-king piece, it must not be pinned, or if it is, to and from pieces must be aligned with king
         */
        bool is_legal(Move move);

        /**
         * If a pseudo-legal move is legal, register it to the move list
         */
        void register_move(Move move);

        /**
         * Generate all legal moves
         * If move list is empty, then player is in checkmate
         */
        void generate_moves();

        /**
         * Test if a position is pinned
         */
        bool is_king_pinned(Position pos);

        /**
         * Check if moving a piece into a position will protect the king
         */
        bool is_protecting_king(Position pos);

        /**
         * Check if removing opposing pieces from the board will protect the king
         */
        bool can_capture_attackers(uint64_t mask);

        /**
         * Test if an en passant move leads to a discovered check
         */
        bool en_passant_discovered(uint64_t color_mask, uint64_t opposite_color_mask);

        /**
         * Get the attack vectors for all of the opposing pieces
         */
        uint64_t get_attackers();
        
    public:
        Board(std::string fen_string="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        Board(Board &other) : Board(other.generate_fen()) {};

        /**
         * Generate a FEN string of the current state for serialization
         */
        std::string generate_fen();

        /**
         * Calculate the material score for the current board state
         * Positive values mean white has more material than black
         */
        int calculate_material();
        
        /**
         * Get a piece on the board
         */
        Piece get_at(Position pos);

        /**
         * Set a piece on the board
         */
        void set_at(Position pos, Piece piece);

        /**
         * Clear a square on the board
         */
        void clear_at(Position pos);

        /**
         * Get the piece at a coordinate
         */
        Piece get_at_coords(int row, int col);

        /**
         * Set the piece at a coordinate
         */
        void set_at_coords(int row, int col, Piece piece);

        /**
         * Execute a move and update internal state
         */
        void execute_move(Move move);

        /**
         * Generate a valid chess move given shift positions
         * Used to validate move positions from user input
         */
        Move create_move(Position from, Position to); 

        /**
         * Get all legal moves available to the current player
         */
        std::vector<Move> get_legal_moves();

        /**
         * Get the current number of halfmoves to enforce the 50-move rule
         */
        int get_halfmoves();

        /**
         * Get the current turn (either 'w' or 'b')
         */
        char get_turn();

        /**
         * Print the board on the console
         */
        void print();
    };
}

#endif