// config.hpp

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <iomanip>
#include <string>
#include <list>
#include <random>
#include <boost/dynamic_bitset.hpp>

namespace config {
    // === Debug Parameters ===
    constexpr std::size_t DEBUG = 1; // 0: no prints, 1: verbose debug prints  
    constexpr std::size_t PRINT_FORMAT_BINARY = 0;  // 1 = binary, 0 = decimal

    // === Base Parameters ===
    // Bimodal
    constexpr std::size_t BM_INDEX_WIDTH = 15; // Number of Index bits
    constexpr std::size_t BM_CTR_WIDTH = 3; // n-bit counter
    // Cache
    constexpr std::size_t CACHE_N_INDEX_WIDTH = 12; // Index field width of N-th Cache/Table
    constexpr std::size_t CACHE_N_ASSOC = 4; // Associativity of N-th Cache/Table
    constexpr std::size_t CACHE_N_TAG_WIDTH = 11; // Tag field width in N-th Cache/Table
    constexpr std::size_t CACHE_N_CTR_WIDTH = 3; // Counter field width in N-th Cache/Table
    constexpr std::size_t CACHE_N_U_WIDTH = 2; // Usefulness field width in N-th Cache/Table
    constexpr std::size_t CACHE_N_U_RST_CNT = 250000; // Branch count to reset the usefulness counters
    constexpr std::size_t CACHE_N_VALID_WIDTH = 1; // Valid field width in N-th Cache/Table
    // Pattern History
    constexpr std::size_t PHR_WIDTH = 388; // Number of branch history bits stored by the PHR 
    // Budget
    constexpr std::size_t EXPECTED_HW_BUDGET_BITS = 192*1024*8; // Hardware budget in bits

    // === Derived Parameters ===
    constexpr std::size_t CACHE_N_LRU_BITS = (CACHE_N_ASSOC > 1) ? log2(CACHE_N_ASSOC) : 1; // Number of LRU bits
    constexpr std::size_t CACHE_N_LRU_MAX = (1 << CACHE_N_LRU_BITS) - 1; // Maximum LRU count 
    constexpr std::size_t CACHE_N_SET = 1 << CACHE_N_INDEX_WIDTH; // Number of Sets in N-th Cache/Table

    // === Derived Functions ===
    constexpr std::size_t log2(std::size_t n, std::size_t p = 0) { // Recursively find log2(x), for backward compatibility
        return (n <= 1) ? p : log2(n >> 1, p + 1);
    }

}

void printConfig() {
    constexpr int label_width = 25;
    constexpr int value_width = 10;

    std::cout << "\n";
    std::cout << "====================[ CONFIGURATION SUMMARY ]====================\n";
    std::cout << std::left;

    auto print_entry = [&](const std::string& name, std::size_t value) {
        std::cout << std::setw(label_width) << name
                  << ": " << std::right << std::setw(value_width) << value << "\n";
        std::cout << std::left;
    };

    // Debug Settings
    std::cout << "\n-- Debug Parameters --\n";
    print_entry("DEBUG", config::DEBUG);
    print_entry("PRINT_FORMAT_BINARY", config::PRINT_FORMAT_BINARY);

    // Base Parameters
    std::cout << "\n-- Base Parameters --\n";
    print_entry("BM_INDEX_WIDTH", config::BM_INDEX_WIDTH);
    print_entry("BM_CTR_WIDTH", config::BM_CTR_WIDTH);
    print_entry("CACHE_N_INDEX_WIDTH", config::CACHE_N_INDEX_WIDTH);
    print_entry("CACHE_N_ASSOC", config::CACHE_N_ASSOC);
    print_entry("CACHE_N_TAG_WIDTH", config::CACHE_N_TAG_WIDTH);
    print_entry("PHR_WIDTH", config::PHR_WIDTH);
    print_entry("CACHE_N_CTR_WIDTH", config::CACHE_N_CTR_WIDTH);
    print_entry("CACHE_N_U_WIDTH", config::CACHE_N_U_WIDTH);
    print_entry("CACHE_N_U_RST_CNT", config::CACHE_N_U_RST_CNT);
    print_entry("CACHE_N_VALID_WIDTH", config::CACHE_N_VALID_WIDTH);
    
    // Derived Parameters
    std::cout << "\n-- Derived Parameters --\n";
    print_entry("CACHE_N_SET", config::CACHE_N_SET);
    
    std::cout << "=================================================================\n";
}

/**
 * @brief Prints a hardware budget table using constexpr values from config.hpp.
 * 
 * Includes breakdown of Bimodal Predictor, Branch History Tables (BHT), and Pattern History Register (PHR).
 * Ends with a total sum and compares against expected budget.
 * 
 * @param colSep Column separator (e.g., "|")
 * @param rowSep Row separator character (e.g., "-")
 * @param pad    Padding between columns
 */
 void printBudget(const std::string& colSep = "|", char rowSep = '-', int pad = 1) {
    using namespace config;

    unsigned int bimodalBits = config::BM_CTR_WIDTH * (1 << config::BM_INDEX_WIDTH);
    unsigned int pht_N_bits = config::CACHE_N_ASSOC * (1 << config::CACHE_N_INDEX_WIDTH) 
                            * (config::CACHE_N_CTR_WIDTH + config::CACHE_N_TAG_WIDTH + config::CACHE_N_U_WIDTH + config::CACHE_N_VALID_WIDTH);
    unsigned int phrBits = config::PHR_WIDTH;
    unsigned int totalBits = bimodalBits + pht_N_bits + pht_N_bits + pht_N_bits + phrBits;

    // Column widths
    constexpr int w1 = 35;
    constexpr int w2 = 38;
    constexpr int w3 = 46;

    auto printRow = [&](const std::string& a, const std::string& b, const std::string& c) {
        std::cout << colSep << std::setw(w1) << std::left << std::string(pad, ' ') + a + std::string(pad, ' ')
                  << colSep << std::setw(w2) << std::left << std::string(pad, ' ') + b + std::string(pad, ' ')
                  << colSep << std::setw(w3) << std::left << std::string(pad, ' ') + c + std::string(pad, ' ')
                  << colSep << "\n";
    };

    auto printDivider = [&]() {
        std::cout << colSep << std::string(w1, rowSep)
                  << colSep << std::string(w2, rowSep)
                  << colSep << std::string(w3, rowSep)
                  << colSep << "\n";
    };

    auto printCenteredHeader = [&](const std::string& title, char fill = '=', int pad = 2) {
        int totalWidth = w1 + w2 + w3 + 4 * colSep.length(); // match table width
        int titleLen = static_cast<int>(title.length());
        int sideLen = (totalWidth - titleLen - 2); // minus 2 for brackets
    
        int sideLeft = sideLen / 2;
        int sideRight = sideLen - sideLeft;
    
        std::cout << std::string(sideLeft, fill)
                  << "[" << title << "]"
                  << std::string(sideRight, fill)
                  << "\n";
    };

    printCenteredHeader("HARDWARE BUDGET");
    printRow("Component", "Equation", "Description");
    printDivider();
    printRow("Bimodal Predictor", std::to_string(config::BM_CTR_WIDTH) + " * 2^" + std::to_string(config::BM_INDEX_WIDTH) + 
             " = " + std::to_string(bimodalBits), "BM_CTR_WIDTH * 2^BM_INDEX_WIDTH");
    printDivider();
    printRow("Pattern History Table - 1", std::to_string(config::CACHE_N_ASSOC) + " x 2^" + std::to_string(config::CACHE_N_INDEX_WIDTH) + 
             " x (" + std::to_string(config::CACHE_N_CTR_WIDTH) + " + " + std::to_string(config::CACHE_N_TAG_WIDTH) + " + " + std::to_string(config::CACHE_N_U_WIDTH) +
             " + " + std::to_string(config::CACHE_N_VALID_WIDTH) +
             ") = " + std::to_string(pht_N_bits), "CACHE_N_ASSOC + 2^CACHE_N_INDEX_WIDTH");
    printRow("", "", " * (CACHE_N_CTR_WIDTH + CACHE_N_TAG_WIDTH");
    printRow("", "", "    + CACHE_N_U_WIDTH + CACHE_N_VALID_WIDTH)");
    printDivider();
    printRow("Pattern History Table - 2", std::to_string(config::CACHE_N_ASSOC) + " x 2^" + std::to_string(config::CACHE_N_INDEX_WIDTH) + 
             " x (" + std::to_string(config::CACHE_N_CTR_WIDTH) + " + " + std::to_string(config::CACHE_N_TAG_WIDTH) + " + " + std::to_string(config::CACHE_N_U_WIDTH) +
             " + " + std::to_string(config::CACHE_N_VALID_WIDTH) +
             ") = " + std::to_string(pht_N_bits), "CACHE_N_ASSOC + 2^CACHE_N_INDEX_WIDTH");
    printRow("", "", " * (CACHE_N_CTR_WIDTH + CACHE_N_TAG_WIDTH");
    printRow("", "", "    + CACHE_N_U_WIDTH + CACHE_N_VALID_WIDTH)");
    printDivider();
    printRow("Pattern History Table - 3", std::to_string(config::CACHE_N_ASSOC) + " x 2^" + std::to_string(config::CACHE_N_INDEX_WIDTH) + 
             " x (" + std::to_string(config::CACHE_N_CTR_WIDTH) + " + " + std::to_string(config::CACHE_N_TAG_WIDTH) + " + " + std::to_string(config::CACHE_N_U_WIDTH) +
             " + " + std::to_string(config::CACHE_N_VALID_WIDTH) +
             ") = " + std::to_string(pht_N_bits), "CACHE_N_ASSOC + 2^CACHE_N_INDEX_WIDTH");
    printRow("", "", " * (CACHE_N_CTR_WIDTH + CACHE_N_TAG_WIDTH");
    printRow("", "", "    + CACHE_N_U_WIDTH + CACHE_N_VALID_WIDTH)");
    printDivider();
    printRow("Pattern History Register", std::to_string(config::PHR_WIDTH), "PHR_WIDTH");
    printDivider();

    // Total row
    printRow("Total Budget", std::to_string(totalBits) + " bits (" + std::to_string(totalBits/(1024.0*8)) + " KB)", "");

    // Expected
    printRow("Expected Budget", std::to_string(config::EXPECTED_HW_BUDGET_BITS) + " bits (" + std::to_string(config::EXPECTED_HW_BUDGET_BITS/(1024.0*8)) + " KB)", "");

    printDivider();
}

#include "bimodal.c"
#include "cache.c"
#include "phr.c"
#include "pht.c"
#include "helper.c"

#endif // CONFIG_HPP