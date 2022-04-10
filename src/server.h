#ifndef SERVER_H_
#define SERVER_H_

#include <brainiac.h>
#include <dpp/dpp.h>
#include <iostream>
#include <variant>

#include "id.h"
#include "image.h"

/**
 * Represents a single game
 */
struct Game {
    uint64_t id;
    dpp::user white;
    dpp::user black;
    chess::Board board;
    bool bot = false;

    Game(dpp::user _white, dpp::user _black) : white(_white), black(_black){};
};

/**
 * Generate a PNG image of the board and save it to disk
 */
void generate_image(chess::Board &board, std::string filename);

/**
 * Discord bot client running main game loop
 */
class ChessServer {
    std::unordered_map<uint64_t, std::unique_ptr<Game>> _games;
    std::unordered_map<std::string, uint64_t> _users;
    IDGen _id_generator;

    dpp::cluster &_bot;

  public:
    ChessServer(dpp::cluster &bot);

    /**
     * Send an embed containing information about a game
     */
    dpp::message game_info(const dpp::interaction_create_t &event,
                           Game &game,
                           std::string message = "");

    /**
     * Get the string hash of a user
     */
    std::string hash_user(dpp::user user);

    /**
     * Delete an active chess game
     */
    void delete_game(dpp::user user);

    /**
     * Run chess bot on a separate thread
     */
    void bot_moves(const dpp::interaction_create_t &event, Game &game);

    /**
     * Play command
     *
     * User initiates a game
     */
    void on_play(const dpp::interaction_create_t &event,
                 dpp::user &opponent,
                 std::string color);

    /**
     * Move command
     *
     * User submits a move for a game
     */
    void on_move(const dpp::interaction_create_t &event,
                 std::string move_input);

    /**
     * Board command
     *
     * User wants to display the board
     */
    void on_board(const dpp::interaction_create_t &event);

    /**
     * Resign command
     *
     * User wants to terminate a game
     */
    void on_resign(const dpp::interaction_create_t &event);
};

#endif