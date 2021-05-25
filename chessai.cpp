#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cassert>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "util/stb_image.h"
#include "util/stb_image_write.h"
#include "sleepy_discord/sleepy_discord.h"
#include "engine/board.h"

struct Color {
    double r;
    double g;
    double b;
    double a;
};

struct Image {
    int width;
    int height;
    int channels;

    unsigned char *data;
    bool from_file;

    Image(int width, int height) : width(width), height(height) {
        from_file = false;
        data = new unsigned char[(width * height) * 4];
        for(int i = 0; i < (width * height) * 4; i++) {
            data[i] = 0;
        }
    }

    Image(std::string filename) {
        int desired_channels = 4; // rgba
        data = stbi_load(filename.c_str(), &width, &height, &channels, desired_channels);
        assert(data);
        from_file = true;
    }

    ~Image() {
        if(from_file) {
            stbi_image_free(data);
        }
        else {
            delete[] data;
        }
    }

    /**
     * Get the color of a pixel
     */
    Color get_at(int x, int y) {
        int start_i = y * (width * 4) + (x * 4);
        if(start_i > (width * height * 4)) return {0, 0, 0, 0};
        return {
            data[start_i] / 255.0,
            data[start_i+1] / 255.0,
            data[start_i+2] / 255.0,
            data[start_i+3] / 255.0,
        };
    }

    /**
     * Set the color of a pixel
     */
    void set_at(Color color, int x, int y) {
        int start_i = y * (width * 4) + (x * 4);
        if(start_i > (width * height * 4)) return;
        Color current = get_at(x, y);
        double a0 = color.a + current.a * (1 - color.a);
        Color blended = {
            (color.r * color.a + current.r * current.a * (1 - color.a)) / a0,
            (color.g * color.a + current.g * current.a * (1 - color.a)) / a0,
            (color.b * color.a + current.b * current.a * (1 - color.a)) / a0,
            a0,
        };

        data[start_i] = 255 * blended.r;
        data[start_i+1] = 255 * blended.g;
        data[start_i+2] = 255 * blended.b;
        data[start_i+3] = 255 * blended.a;
    }

    /**
     * Draw another image from the top left corner
     */
    void draw(Image *image, int x, int y) {
        for(int row = 0; row < image->height; row++) {
            for(int col = 0; col < image->width; col++) {
                set_at(image->get_at(col, row), x + col, y + row);
            }
        }
    }

    /**
     * Save an image to disk (as a png)
     */
    void save(std::string filename) {
        int success = stbi_write_png(filename.c_str(), width, height, 4, data, 4*width);
        assert(success);
    }
};


/**
 * Represents a single game
 */
struct Game {
    SleepyDiscord::User white;
    SleepyDiscord::User black;
    chess::Board board;
};


/**
 * Generate a PNG image of the board and save it to disk
 */
void generate_image(chess::Board &board, std::string filename) {
    Image *base = new Image(128*8, 128*8);
    std::vector<Image *> pieces;
    for(int i = 0; i < 12; i++) {
        pieces.push_back(new Image("../images/" + std::to_string(i) + ".png"));
    }
    std::vector<Image *> tiles;
    tiles.push_back(new Image("../images/brown0.png"));
    tiles.push_back(new Image("../images/brown1.png"));

    bool square_white = true; 
    for(int rank = 0; rank < 8; rank++) {
        bool current_color = square_white;
        for(int file = 0; file < 8; file++) {
            // Render appropriate squares
            Image *tile = tiles[current_color];
            base->draw(tile, file * tile->width, rank * tile->height);
            current_color = !current_color;
        }
        square_white = !square_white;
    }
    for(int rank = 7; rank >= 0; rank--) {
        for(int file = 0; file < 8; file++) {
            // Render a the piece on top (if any)
            chess::Piece piece = board.get_at_coords(rank, file);
            if(piece.is_empty()) {
                continue;
            }
            Image *piece_image = pieces[piece.get_piece_index()];

            int x_offset = 0;
            if(piece.type == chess::PieceType::Pawn   || 
               piece.type == chess::PieceType::Knight ||
               piece.type == chess::PieceType::Rook) {
                x_offset = 10;
            }
            base->draw(piece_image, (file * tiles[0]->width) + x_offset, ((7 - rank) * tiles[0]->height));
        }
    }
    base->save(filename);
    for(auto piece : pieces) {
        delete piece;
    }
    for(auto tile : tiles) {
        delete tile;
    }
    delete base;
}

/**
 * Discord bot client running main game loop
 */
class ChessAI : public SleepyDiscord::DiscordClient {
    std::unordered_map<uint64_t, Game> games;
    std::unordered_map<std::string, uint64_t> users;
    uint64_t id_counter = 0;

public:
    using SleepyDiscord::DiscordClient::DiscordClient;
    void onMessage(SleepyDiscord::Message message) override {
        if(!message.startsWith(";")) {
            return;
        }
        auto tokens = chess::util::tokenize(message.content, ' ');
        std::string command = tokens[0].substr(1, tokens[0].length()-1);
        if(command == "play") {
            auto &mentions = message.mentions;
            if(mentions.size() == 0) {
                sendMessage(message.channelID, "You must tag someone to play against them.");
                return;
            }

            std::string white = hash_user(message.author);
            std::string black = hash_user(mentions[0]);
            if(users.count(white) || users.count(black)) {
                sendMessage(message.channelID, "One or both players are already in a game.");
                return;
            }

            // Register a new game and assign its players
            Game g;
            g.white = message.author;
            g.black = mentions[0];
            games[id_counter] = g;
            users[white] = id_counter;
            users[black] = id_counter;
            
            sendMessage(message.channelID, "@" + message.author.username + " challenges @" + mentions[0].username + " to a chess battle! \u265A");
            sendMessage(message.channelID, "Game start!");
            
            std::string image_filename = "../boards/"+std::to_string(id_counter)+".png";
            generate_image(g.board, image_filename);
            uploadFile(message.channelID, image_filename, "");

            sendMessage(message.channelID, "@" + g.white.username + "'s turn.");
            
            id_counter++;
        }
        else if(command == "move") {
            std::string player = hash_user(message.author);
            if(tokens.size() != 2) {
                sendMessage(message.channelID, "Invalid move format.");
                return;
            }
            if(users.count(player) == 0) {
                sendMessage(message.channelID, "You must challenge someone to a game first.");
                return;
            }
            uint64_t game_id = users[player];
            auto &game = games[game_id];
            if((game.board.get_turn() == chess::Color::White && player != hash_user(game.white)) || 
               (game.board.get_turn() == chess::Color::Black && player != hash_user(game.black))) {
                sendMessage(message.channelID, "Impatient! Wait for your turn.. :angry:");
                return;
            }
            std::string move_input = tokens[1];
            std::string from = move_input.substr(0, 2);
            std::string to = move_input.substr(2, 2);
            char promotion = 0;
            if(move_input.length() == 5) {
                promotion = move_input[4];
            }
            chess::Move move = game.board.create_move(chess::Square(from), chess::Square(to), promotion);
            if(!move.is_invalid()) {
                game.board.execute_move(move);

                std::string image_filename = "../boards/"+std::to_string(id_counter)+".png";
                generate_image(game.board, image_filename);
                uploadFile(message.channelID, image_filename, "");

                sendMessage(message.channelID, "@" + message.author.username + " played " + move.standard_notation());
                sendMessage(message.channelID, "@" + (game.board.get_turn() == chess::Color::White ? game.white.username : game.black.username) + "'s turn.");
            }
            else {
                sendMessage(message.channelID, "Invalid move! :angry:");
            }

            if(game.board.is_check()) {
                sendMessage(message.channelID, "Check! Defend your king! \u265A");
            }
            if(game.board.is_checkmate()) {
                std::string winner = (game.board.get_turn() == chess::Color::White) ? game.black.username : game.white.username;
                sendMessage(message.channelID, "Checkmate! @" + winner + " wins! :party: :party: :party: :confetti_ball:");
                delete_game(message.author);
            }
        }
        else if(command == "fen") {
            std::string player = hash_user(message.author);
            if(users.count(player) == 0) {
                sendMessage(message.channelID, "You must challenge someone to a game first.");
                return;
            }
            auto &game = games[users[player]];
            sendMessage(message.channelID, game.board.generate_fen());
        }
        else if(command == "resign") {
            if(users.count(hash_user(message.author)) == 0) {
                sendMessage(message.channelID, "You must challenge someone to a game first.");
                return;
            }
            sendMessage(message.channelID, "Game over. " + message.author.username + " resigned. :frowning2:");
            delete_game(message.author);
        }
    }

    std::string hash_user(SleepyDiscord::User user) {
        return user.username + user.discriminator;
    }

    void delete_game(SleepyDiscord::User user) {
        std::string hash = hash_user(user);
        uint64_t game_id = users[hash];
        auto &game = games[game_id];

        users.erase(hash_user(game.white));
        
        // In case user is playing against themself
        if(hash_user(game.white) != hash_user(game.black)) {
            users.erase(hash_user(game.black));
        }
    }
};

bool prepends(std::string line, std::string query) {
    int n = line.length();
    int m = query.length();
    if(m > n) return false;

    for(int i = 0; i < m; i++) {
        if(line[i] != query[i]) return false;
    }
    return true;
}

std::string fetch_API_key(std::string const &key) {
    std::ifstream env(".env"); // .env should be in the same directory as the executable
    std::string line;
    while(std::getline(env, line)) {
        if(prepends(line, key+"=")) {
            env.close();
            return line.substr(key.length()+1);
        }
    }
    env.close();
    return "";
}

int main() {
    ChessAI client(fetch_API_key("DISCORD_API_KEY"), SleepyDiscord::USER_CONTROLED_THREADS);
    client.run();
    return 0;
}