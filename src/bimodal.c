/**
 * @class BimodalPredictor
 * 
 * This class represents a bimodal branch predictor indexed using the Program Counter (PC).
 * The predictor uses a table to predict whether branches are taken or not based on historical data.
 */
class BimodalPredictor {
private:
    // Bimodal prediction table, stores 2-bit saturating counters
    std::vector<int> bimodalTable;

    // Number of bits to index the table
    unsigned int indexBits;

    // Number of bits for each entry (size of the saturating counter)
    unsigned int counterBits;

    // Number of entries in the prediction table
    unsigned int tableSize;

    // Helper method to get the index into the prediction table based on the PC
    unsigned int getTableIndex(unsigned int PC) {
        // Extract the lower `indexBits` of the Program Counter to index the prediction table
        // Ignore the last 2 bits (i.e., divide by 4), then extract `indexBits` bits
        return (PC >> 2) & ((1 << indexBits) - 1);
    }

public:
    /**
     * Constructor to initialize the Bimodal predictor.
     * 
     */
    BimodalPredictor(unsigned int indexBits, unsigned int counterBits) {
        this->indexBits = indexBits;
        this->counterBits = counterBits;
        this->tableSize = 1 << indexBits;  // Table size is 2^indexBits
        bimodalTable.resize(tableSize, (1 << (counterBits - 1)));  // Initialize to middle state (weakly yaken for n-bit counter)
    }

    /**
     * Predict the branch outcome based on the given Program Counter (PC).
     * 
     * @param PC: The Program Counter value to index the prediction table
     * @return: A boolean indicating the predicted outcome (true for branch taken, false for not taken)
     */
    bool predict(unsigned int PC) {
        unsigned int index = getTableIndex(PC);
        // A value >= (2^(counterBits - 1)) indicates taken (for an n-bit counter)
        return bimodalTable[index] >= (1 << (counterBits - 1));
    }

    /**
     * Update the prediction table based on the actual outcome of the branch.
     * 
     * @param PC: The Program Counter value to index the prediction table
     * @param actualOutcome: The actual branch outcome (true for taken, false for not taken)
     */
    void update(unsigned int PC, bool actualOutcome) {
        unsigned int index = getTableIndex(PC);
        
        // Update the saturating counter based on actual outcome
        if (actualOutcome) {
            if (bimodalTable[index] < (1 << counterBits) - 1) bimodalTable[index]++;  // Increment counter (taken)
        } else {
            if (bimodalTable[index] > 0) bimodalTable[index]--;  // Decrement counter (not taken)
        }
    }

    /**
     * Display the current state of the prediction table.
     */
    void displayTable() const {
        for (unsigned int i = 0; i < tableSize; ++i) {
            std::cout << "Index " << i << ": " << bimodalTable[i] << std::endl;
        }
    }

    /**
     * @brief Formats an unsigned integer value as a string in binary or decimal.
     * 
     * This utility function is used to convert an integer to its string representation,
     * either in fixed-width binary format or standard decimal format, depending on the 
     * specified mode.
     * 
     * @param value The unsigned integer value to be formatted.
     * @param binary If true, returns the binary representation of the value.
     *               If false, returns the decimal representation.
     * @param width The number of lower bits to include in the binary representation.
     *              Ignored if binary is false.
     * 
     * @return A std::string containing the formatted representation of the value.
     */
    std::string formatValue(unsigned int value, bool binary = false, std::size_t width = 8) const {
        if (binary) {
            return std::bitset<64>(value).to_string().substr(64 - width);  // Works for wide counters too
        } else {
            return std::to_string(value);
        }
    }

    /**
     * @brief Prints the contents of the Bimodal Predictor table.
     *
     * Displays the index, counter value (in binary or decimal), 
     * predicted outcome ("Taken" or "Not Taken"), and prediction bit.
     * Output format is controlled by the PRINT_FORMAT_BINARY flag.
     */

     void printBimodal(bool binary = config::PRINT_FORMAT_BINARY) const {
        // Define column widths
        constexpr std::size_t indexColBase     = 8;
        constexpr std::size_t counterColBinary = config::BM_CTR_WIDTH + 8;
        constexpr std::size_t counterColDec    = 12;
        constexpr std::size_t predColBinary    = 12;
        constexpr std::size_t predColDec       = 12;
    
        std::size_t indexColWidth   = indexColBase;
        std::size_t counterColWidth = binary ? counterColBinary : counterColDec;
        std::size_t predColWidth    = binary ? predColBinary    : predColDec;
    
        std::size_t totalWidth = indexColWidth + counterColWidth + predColWidth;
    
        // Print header
        std::cout << std::left
                  << std::setw(indexColWidth)   << "Index"
                  << std::setw(counterColWidth) << "Counter"
                  << std::setw(predColWidth)    << "Prediction"
                  << "\n";
        std::cout << std::string(totalWidth, '-') << "\n";
    
        // Print each entry
        for (unsigned int i = 0; i < tableSize; ++i) {
            unsigned int counter = bimodalTable[i];
            std::string counterStr = formatValue(counter, binary, config::BM_CTR_WIDTH);
            bool predBit = counter >= (1 << (counterBits - 1));
            std::string predictionStr = binary ? std::to_string(predBit) : (predBit ? "Taken" : "Not Taken");
    
            std::cout << std::left
                      << std::setw(indexColWidth)   << formatValue(i, binary, config::BM_CTR_WIDTH)
                      << std::setw(counterColWidth) << counterStr
                      << std::setw(predColWidth)    << ("(" + predictionStr + ")")
                      << "\n";
        }
    
        std::cout << std::string(totalWidth, '=') << "\n";
    }
};