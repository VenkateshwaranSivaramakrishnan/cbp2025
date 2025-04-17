#include "config.hpp"

/**
 * @class PatternHistoryRegister
 * @brief Implements a dynamic Path History Register (PHR) for branch prediction.
 *
 * This class models the behavior of a Path History Register used in modern
 * branch prediction schemes (e.g., Intel Alder Lake). It maintains a configurable-width
 * history of recent branch behavior and supports:
 * 
 * - Folding the PHR to generate compact indices for prediction tables.
 * - Computing a microarchitectural "footprint" hash from branch/target pairs.
 * - Updating the PHR on taken branches via a two-step shift and XOR process.
 * - Printing the current PHR state for debugging and visualization.
 *
 * The internal PHR is implemented using a `boost::dynamic_bitset`, allowing
 * runtime-resizable bit-vector manipulation.
 */
class PatternHistoryRegister {
private:
    // Pattern History Register
    boost::dynamic_bitset<> PHR;
public:
    /**
     * @brief Constructs a PatternHistoryRegister with the configured bit width.
     *
     * Initializes the internal Path History Register (PHR) with a width specified
     * at compile-time by `config::PHR_WIDTH`. All bits are initially set to 0.
     */
    PatternHistoryRegister(unsigned int phrWidth) {
        PHR.resize(phrWidth);
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
     * @param PC The Program Counter.
     * @param i_min Minimum value of i (inclusive) for folding.
     * @param i_max Maximum value of i (inclusive) for folding.
     * @param j_min Minimum value of j (inclusive) for folding.
     * @param j_max Maximum value of j (inclusive) for folding.
     * @return A 9-bit std::bitset representing the folded index.
     */
    std::bitset<12> foldPHR_3(uint32_t PC,
                           int i_min = 1, int i_max = 17,
                           int j_min = 1, int j_max = 17) const {
        std::bitset<11> folded;

        for (int i = i_min; i <= i_max; ++i) {
            for (int k = 0; k < 11; ++k) {
                int pos = 22 * i + 12 - 2 * k;  // PHR[16i + 8, 16i + 6, ..., 16i - 6]
                if (pos < config::PHR_WIDTH) {
                    folded[k] = folded[k] ^ PHR[pos];
                }
            }
        }

        for (int j = j_min; j <= j_max; ++j) {
            for (int k = 0; k < 11; ++k) {
                int pos = 22 * j + 1 - 2 * k;  // PHR[16j + 1, 16j - 1, ..., 16j - 13]
                if (pos < config::PHR_WIDTH) {
                    folded[k] = folded[k] ^ PHR[pos];
                }
            }
        }

        // Set the 9th bit as PC[5]
        std::bitset<12> finalIndex;
        for (int k = 0; k < 11; ++k) {
            finalIndex[k] = folded[k];
        }
        finalIndex[11] = (PC >> 5) & 0x1;

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
     * @param PC The Program Counter.
     * @param i_min Minimum value of i (inclusive) for folding.
     * @param i_max Maximum value of i (inclusive) for folding.
     * @param j_min Minimum value of j (inclusive) for folding.
     * @param j_max Maximum value of j (inclusive) for folding.
     * @return A 9-bit std::bitset representing the folded index.
     */
    std::bitset<12> foldPHR_2(uint32_t PC,
                           int i_min = 1, int i_max = 5,
                           int j_min = 1, int j_max = 5) const {
        std::bitset<11> folded;

        for (int i = i_min; i <= i_max; ++i) {
            for (int k = 0; k < 11; ++k) {
                int pos = 22 * i + 12 - 2 * k;  // PHR[16i + 8, 16i + 6, ..., 16i - 6]
                if (pos < config::PHR_WIDTH) {
                    folded[k] = folded[k] ^ PHR[pos];
                }
            }
        }

        for (int j = j_min; j <= j_max; ++j) {
            for (int k = 0; k < 11; ++k) {
                int pos = 22 * j + 1 - 2 * k;  // PHR[16j + 1, 16j - 1, ..., 16j - 13]
                if (pos < config::PHR_WIDTH) {
                    folded[k] = folded[k] ^ PHR[pos];
                }
            }
        }

        // Set the 9th bit as PC[5]
        std::bitset<12> finalIndex;
        for (int k = 0; k < 11; ++k) {
            finalIndex[k] = folded[k];
        }
        finalIndex[11] = (PC >> 5) & 0x1;

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
     * @param PC The Program Counter.
     * @return A 9-bit std::bitset representing the folded index.
     */
    std::bitset<12> foldPHR_1(uint32_t PC) const {
        std::bitset<11> folded;

        for (int k = 0; k < 11; ++k) {
            int pos = 26 - 2 * k;  // PHR[20, 18, 16, ..., 6]
            if (pos < config::PHR_WIDTH) {
                folded[k] = folded[k] ^ PHR[pos];
            }
        }

        for (int k = 0; k < 11; ++k) {
            int pos = 21 - 2 * k;  // PHR[15, 13, 11, ..., 1]
            if (pos < config::PHR_WIDTH) {
                folded[k] = folded[k] ^ PHR[pos];
            }
        }

        // Set the 9th bit as PC[5]
        std::bitset<12> finalIndex;
        for (int k = 0; k < 11; ++k) {
            finalIndex[k] = folded[k];
        }
        finalIndex[11] = (PC >> 5) & 0x1;

        return finalIndex;
    }

    #include <boost/dynamic_bitset.hpp>

    /**
     * @brief Folds a portion of the Pattern History Register (PHR) into a compact index using XOR.
     *
     * This function reduces a segment of the PHR of length L into an index of 'n' bits, by
     * repeatedly XORing segments of 'n' bits until the full history is folded.
     *
     * @param phr The Pattern History Register (as a dynamic_bitset)
     * @param n   Target length to fold into (e.g., 11 bits)
     * @param L   Length of the PHR segment to fold (must be <= phr.size())
     * @return boost::dynamic_bitset<> The folded index of size 'n'
     */
    boost::dynamic_bitset<> foldPHR(uint64_t PC, unsigned int n) const {
        assert(n > 0);

        boost::dynamic_bitset<> folded(config::CACHE_N_INDEX_WIDTH);  // Initialize n-bit folded index to 0

        for (unsigned int i = 0; i < n; ++i) {
            if (PHR[i]) {
                folded[i % config::CACHE_N_INDEX_WIDTH].flip();  // XOR using modular folding
            }
        }

        return folded;
    }


    /**
     * @brief Computes the Alder Lake-style hash of a branch and target address.
     *
     * This function performs a microarchitecture-specific bitwise transformation
     * using a combination of static and XORed bit fields from the branch and target
     * addresses. The pattern follows the Alder Lake predictor hashing logic.
     *
     * @param branch The branch address (as unsigned integer)
     * @param target The target address (as unsigned integer)
     * @return Transformed result as unsigned integer, based on bitfield mapping
     */
    uint16_t footprint(uint32_t branch, uint32_t target) {
       // 16-bit result
       boost::dynamic_bitset<> result(16);
       boost::dynamic_bitset<> B(32, branch);
       boost::dynamic_bitset<> T(32, target);
       // Direct mappings
       result[15] = B[15];
       result[14] = B[14];
       result[13] = B[13];
       result[12] = B[12];
       result[11] = B[11] ^ T[5];
       result[10] = B[2]  ^ T[4];
       result[9]  = B[1]  ^ T[3];
       result[8]  = B[0]  ^ T[2];
       result[7]  = B[10];
       result[6]  = B[9];
       result[5]  = B[8];
       result[4]  = B[7];
       result[3]  = B[6];
       result[2]  = B[5];
       result[1]  = B[4]  ^ T[1];
       result[0]  = B[3]  ^ T[0];
       // Convert bitset to integer
       return static_cast<uint16_t>(result.to_ulong());
    }

    /**
     * @brief Updates the Path History Register (PHR) based on a taken branch.
     *
     * This function performs a two-step update:
     * 1. The entire PHR is shifted left by 2 bits to make room for new history.
     * 2. The lower 16 bits of the PHR are XOR-ed with the 16-bit "footprint" 
     *    derived from the branch and target addresses.
     *
     * This update scheme is based on the Alder Lake folding predictor behavior.
     *
     * @param branch The branch address (source PC).
     * @param target The target address (destination PC).
     */
    void updatePHR(uint32_t branch, uint32_t target, bool resolveDir) {
        if (resolveDir) {
            // Step 1: Shift left by 2 bits
            PHR <<= 2;   
            // Step 2: Compute footprint and XOR into PHR[15:0]
            uint16_t fp = footprint(branch, target);
            for (size_t i = 0; i < 16 && i < PHR.size(); ++i) { 
                PHR[i] = PHR[i] ^ ((fp >> i) & 0x1);
            }
        }
    }

    /**
     * @brief Generates an 11-bit tag by XOR-folding segments of the Path History Register (PHR).
     *
     * The tag is computed by XOR-ing consecutive 11-bit segments of the PHR:
     *   Tag = h[10:0] ⊕ h[21:11] ⊕ h[32:22] ⊕ ... ⊕ h[n:m]
     * 
     * If the final chunk has fewer than 11 bits, it is still included in the XOR
     * by zero-padding the missing bits.
     *
     * @return A std::bitset<11> representing the folded tag.
     */
    std::bitset<11> generateTag() const {
        std::bitset<11> tag;

        size_t offset = 0;
        while (offset < PHR.size()) {
            std::bitset<11> chunk;

            for (size_t i = 0; i < 11 && (offset + i) < PHR.size(); ++i) {
                chunk[i] = PHR[offset + i];
            }

            tag ^= chunk;
            offset += 11;
        }

        return tag;
    }   

    /**
     * @brief Prints the current state of the Path History Register (PHR).
     *
     * This function outputs the PHR content as a binary string to std::cout.
     * The output is printed from the highest index to 0 for intuitive readability.
     */
    void printPHR() const {
        std::cout << "PHR: " << PHR << std::endl;
    }
}; 