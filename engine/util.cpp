#include "util.h"

namespace chess::util {
    std::vector<std::string> tokenize(std::string base, char delimiter) {
        std::vector<std::string> tokens;
        std::string current = "";
        for(auto &c : base) {
            if(c == delimiter) {
                if(current.length()) tokens.push_back(current);
                current = "";
            }
            else {
                current += c;
            }
        }
        if(current.length()) tokens.push_back(current);
        return tokens;
    }
}
