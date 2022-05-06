#include "server.h"

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
            base->draw(piece_image,
                       (file * tiles[0]->width) + x_offset + 32,
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

ChessServer::ChessServer(dpp::cluster &bot) : _bot(bot) {
    bot.on_interaction_create([&bot,
                               this](const dpp::interaction_create_t &event) {
        std::string command = event.command.get_command_name();
        std::vector<dpp::command_data_option> params =
            event.command.get_command_interaction().options;
        if (command == "play") {
            const dpp::command_value &user_param = event.get_parameter("user");
            const dpp::command_value &color_param =
                event.get_parameter("color");

            dpp::user &opponent =
                *dpp::find_user(std::get<dpp::snowflake>(user_param));
            std::string color = "w";
            if (std::holds_alternative<std::string>(color_param)) {
                color = std::get<std::string>(color_param);
            }
            on_play(event, opponent, color);
        } else if (command == "move") {
            const dpp::command_value &move_param = event.get_parameter("move");
            std::string move = std::get<std::string>(move_param);
            on_move(event, move);
        } else if (command == "board") {
            on_board(event);
        } else if (command == "resign") {
            on_resign(event);
        }
    });
};

std::string ChessServer::hash_user(dpp::user user) {
    return user.username + std::to_string(user.discriminator);
}

dpp::message ChessServer::game_info(const dpp::interaction_create_t &event,
                                    Game &game,
                                    std::string message) {
    std::string image_filename =
        "../boards/" + std::to_string(game.id) + ".png";
    generate_image(game.board, image_filename);

    dpp::message msg(event.command.channel_id, message);
    msg.set_file_content(dpp::utility::read_file(image_filename));
    msg.set_filename("board.png");

    dpp::embed embed =
        dpp::embed()
            .set_color(dpp::colors::blue)
            .set_title(game.white.username + " versus " + game.black.username)
            .set_author("Keith Leonardo",
                        "https://keithleonardo.ml",
                        "https://avatars.githubusercontent.com/u/10874047")
            .set_description("Match information")
            .add_field("FEN", game.board.generate_fen())
            .set_image("attachment://board.png");

    msg.add_embed(embed);
    return msg;
}

void ChessServer::delete_game(dpp::user user) {
    std::string hash = hash_user(user);
    uint64_t game_id = _users[hash];
    Game &game = *_games[game_id];

    _users.erase(hash_user(game.white));
    _users.erase(hash_user(game.black));

    // Reuse the id if possible
    _games.erase(game_id);
    _id_generator.unregister_id(game_id);
}

void ChessServer::bot_moves(const dpp::interaction_create_t &event,
                            Game &game) {
    chess::Brainiac bot;
    chess::Move move = bot.move(game.board);
    game.board.execute_move(move);

    dpp::user user;
    if (event.command.usr.id == game.black.id) {
        user = game.black;
    } else {
        user = game.white;
    }
    _bot.message_create(game_info(event,
                                  game,
                                  "<@" + std::to_string(user.id) + "> I move " +
                                      move.standard_notation()));
}

void ChessServer::on_play(const dpp::interaction_create_t &event,
                          dpp::user &opponent,
                          std::string color) {
    // Make sure both players are not in a game
    if (_users.count(hash_user(event.command.usr)) ||
        _users.count(hash_user(opponent))) {
        event.reply("One of you is already in a game!");
        return;
    }

    // Register a new game and assign its players
    uint64_t game_id = _id_generator.get_id();
    _users[hash_user(event.command.usr)] = game_id;
    _users[hash_user(opponent)] = game_id;

    // Assign each player a color (sender is white by default)
    std::unique_ptr<Game> game_ptr;
    if (color == "b") {
        game_ptr = std::make_unique<Game>(opponent, event.command.usr);
    } else {
        game_ptr = std::make_unique<Game>(event.command.usr, opponent);
    }
    _games[game_id] = std::move(game_ptr);
    Game &game = *_games[game_id];
    game.id = game_id;
    game.bot = opponent.id == _bot.me.id;

    event.reply(game_info(event, game));

    // Handle bot making the first move
    if (game.bot && game.white.id == _bot.me.id) {
        std::thread move(&ChessServer::bot_moves,
                         std::ref(*this),
                         std::ref(event),
                         std::ref(game));
        move.join();
    }
}

void ChessServer::on_move(const dpp::interaction_create_t &event,
                          std::string move_input) {
    std::string player = hash_user(event.command.usr);
    if (_users.find(player) == _users.end()) {
        event.reply("You are not currently in a game.");
        return;
    }

    // Ensure that it's this player's turn
    uint64_t game_id = _users[player];
    Game &game = *_games[game_id];
    if ((game.board.get_turn() == chess::Color::White &&
         event.command.usr.id != game.white.id) ||
        (game.board.get_turn() == chess::Color::Black &&
         event.command.usr.id != game.black.id)) {
        event.reply("Impatient! Wait for your turn... :angry:");
        return;
    }

    // Parse move input
    std::string from = move_input.substr(0, 2);
    std::string to = move_input.substr(2, 2);
    char promotion = 0;
    if (move_input.length() == 5) {
        promotion = move_input[4];
    }
    chess::Move move = game.board.create_move(chess::Square(from),
                                              chess::Square(to),
                                              promotion);

    // Execute the move
    if (!move.is_invalid()) {
        game.board.execute_move(move);

        // Send messages based on board state
        std::string message = "";
        if (game.board.is_checkmate()) {
            dpp::snowflake winner =
                (game.board.get_turn() == chess::Color::White) ? game.black.id
                                                               : game.white.id;
            message = "Checkmate! <@" + std::to_string(winner) +
                      "> wins! "
                      ":confetti_ball: :confetti_ball: :confetti_ball:";
            event.reply(game_info(event, game, message));
            delete_game(event.command.usr);
        } else if (game.board.is_draw()) {
            message =
                "It's a draw! :confetti_ball: :confetti_ball: :confetti_ball:";
            event.reply(game_info(event, game, message));
            delete_game(event.command.usr);
        } else {
            if (game.board.is_check()) {
                message = "Check! Defend your king!";
            }
            event.reply(game_info(event, game));
            if (game.bot && event.command.usr.id != _bot.me.id) {
                // Handle bot response if it is the other player
                std::thread response(&ChessServer::bot_moves,
                                     std::ref(*this),
                                     std::ref(event),
                                     std::ref(game));
                response.join();
            }
        }
    } else {
        event.reply("Invalid move! :angry:");
    }
}

void ChessServer::on_board(const dpp::interaction_create_t &event) {
    std::string player = hash_user(event.command.usr);
    if (_users.count(player) == 0) {
        event.reply("You are not currently in a game.");
        return;
    }
    uint64_t game_id = _users[player];
    Game &game = *_games[game_id];
    event.reply(game_info(event, game));
}

void ChessServer::on_resign(const dpp::interaction_create_t &event) {
    std::string player = hash_user(event.command.usr);
    if (_users.count(player) == 0) {
        event.reply("You are not currently in a game.");
        return;
    }
    uint64_t game_id = _users[player];
    Game &game = *_games[game_id];
    dpp::user opponent;
    if (event.command.usr.id == game.black.id) {
        opponent = game.white;
    } else {
        opponent = game.black;
    }
    event.reply("<@" + std::to_string(event.command.usr.id) +
                "> resigned :frowning2:\n"
                "<@" +
                std::to_string(opponent.id) + "> wins by default!");
    delete_game(event.command.usr);
}
