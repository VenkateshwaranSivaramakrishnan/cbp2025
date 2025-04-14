/**
 * @class PatternHistoryTable
 * @brief Extension of SetAssociativeCache to support update functionality for branch prediction.
 *
 * This class simulates a Pattern History Table (PHT) by leveraging a set-associative cache structure.
 * Each cache line stores a saturating counter representing the confidence of taken/not-taken outcomes.
 * The `update()` method performs behavior similar to a bimodal predictor:
 * - If a cache hit occurs, the associated counter is updated.
 * - If there is no matching tag (miss), the update is ignored.
 */
class PatternHistoryTable : public SetAssociativeCache {
public:
    /**
     * @brief Constructs a PatternHistoryTable using base SetAssociativeCache constructor.
     * 
     * @param numSets Number of sets in the PHT (cache).
     * @param associativity Number of cache lines per set.
     */
    PatternHistoryTable(unsigned int numSets, unsigned int associativity)
        : SetAssociativeCache(numSets, associativity) {}
    
    /**
     * @brief Updates the counter associated with a given PC and branch outcome.
     *
     * This function simulates branch predictor training by locating the cache line
     * corresponding to the PC-derived tag and updating its saturating counter:
     * - Increments the counter if the branch was taken, up to the maximum.
     * - Decrements the counter if the branch was not taken, down to 0.
     *
     * @param index The cache set index derived from PC or hash.
     * @param tag The tag derived from the PC or folded history.
     * @param actualOutcome The actual branch outcome (true = taken, false = not taken).
     */
    void updateCtr(unsigned int index, unsigned int tag, bool actualOutcome) {
        unsigned int maxVal = (1 << config::CACHE_N_CTR_WIDTH) - 1;

        for (std::size_t i = 0; i < sets[index].lines.size(); ++i) {
            if (sets[index].lines[i].valid && sets[index].lines[i].tag == tag) {
                if (actualOutcome) {
                    if (sets[index].lines[i].ctr < maxVal) {
                        sets[index].lines[i].ctr++;  // Strengthen taken
                    }
                } else {
                    if (sets[index].lines[i].ctr > 0) {
                        sets[index].lines[i].ctr--;  // Strengthen not-taken
                    }
                }
                return;  // Only update first matching line
            }
        }

        // Handle case where tag not found (miss)
        std::cerr << "PatternHistoryTable::update(): Cache miss for tag " << tag << " at index " << index << "\n";
        assert(false);
    }

    /**
     * @brief Updates the usefulness counter (u) for a cache line based on prediction correctness.
     *
     * This function is used to train the usefulness predictor (TAGE-predictor).
     * It updates the `u` counter **only if** the main prediction differs from the alternate predictor:
     *
     * - If the main prediction was correct (`prediction == branchOutcome`), increment the `u` counter.
     * - If the main prediction was wrong, decrement the `u` counter.
     * @param index The cache set index derived from PC or hash.
     * @param tag The tag derived from the PC or folded history.
     * @param prediction The actual prediction made by the selected predictor (true = taken, false = not taken).
     * @param altPred The prediction made by the alternate (backup) predictor.
     * @param branchOutcome The actual outcome of the branch (true = taken, false = not taken).
     */
    void updateUCtr(unsigned int index, unsigned int tag, bool prediction, bool altPred, bool branchOutcome) {
        if (prediction == altPred) return;  // No disagreement
    
        unsigned int maxU = (1 << config::CACHE_N_U_WIDTH) - 1;
    
        for (std::size_t i = 0; i < sets[index].lines.size(); ++i) {
            if (sets[index].lines[i].valid && sets[index].lines[i].tag == tag) {
                bool predictionCorrect = (prediction == branchOutcome);
    
                if (predictionCorrect && sets[index].lines[i].u < maxU) {
                    sets[index].lines[i].u++;
                } else if (!predictionCorrect && sets[index].lines[i].u > 0) {
                    sets[index].lines[i].u--;
                }
    
                return;
            }
        }
    
        std::cerr << "PatternHistoryTable::updateUCtr(): Cache miss for tag " << tag << " at index " << index << "\n";
        assert(false);
    }
    
    /**
     * @brief Resets the usefulness (u) counters periodically based on the branch access count.
     *
     * This function is called periodically during branch prediction simulation.
     * When the total `branchCount` is a multiple of `config::CACHE_N_U_RST_CNT`,
     * it halves all usefulness counters (u >>= 1) across the entire cache.
     *
     * @param branchCount The current total number of branch lookups or updates performed.
     */
    void resetUCtr() {
        if (branchCount % config::CACHE_N_U_RST_CNT != 0) {
            return;  // Not yet time to reset
        }

        for (std::size_t setIdx = 0; setIdx < sets.size(); ++setIdx) {
            for (std::size_t lineIdx = 0; lineIdx < sets[setIdx].lines.size(); ++lineIdx) {
                    if (sets[setIdx].lines[lineIdx].valid) {
                        sets[setIdx].lines[lineIdx].u >>= 1;  // Halve usefulness counter
                    }
            }
        }
    }
};