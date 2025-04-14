#include <bitset>
#include <cstdint>
#include <iostream>
#include <cmath>
#include <vector>
#include <cassert>
#include "config.hpp"

/**
 * @brief Computes a 9-bit folded index from the Path History Register (PHR) and Program Counter (PC).
 *
 * The folding operation is based on the Alder Lake folding scheme:
 * - Index[7:0] is computed by XOR-ing selected bits from the PHR, based on configurable ranges of i and j.
 * - Index[8] is derived from PC[5].
 *
 * Index[7:0] =    (PHR[16i + 8, 16i + 6, ..., 16i - 6]) 
 *              ⊕ (PHR[16j + 1, 16j - 1, ..., 16j - 13])
 * 
 *  Example for i = 1, j = 1:
 *  Index[7:0] = XOR of:
 *      PHR[24, 22, 20, 18, 16, 14, 12, 10]
 *  ⊕  PHR[17, 15, 13, 11,  9,  7,  5,  3]
 *
 * @param PHR The Path History Register as a std::bitset<PHR_WIDTH>.
 * @param PC The Program Counter.
 * @param i_min Minimum value of i (inclusive) for folding.
 * @param i_max Maximum value of i (inclusive) for folding.
 * @param j_min Minimum value of j (inclusive) for folding.
 * @param j_max Maximum value of j (inclusive) for folding.
 * @return A 9-bit std::bitset representing the folded index.
 */
std::bitset<9> foldPHR_3(const std::bitset<config::PHR_WIDTH>& PHR, uint32_t PC,
                       int i_min = 1, int i_max = 11,
                       int j_min = 1, int j_max = 11) {
    std::bitset<8> folded;

    for (int i = i_min; i <= i_max; ++i) {
        for (int k = 0; k < 8; ++k) {
            int pos = 16 * i + 8 - 2 * k;  // PHR[16i + 8, 16i + 6, ..., 16i - 6]
            if (pos < config::PHR_WIDTH) {
                folded[k] = folded[k] ^ PHR[pos];
            }
        }
    }

    for (int j = j_min; j <= j_max; ++j) {
        for (int k = 0; k < 8; ++k) {
            int pos = 16 * j + 1 - 2 * k;  // PHR[16j + 1, 16j - 1, ..., 16j - 13]
            if (pos < config::PHR_WIDTH) {
                folded[k] = folded[k] ^ PHR[pos];
            }
        }
    }

    // Set the 9th bit as PC[5]
    std::bitset<9> finalIndex;
    for (int k = 0; k < 8; ++k) {
        finalIndex[k] = folded[k];
    }
    finalIndex[8] = (PC >> 5) & 0x1;

    return finalIndex;
}

/**
 * @brief Computes a 9-bit folded index from the Path History Register (PHR) and Program Counter (PC).
 *
 * The folding operation is based on the Alder Lake folding scheme:
 * - Index[7:0] is computed by XOR-ing selected bits from the PHR, based on configurable ranges of i and j.
 * - Index[8] is derived from PC[5].
 *
 * Index[7:0] =    (PHR[16i + 8, 16i + 6, ..., 16i - 6]) 
 *              ⊕ (PHR[16j + 1, 16j - 1, ..., 16j - 13])
 * 
 *  Example for i = 1, j = 1:
 *  Index[7:0] = XOR of:
 *      PHR[24, 22, 20, 18, 16, 14, 12, 10]
 *  ⊕  PHR[17, 15, 13, 11,  9,  7,  5,  3]
 *
 * @param PHR The Path History Register as a std::bitset<PHR_WIDTH>.
 * @param PC The Program Counter.
 * @param i_min Minimum value of i (inclusive) for folding.
 * @param i_max Maximum value of i (inclusive) for folding.
 * @param j_min Minimum value of j (inclusive) for folding.
 * @param j_max Maximum value of j (inclusive) for folding.
 * @return A 9-bit std::bitset representing the folded index.
 */
std::bitset<9> foldPHR_2(const std::bitset<config::PHR_WIDTH>& PHR, uint32_t PC,
                       int i_min = 3, int i_max = 1,
                       int j_min = 3, int j_max = 1) {
    std::bitset<8> folded;

    for (int i = i_min; i <= i_max; ++i) {
        for (int k = 0; k < 8; ++k) {
            int pos = 16 * i + 8 - 2 * k;  // PHR[16i + 8, 16i + 6, ..., 16i - 6]
            if (pos < config::PHR_WIDTH) {
                folded[k] = folded[k] ^ PHR[pos];
            }
        }
    }

    for (int j = j_min; j <= j_max; ++j) {
        for (int k = 0; k < 8; ++k) {
            int pos = 16 * j + 1 - 2 * k;  // PHR[16j + 1, 16j - 1, ..., 16j - 13]
            if (pos < config::PHR_WIDTH) {
                folded[k] = folded[k] ^ PHR[pos];
            }
        }
    }

    // Set the 9th bit as PC[5]
    std::bitset<9> finalIndex;
    for (int k = 0; k < 8; ++k) {
        finalIndex[k] = folded[k];
    }
    finalIndex[8] = (PC >> 5) & 0x1;

    return finalIndex;
}

/**
 * @brief Computes a 9-bit folded index from the Path History Register (PHR) and Program Counter (PC).
 *
 * The folding operation is based on the Alder Lake folding scheme:
 * - Index[7:0] is computed by XOR-ing selected bits from the PHR, based on configurable ranges of i and j.
 * - Index[8] is derived from PC[5].
 *
 * Index[7:0] =    (PHR[16i + 8, 16i + 6, ..., 16i - 6]) 
 *              ⊕ (PHR[16j + 1, 16j - 1, ..., 16j - 13])
 * 
 *  Example for i = 1, j = 1:
 *  Index[7:0] = XOR of:
 *      PHR[24, 22, 20, 18, 16, 14, 12, 10]
 *  ⊕  PHR[17, 15, 13, 11,  9,  7,  5,  3]
 *
 * @param PHR The Path History Register as a std::bitset<PHR_WIDTH>.
 * @param PC The Program Counter.
 * @return A 9-bit std::bitset representing the folded index.
 */
std::bitset<9> foldPHR_1(const std::bitset<config::PHR_WIDTH>& PHR, uint32_t PC) {
    std::bitset<8> folded;

    for (int k = 0; k < 8; ++k) {
        int pos = 20 - 2 * k;  // PHR[20, 18, 16, ..., 6]
        if (pos < config::PHR_WIDTH) {
            folded[k] = folded[k] ^ PHR[pos];
        }
    }

    for (int k = 0; k < 8; ++k) {
        int pos = 15 - 2 * k;  // PHR[15, 13, 11, ..., 1]
        if (pos < config::PHR_WIDTH) {
            folded[k] = folded[k] ^ PHR[pos];
        }
    }

    // Set the 9th bit as PC[5]
    std::bitset<9> finalIndex;
    for (int k = 0; k < 8; ++k) {
        finalIndex[k] = folded[k];
    }
    finalIndex[8] = (PC >> 5) & 0x1;

    return finalIndex;
}

int main(){
    std::bitset<config::PHR_WIDTH> PHR("1101001101110101011110001011101010111000101110101011100010111010");
    uint32_t PC = 0xA5B3;
    std::cout << "PHR: " << PHR << std::endl;
    std::bitset<9> index = foldPHR_3(PHR, PC);

    std::cout << "Folded Index: " << index << std::endl;
    return 0;
}
