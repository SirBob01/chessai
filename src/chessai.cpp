#include <brainiac.h>
#include <dpp/dpp.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_map>

#include "server.h"

/**
 * Test if a query prepends a string
 */
bool prepends(std::string line, std::string query) {
    int n = line.length();
    int m = query.length();
    if (m > n) return false;

    for (int i = 0; i < m; i++) {
        if (line[i] != query[i]) return false;
    }
    return true;
}

/**
 * Read a key from a .env file
 */
std::string env_get(std::string const &key) {
    // .env should be in the same directory as the executable
    std::ifstream env(".env");
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
    std::string token = env_get("API_KEY");
    dpp::cluster bot(token);
    ChessServer server(bot);
    chess::init();

    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot, &server](const dpp::ready_t &event) {
        std::cout << "ChessAI is online.\n";

        // Create the slash commands
        dpp::slashcommand play("play",
                               "Start a match against another user",
                               bot.me.id);
        play.add_option(dpp::command_option(dpp::co_mentionable,
                                            "user",
                                            "Challenged user mention",
                                            true));
        play.add_option(dpp::command_option(dpp::co_string,
                                            "color",
                                            "Which color do you want to play?",
                                            false)
                            .add_choice(dpp::command_option_choice("w", "w"))
                            .add_choice(dpp::command_option_choice("b", "b")));
        bot.global_command_create(play);

        dpp::slashcommand move("move", "Execute a move in a match", bot.me.id);
        move.add_option(dpp::command_option(dpp::co_string,
                                            "move",
                                            "Standard notation move string",
                                            true));
        bot.global_command_create(move);

        dpp::slashcommand board("board",
                                "Display the state of the match",
                                bot.me.id);
        bot.global_command_create(board);

        dpp::slashcommand resign("resign",
                                 "Resign from the current match",
                                 bot.me.id);
        bot.global_command_create(resign);
    });

    bot.start(false);
    return 0;
}
