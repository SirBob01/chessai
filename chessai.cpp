#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "util/stb_image.h"
#include "util/stb_image_write.h"
#include "vendor/Brainiac/engine/brainiac.h"
#include "vendor/sleepy-discord/include/sleepy_discord/sleepy_discord.h"

/**
 * RGBA color value in the range [0.0 - 1.0]
 */
struct Color {
    double r;
    double g;
    double b;
    double a;
};

/**
 * Image class
 */
struct Image {
    int width;
    int height;
    int channels;

    unsigned char *data;
    bool from_file;

    Image(int width, int height) : width(width), height(height) {
        from_file = false;
        data = new unsigned char[(width * height) * 4];
        for (int i = 0; i < (width * height) * 4; i++) {
            data[i] = 0;
        }
    }

    Image(std::string filename) {
        int desired_channels = 4; // rgba
        data = stbi_load(filename.c_str(), &width, &height, &channels,
                         desired_channels);
        assert(data);
        from_file = true;
    }

    ~Image() {
        if (from_file) {
            stbi_image_free(data);
        } else {
            delete[] data;
        }
    }

    /**
     * Get the color of a pixel
     */
    Color get_at(int x, int y) {
        int start_i = y * (width * 4) + (x * 4);
        if (start_i > (width * height * 4))
            return {0, 0, 0, 0};
        return {
            data[start_i] / 255.0,
            data[start_i + 1] / 255.0,
            data[start_i + 2] / 255.0,
            data[start_i + 3] / 255.0,
        };
    }

    /**
     * Draw a color over a pixel with alpha blending
     */
    void draw_at(Color color, int x, int y) {
        int start_i = y * (width * 4) + (x * 4);
        if (start_i > (width * height * 4))
            return;
        Color current = get_at(x, y);
        double a0 = color.a + current.a * (1 - color.a);
        Color blended = {
            (color.r * color.a + current.r * current.a * (1 - color.a)) / a0,
            (color.g * color.a + current.g * current.a * (1 - color.a)) / a0,
            (color.b * color.a + current.b * current.a * (1 - color.a)) / a0,
            a0,
        };

        data[start_i] = 255 * blended.r;
        data[start_i + 1] = 255 * blended.g;
        data[start_i + 2] = 255 * blended.b;
        data[start_i + 3] = 255 * blended.a;
    }

    /**
     * Overwrite the color of a pixel
     */
    void set_at(Color color, int x, int y) {
        int start_i = y * (width * 4) + (x * 4);
        if (start_i > (width * height * 4))
            return;

        data[start_i] = 255 * color.r;
        data[start_i + 1] = 255 * color.g;
        data[start_i + 2] = 255 * color.b;
        data[start_i + 3] = 255 * color.a;
    }

    /**
     * Fill the image with a color
     */
    void fill(Color color) {
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                set_at(color, col, row);
            }
        }
    }

    /**
     * Draw another image from the top left corner
     */
    void draw(Image *image, int x, int y) {
        for (int row = 0; row < image->height; row++) {
            for (int col = 0; col < image->width; col++) {
                draw_at(image->get_at(col, row), x + col, y + row);
            }
        }
    }

    /**
     * Save an image to disk (as a png)
     */
    void save(std::string filename) {
        int success =
            stbi_write_png(filename.c_str(), width, height, 4, data, 4 * width);
        assert(success);
    }
};

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

/**
 * Generates a unique ID and reuses old ones
 */
class IDGen {
    std::vector<uint64_t> _unused;
    uint64_t _id_counter = 0;

  public:
    uint64_t get_id() {
        if (_unused.size()) {
            uint64_t id = _unused.back();
            _unused.pop_back();
            if (_unused.size() == _unused.capacity() / 4) {
                _unused.shrink_to_fit();
            }
            return id;
        }
        return _id_counter++;
    }

    void unregister_id(uint64_t id) { _unused.push_back(id); }
};

/**
 * Represents a single game
 */
struct Game {
    SleepyDiscord::User white;
    SleepyDiscord::User black;
    chess::Board board;
    bool bot = false;
};

/**
 * Discord bot client running main game loop
 */
class ChessAI : public SleepyDiscord::DiscordClient {
    std::unordered_map<uint64_t, Game> games;
    std::unordered_map<std::string, uint64_t> users;
    IDGen id_generator;

  public:
    using SleepyDiscord::DiscordClient::DiscordClient;

    /**
     * Get the string hash of a user
     */
    std::string hash_user(SleepyDiscord::User user) {
        return user.username + user.discriminator;
    }

    /**
     * Delete an active chess game
     */
    void delete_game(SleepyDiscord::User user) {
        std::string hash = hash_user(user);
        uint64_t game_id = users[hash];
        Game &game = games[game_id];

        users.erase(hash_user(game.white));

        // In case user is playing against themself
        if (hash_user(game.white) != hash_user(game.black)) {
            users.erase(hash_user(game.black));
        }

        // Reuse the id if possible
        games[game_id] = {};
        id_generator.unregister_id(game_id);
    }

    /**
     * Play command
     *
     * User initiates a game
     */
    void on_play(SleepyDiscord::Message &message,
                 std::vector<std::string> &params) {
        std::vector<SleepyDiscord::User> &mentions = message.mentions;
        if (mentions.size() != 1) {
            sendMessage(message.channelID,
                        "You must tag someone to play against them.");
            return;
        }

        // Make sure both players are not in a game
        // If playing against the bot, only make sure that the sender is not
        // currently in a game
        if (users.count(hash_user(message.author)) ||
            (mentions[0].ID != getID() &&
             users.count(hash_user(mentions[0])))) {
            sendMessage(message.channelID,
                        "One or both players are already in a game.");
            return;
        }

        // Register a new game and assign its players
        uint64_t game_id = id_generator.get_id();
        Game game;
        game.bot = mentions[0].ID == getID();
        games[game_id] = game;
        users[hash_user(message.author)] = game_id;
        users[hash_user(mentions[0])] = game_id;

        // Assign each player a color (sender is white by default)
        if (params.size() == 1 && params[0] == "b") {
            game.black = message.author;
            game.white = mentions[0];
        } else {
            game.white = message.author;
            game.black = mentions[0];
        }

        // Send the messages and display the board
        sendMessage(message.channelID,
                    "@" + message.author.username + " challenges @" +
                        mentions[0].username + " to a chess battle! \u265A");
        sendMessage(message.channelID, "Game start!");

        std::string image_filename =
            "../boards/" + std::to_string(game_id) + ".png";
        generate_image(game.board, image_filename);
        uploadFile(message.channelID, image_filename,
                   "@" + game.white.username + "'s turn.");

        // Handle bot making the first move
        if (game.bot && game.white.ID == getID()) {
            std::thread move(&ChessAI::bot_moves, std::ref(*this),
                             std::ref(games[game_id].board), message.channelID);
            move.join();
        }
    }

    /**
     * Move command
     *
     * User submits a move for a game
     */
    void on_move(SleepyDiscord::Message &message,
                 std::vector<std::string> &params) {
        std::string player_key = hash_user(message.author);

        // Sanity checking
        if (params.size() != 1) {
            sendMessage(message.channelID, "Invalid move format.");
            return;
        }
        if (users.count(player_key) == 0) {
            sendMessage(message.channelID,
                        "You must challenge someone to a game first.");
            return;
        }

        uint64_t game_id = users[player_key];
        Game &game = games[game_id];

        // Ensure that it's this player's turn
        if ((game.board.get_turn() == chess::Color::White &&
             message.author.ID != game.white.ID) ||
            (game.board.get_turn() == chess::Color::Black &&
             message.author.ID != game.black.ID)) {
            sendMessage(message.channelID,
                        "Impatient! Wait for your turn.. :angry:");
            return;
        }

        // Parse move input
        std::string move_input = params[0];
        std::string from = move_input.substr(0, 2);
        std::string to = move_input.substr(2, 2);
        char promotion = 0;
        if (move_input.length() == 5) {
            promotion = move_input[4];
        }
        chess::Move move = game.board.create_move(chess::Square(from),
                                                  chess::Square(to), promotion);

        // Execute the move
        if (!move.is_invalid()) {
            game.board.execute_move(move);

            std::string image_filename =
                "../boards/" + std::to_string(game_id) + ".png";
            std::string caption = "@" + message.author.username + " played " +
                                  move.standard_notation() + "\n";
            caption += "@" +
                       (game.board.get_turn() == chess::Color::White
                            ? game.white.username
                            : game.black.username) +
                       "'s turn.";
            generate_image(game.board, image_filename);
            uploadFile(message.channelID, image_filename, caption);
        } else {
            sendMessage(message.channelID, "Invalid move! :angry:");
        }

        // Send messages based on board state
        if (game.board.is_check()) {
            sendMessage(message.channelID, "Check! Defend your king! \u265A");
        } else if (game.board.is_checkmate()) {
            std::string winner = (game.board.get_turn() == chess::Color::White)
                                     ? game.black.username
                                     : game.white.username;
            sendMessage(message.channelID,
                        "Checkmate! @" + winner +
                            " wins! :party: :party: :party: :confetti_ball:");
            delete_game(message.author);
        } else if (game.board.is_draw()) {
            sendMessage(message.channelID,
                        "It's a draw! :party: :party: :party: :confetti_ball:");
            delete_game(message.author);
        } else if (game.bot && message.author.ID != getID()) {
            // Handle bot response if it is the other player
            std::thread response(&ChessAI::bot_moves, std::ref(*this),
                                 std::ref(game.board), message.channelID);
            response.join();
        }
    }

    /**
     * Run chess bot on a separate thread
     */
    void bot_moves(chess::Board &board,
                   SleepyDiscord::Snowflake<SleepyDiscord::Channel> channelID) {
        chess::Brainiac bot;
        chess::Move move = bot.move(board);
        sendMessage(channelID, ";move " + move.standard_notation());
    }

    /**
     * Board command
     *
     * User wants to display the board
     */
    void on_board(SleepyDiscord::Message &message,
                  std::vector<std::string> &params) {
        std::string player = hash_user(message.author);
        if (users.count(player) == 0) {
            sendMessage(message.channelID,
                        "You must challenge someone to a game first.");
            return;
        }
        uint64_t game_id = users[player];
        Game &game = games[game_id];
        std::string image_filename =
            "../boards/" + std::to_string(game_id) + ".png";
        generate_image(game.board, image_filename);
        uploadFile(message.channelID, image_filename,
                   game.board.generate_fen());
    }

    /**
     * Resign command
     *
     * User wants to terminate a game
     */
    void on_resign(SleepyDiscord::Message &message,
                   std::vector<std::string> &params) {
        if (users.count(hash_user(message.author)) == 0) {
            sendMessage(message.channelID,
                        "You must challenge someone to a game first.");
            return;
        }
        sendMessage(message.channelID, "Game over. " + message.author.username +
                                           " resigned. :frowning2:");
        delete_game(message.author);
    }

    /**
     * Discord message receiver
     */
    void onMessage(SleepyDiscord::Message message) override {
        if (!message.startsWith(";")) {
            return;
        }
        std::vector<std::string> tokens =
            chess::util::tokenize(message.content, ' ');
        std::string command = tokens[0].substr(1, tokens[0].length() - 1);
        std::vector<std::string> params = {tokens.begin() + 1, tokens.end()};

        if (command == "play") {
            on_play(message, params);
        } else if (command == "move") {
            on_move(message, params);
        } else if (command == "board") {
            on_board(message, params);
        } else if (command == "resign") {
            on_resign(message, params);
        }
    }
};

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
int main() {
    ChessAI client(fetch_API_key("DISCORD_API_KEY"),
                   SleepyDiscord::USER_CONTROLED_THREADS);
    client.run();
    return 0;
}