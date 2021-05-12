#ifndef CHESS_UTIL_H_
#define CHESS_UTIL_H_

#include <vector>
#include <string>

namespace chess::util {
    /**
     * Tokenize a string by splitting on a delimiter.
     */
    std::vector<std::string> tokenize(std::string base, char delimiter);
}

#endif