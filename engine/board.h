#ifndef CHESS_BOARD_H_
#define CHESS_BOARD_H_

#include <iostream>
#include <string>

#include "piece.h"
#include "move.h"
#include "bits.h"
#include "util.h"

namespace chess {
    /**
     * Represents a chess board's state
     */
    class Board {
        // Represent the positions of each of the 12 pieces on the board
        // Extra 2 bitboards represent white/black pieces in general
        uint64_t _bitboards[12] = {0};
        uint64_t _whiteboard = 0;
        uint64_t _blackboard = 0;

        // qkQK (from most to least significant bit)
        uint8_t _castling_rights;
        Square _en_passant_target;
        Color _turn;

        int _halfmoves;
        int _fullmoves;
        
        // Bitboard representing the attackers on each square (excluding king)
        uint64_t _attackers = 0; 
        std::vector<Move> _legal_moves;

        /**
         * Generate all pseudo-legal moves for single step moves
         */
        void generate_step_moves(uint64_t bitboard, bool is_king, uint64_t(*mask_func)(uint64_t));

        /**
         *  Slider moves need more information about the board
         */
        void generate_slider_moves(uint64_t bitboard, uint64_t(*mask_func)(uint64_t, uint64_t, uint64_t));

        /**
         * Pawn function has special cases (ugh.)
         */
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
         * Get the attack vectors for all of the opposing pieces
         */
        void get_attackers();

        /**
         * Generate all legal moves
         * If move list is empty, then player is in checkmate
         */
        void generate_moves();

        // /**
        //  * Test if a position is pinned
        //  */
        // bool is_king_pinned(Square sq);

        // /**
        //  * Check if moving a piece into a position will protect the king
        //  */
        // bool is_protecting_king(Square sq);

        // /**
        //  * Check if removing opposing pieces from the board will protect the king
        //  */
        // bool can_capture_attackers(uint64_t mask);

        // /**
        //  * Test if an en passant move leads to a discovered check
        //  */
        // bool en_passant_discovered(uint64_t color_mask, uint64_t opposite_color_mask);
        
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
        Piece get_at(Square sq);

        /**
         * Set a piece on the board
         */
        void set_at(Square sq, Piece piece);

        /**
         * Get a piece on the board by coordinates
         */
        Piece get_at_coords(int row, int col);

        /**
         * Set a piece on the board by coordinates
         */
        void set_at_coords(int row, int col, Piece piece);

        /**
         * Clear a square on the board
         */
        void clear_at(Square sq);

        /**
         * Execute a move and update internal state
         */
        void execute_move(Move move);

        /**
         * Generate a valid chess move given shift positions
         * Used to validate move positions from user input
         */
        Move create_move(Square from, Square to); 

        /**
         * Get all legal moves available to the current player
         */
        std::vector<Move> get_moves();

        /**
         * Get the current number of halfmoves to enforce the 50-move rule
         */
        int get_halfmoves();

        /**
         * Get the current turn (either White or Black)
         */
        Color get_turn();

        /**
         * Print the board on the console
         */
        void print();
    };
}

#endif