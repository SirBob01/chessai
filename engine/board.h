#ifndef CHESS_BOARD_H_
#define CHESS_BOARD_H_

#include <iostream>
#include <string>

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
     * A position on the board represented by a bitshift value (0 - 63)
     */
    struct Position {
        int shift;

        Position() : shift(-1) {}
        Position(int shift) : shift(shift) {}
        Position(char file, char rank);

        std::string standard_notation();
    };

    /**
     * Flags that describe a chess move
     */
    enum MoveFlag {
        Quiet       = 0,
        Capture     = 1 << 0,
        EnPassant   = 1 << 1,
        PawnAdvance = 1 << 2,
        PawnDouble  = 1 << 3,
        KingCastle  = 1 << 4,
        QueenCastle = 1 << 5,
        KnightPromo = 1 << 6,
        QueenPromo  = 1 << 7,
        BishopPromo = 1 << 8,
        RookPromo   = 1 << 9,
        Invalid     = 1 << 10
    };

    /**
     * Container of a chess move
     */
    struct Move {
        Position from;
        Position to;
        unsigned flags = MoveFlag::Quiet;
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

        std::vector<Move> _legal_moves;

        /**
         * Generate all pseudo-legal moves for each piece and add them to a move vector
         */
        void generate_piece_moves(uint64_t bitboard, std::vector<Move> &moves, 
                                  uint64_t(*mask_func)(uint64_t, uint64_t));

        // Slider moves need more information about the board
        void generate_slider_moves(uint64_t bitboard, std::vector<Move> &moves, 
                                   uint64_t(*mask_func)(uint64_t, uint64_t, uint64_t));

        // Pawn function has special cases (ugh.)
        void generate_pawn_moves(uint64_t bitboard, std::vector<Move> &moves);

        /**
         * Generate all pseudo legal moves by the current turn
         */
        std::vector<Move> generate_pseudo_legal_moves();

        /**
         * Test if the opposing king is in check
         */
        bool is_in_check();

    public:
        Board(std::string fen_string="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        Board(Board &other) : Board(other.generate_fen()) {};

        /**
         * Generate all legal moves
         * If move list is empty, then player is in checkmate
         * Algorithm for generating legal moves from pseudo legal?
         * - Create a copy of the board 
         * - Execute the move
         * - Make sure king is not in check at the start
         */
        void generate();

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