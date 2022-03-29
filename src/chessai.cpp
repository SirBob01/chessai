#include <brainiac.h>
#include <cassert>
#include <cstdlib>
#include <dpp/dpp.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>

#include "image.h"

/**
 * Generate a PNG image of the board and save it to disk
 */
void generate_image(chess::Board &board, std::string filename) {
    Image *base = new Image(64 + 128 * 8, 64 + 128 * 8);
    base->fill({0.08, 0.08, 0.08, 1.0});

    std::vector<Image *> pieces;
    for (int i = 0; i < 12; i++) {
        pieces.push_back(new Image("../images/" + std::to_string(i) + ".png"));
    }
    std::vector<Image *> tiles;
    tiles.push_back(new Image("../images/brown0.png"));
    tiles.push_back(new Image("../images/brown1.png"));

    bool square_white = true;
    for (int rank = 0; rank < 8; rank++) {
        bool current_color = square_white;
        for (int file = 0; file < 8; file++) {
            // Render appropriate squares
            Image *tile = tiles[current_color];
            base->draw(tile, file * tile->width + 32, rank * tile->height + 32);
            current_color = !current_color;
        }
        square_white = !square_white;
    }
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            // Render a the piece on top (if any)
            chess::Piece piece = board.get_at_coords(rank, file);
            if (piece.is_empty()) {
                continue;
            }
            Image *piece_image = pieces[piece.get_piece_index()];

            int x_offset = 0;
            if (piece.type == chess::PieceType::Pawn ||
                piece.type == chess::PieceType::Knight ||
                piece.type == chess::PieceType::Rook) {
                x_offset = 10;
            }
            base->draw(piece_image, (file * tiles[0]->width) + x_offset + 32,
                       ((7 - rank) * tiles[0]->height + 32));
        }
    }
    base->save(filename);
    for (auto piece : pieces) {
        delete piece;
    }
    for (auto tile : tiles) {
        delete tile;
    }
    delete base;
}

// /**
//  * Represents a single game
//  */
// struct Game {
//     SleepyDiscord::User white;
//     SleepyDiscord::User black;
//     chess::Board board;
//     bool bot = false;

//     Game(SleepyDiscord::User &_white, SleepyDiscord::User &_black)
//         : white(_white), black(_black){};
// };

// /**
//  * Discord bot client running main game loop
//  */
// class ChessAI : public SleepyDiscord::DiscordClient {
//     std::unordered_map<uint64_t, std::unique_ptr<Game>> games;
//     std::unordered_map<std::string, uint64_t> users;
//     IDGen id_generator;

//   public:
//     using SleepyDiscord::DiscordClient::DiscordClient;

//     /**
//      * Get the string hash of a user
//      */
//     std::string hash_user(SleepyDiscord::User user) {
//         return user.username + user.discriminator;
//     }

//     /**
//      * Run chess bot on a separate thread
//      */
//     void bot_moves(chess::Board &board,
//                    SleepyDiscord::Snowflake<SleepyDiscord::Channel>
//                    channelID) {
//         chess::Brainiac bot;
//         chess::Move move = bot.move(board);
//         sendMessage(channelID, ";move " + move.standard_notation());
//     }

//     /**
//      * Delete an active chess game
//      */
//     void delete_game(SleepyDiscord::User user) {
//         std::string hash = hash_user(user);
//         uint64_t game_id = users[hash];
//         Game &game = *games[game_id];

//         users.erase(hash_user(game.white));
//         users.erase(hash_user(game.black));

//         // Reuse the id if possible
//         games.erase(game_id);
//         id_generator.unregister_id(game_id);
//     }

//     /**
//      * Play command
//      *
//      * User initiates a game
//      */
//     void on_play(SleepyDiscord::Message &message,
//                  std::vector<std::string> &params) {
//         std::vector<SleepyDiscord::User> &mentions = message.mentions;
//         if (mentions.size() != 1) {
//             sendMessage(message.channelID,
//                         "You must tag someone to play against them.");
//             return;
//         }

//         // Make sure both players are not in a game
//         if (users.count(hash_user(message.author)) ||
//             users.count(hash_user(mentions[0]))) {
//             sendMessage(message.channelID,
//                         "One or both players are already in a game.");
//             return;
//         }

//         // Register a new game and assign its players
//         uint64_t game_id = id_generator.get_id();
//         users[hash_user(message.author)] = game_id;
//         users[hash_user(mentions[0])] = game_id;

//         // Assign each player a color (sender is white by default)
//         std::unique_ptr<Game> game_ptr;
//         if (params.size() > 0 && params[0] == "b") {
//             game_ptr = std::make_unique<Game>(mentions[0], message.author);
//         } else {
//             game_ptr = std::make_unique<Game>(message.author, mentions[0]);
//         }
//         games[game_id] = std::move(game_ptr);
//         Game &game = *games[game_id];
//         game.bot = mentions[0].ID == getID();

//         // Send the messages and display the board
//         sendMessage(message.channelID,
//                     "@" + message.author.username + " challenges @" +
//                         mentions[0].username + " to a chess battle! \u265A");
//         sendMessage(message.channelID, "Game start!");

//         std::string image_filename =
//             "../boards/" + std::to_string(game_id) + ".png";
//         generate_image(game.board, image_filename);
//         uploadFile(message.channelID, image_filename,
//                    "@" + game.white.username + "'s turn.");

//         // Handle bot making the first move
//         if (game.bot && game.white.ID == getID()) {
//             std::thread move(&ChessAI::bot_moves, std::ref(*this),
//                              std::ref(game.board), message.channelID);
//             move.join();
//         }
//     }

//     /**
//      * Move command
//      *
//      * User submits a move for a game
//      */
//     void on_move(SleepyDiscord::Message &message,
//                  std::vector<std::string> &params) {
//         std::string player_key = hash_user(message.author);

//         // Sanity checking
//         if (params.size() != 1) {
//             sendMessage(message.channelID, "Invalid move format.");
//             return;
//         }
//         if (users.count(player_key) == 0) {
//             sendMessage(message.channelID,
//                         "You must challenge someone to a game first.");
//             return;
//         }

//         uint64_t game_id = users[player_key];
//         Game &game = *games[game_id];

//         // Ensure that it's this player's turn
//         if ((game.board.get_turn() == chess::Color::White &&
//              message.author.ID != game.white.ID) ||
//             (game.board.get_turn() == chess::Color::Black &&
//              message.author.ID != game.black.ID)) {
//             sendMessage(message.channelID,
//                         "Impatient! Wait for your turn.. :angry:");
//             return;
//         }

//         // Parse move input
//         std::string move_input = params[0];
//         std::string from = move_input.substr(0, 2);
//         std::string to = move_input.substr(2, 2);
//         char promotion = 0;
//         if (move_input.length() == 5) {
//             promotion = move_input[4];
//         }
//         chess::Move move = game.board.create_move(chess::Square(from),
//                                                   chess::Square(to),
//                                                   promotion);

//         // Execute the move
//         if (!move.is_invalid()) {
//             game.board.execute_move(move);

//             std::string image_filename =
//                 "../boards/" + std::to_string(game_id) + ".png";
//             std::string caption = "@" + message.author.username + " played "
//             +
//                                   move.standard_notation() + "\n";
//             caption += "@" +
//                        (game.board.get_turn() == chess::Color::White
//                             ? game.white.username
//                             : game.black.username) +
//                        "'s turn.";
//             generate_image(game.board, image_filename);
//             uploadFile(message.channelID, image_filename, caption);
//         } else {
//             sendMessage(message.channelID, "Invalid move! :angry:");
//         }

//         // Send messages based on board state
//         if (game.board.is_check()) {
//             sendMessage(message.channelID, "Check! Defend your king!
//             \u265A");
//         } else if (game.board.is_checkmate()) {
//             std::string winner = (game.board.get_turn() ==
//             chess::Color::White)
//                                      ? game.black.username
//                                      : game.white.username;
//             sendMessage(message.channelID,
//                         "Checkmate! @" + winner +
//                             " wins! :party: :party: :party:
//                             :confetti_ball:");
//             delete_game(message.author);
//         } else if (game.board.is_draw()) {
//             sendMessage(message.channelID,
//                         "It's a draw! :party: :party: :party:
//                         :confetti_ball:");
//             delete_game(message.author);
//         } else if (game.bot && message.author.ID != getID()) {
//             // Handle bot response if it is the other player
//             std::thread response(&ChessAI::bot_moves, std::ref(*this),
//                                  std::ref(game.board), message.channelID);
//             response.join();
//         }
//     }

//     /**
//      * Board command
//      *
//      * User wants to display the board
//      */
//     void on_board(SleepyDiscord::Message &message,
//                   std::vector<std::string> &params) {
//         std::string player = hash_user(message.author);
//         if (users.count(player) == 0) {
//             sendMessage(message.channelID,
//                         "You must challenge someone to a game first.");
//             return;
//         }
//         uint64_t game_id = users[player];
//         Game &game = *games[game_id];
//         std::string image_filename =
//             "../boards/" + std::to_string(game_id) + ".png";
//         generate_image(game.board, image_filename);
//         uploadFile(message.channelID, image_filename,
//                    game.board.generate_fen());
//     }

//     /**
//      * Resign command
//      *
//      * User wants to terminate a game
//      */
//     void on_resign(SleepyDiscord::Message &message,
//                    std::vector<std::string> &params) {
//         if (users.count(hash_user(message.author)) == 0) {
//             sendMessage(message.channelID,
//                         "You must challenge someone to a game first.");
//             return;
//         }
//         sendMessage(message.channelID, "Game over. " +
//         message.author.username +
//                                            " resigned. :frowning2:");
//         delete_game(message.author);
//     }

//     /**
//      * Discord message receiver
//      */
//     void onMessage(SleepyDiscord::Message message) override {
//         if (!message.startsWith(";")) {
//             return;
//         }
//         std::vector<std::string> tokens =
//             chess::util::tokenize(message.content, ' ');
//         std::string command = tokens[0].substr(1, tokens[0].length() - 1);
//         std::vector<std::string> params = {tokens.begin() + 1, tokens.end()};

//         if (command == "play") {
//             on_play(message, params);
//         } else if (command == "move") {
//             on_move(message, params);
//         } else if (command == "board") {
//             on_board(message, params);
//         } else if (command == "resign") {
//             on_resign(message, params);
//         }
//     }
// };

/**
 * Test if a query prepends a string
 */
bool prepends(std::string line, std::string query) {
    int n = line.length();
    int m = query.length();
    if (m > n)
        return false;

    for (int i = 0; i < m; i++) {
        if (line[i] != query[i])
            return false;
    }
    return true;
}

/**
 * Fetch the Discord API key
 */
std::string fetch_API_key(std::string const &key) {
    std::ifstream env(
        ".env"); // .env should be in the same directory as the executable
    std::string line;
    while (std::getline(env, line)) {
        if (prepends(line, key + "=")) {
            env.close();
            return line.substr(key.length() + 1);
        }
    }
    env.close();
    return "";
}

/**
 * Entry function
 */
int main() { return 0; }
