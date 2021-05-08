#include <iostream>
#include <fstream>
#include <cstdlib>
#include "sleepy_discord/sleepy_discord.h"

class ChessAI : public SleepyDiscord::DiscordClient {
public:
    using SleepyDiscord::DiscordClient::DiscordClient;
    void onMessage(SleepyDiscord::Message message) override {
        if (message.startsWith("`")) {
            sendMessage(message.channelID, "Hello " + message.author.username);
            std::cout << message.content << "\n";
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