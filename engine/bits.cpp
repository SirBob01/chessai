#include "bits.h"

namespace chess {
    void print_bitboard(uint64_t bitboard) {
        std::bitset<64> bitarray(bitboard);
        for(int i = 0; i < 64; i++) {
            std::cout << (bitarray[i] ? "o " : ". ");
            if((i+1)%8 == 0) std::cout << "\n";
        }
        std::cout << "\n";
    }
}