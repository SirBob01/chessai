#ifndef CHESS_BOARD_H_
#define CHESS_BOARD_H_

#include <iostream>
#include <string>

#include "bits.h"
#include "util.h"

namespace chess {
    /**
     *  Chess pieces are uniquely indexed on bitboard and piece weight arrays
     */
    enum ChessPiece {
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
    struct ChessPosition {
        int shift;

        ChessPosition() : shift(-1) {}
        ChessPosition(int shift) : shift(shift) {}
        ChessPosition(char file, char rank);

        std::string standard_notation();
    };

    /**
     * Flags that describe a chess move
     */
    enum ChessMoveFlag {
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
    struct ChessMove {
        ChessPosition from;
        ChessPosition to;
        unsigned flags = ChessMoveFlag::Quiet;
    };


    /**
     * Represents a chess board's state
     */
    class ChessBoard {
        // Represent the positions of each of the 12 pieces on the board
        // Extra 2 bitboards represent white/black pieces in general
        uint64_t _bitboards[14] = {0};
        uint8_t _turn;            // w or b
        uint8_t _castling_rights; // qkQK (from most to least significant bit)
        ChessPosition _en_passant_target;
        int _halfmoves;
        int _fullmoves;

        /**
         * Generate all possible pawn moves and append them to a move vector
         */
        void generate_pawn_moves(uint64_t bitboard, std::vector<ChessMove> &moves);

    public:
        ChessBoard(std::string fen_string="rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        ChessBoard(ChessBoard &other) : ChessBoard(other.generate_fen()) {};

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
        ChessPiece get_at(ChessPosition pos);

        /**
         * Set a piece on the board
         */
        void set_at(ChessPosition pos, ChessPiece piece);

        /**
         * Clear a square on the board
         */
        void clear_at(ChessPosition pos);

        /**
         * Get the piece at a coordinate
         */
        ChessPiece get_at_coords(int row, int col);

        /**
         * Set the piece at a coordinate
         */
        void set_at_coords(int row, int col, ChessPiece piece);

        /**
         * Execute a move and update internal state
         */
        void execute_move(ChessMove move);

        /**
         * Generate a valid chess move given shift positions
         * Used to validate move positions from user input
         */
        ChessMove create_move(ChessPosition from, ChessPosition to); 

        /**
         * Generate a move given from and to positions
         * If move list is empty, then player is in checkmate
         */
        std::vector<ChessMove> generate_move_list();

        /**
         * Print the board on the console
         */
        void print();
    };
}

#endif