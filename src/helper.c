/**
 * @brief Attempts to allocate an entry in a higher-indexed component if the provider is not T3.
 *
 * This logic checks components T[selectedT+1] to T3 for available lines (u == 0).
 * If multiple components are eligible, one is selected using **weighted probability**:
 *     - T1 (index 0) → weight 4
 *     - T2 (index 1) → weight 2
 *     - T3 (index 2) → weight 1
 * If no eligible line is found, the u counters in each of those components are decremented.
 * On allocation, the counter is initialized to weakly correct and usefulness (u) to 0.
 *
 * @param selectedT The current provider component (0 to 3)
 * @param tag The tag to allocate
 * @param prediction The prediction result at the time of allocation
 * @param T[] Array of PatternHistoryTable components (T0 to T3)
 * @param index Array of indices (one per table): index[0] = index1, index[1] = index2, index[2] = index3
 */
void tryAllocate(int selectedT, unsigned int tag, bool prediction, bool resolveDir,
                  PatternHistoryTable* T[], uint32_t index[]) {

    if (selectedT >= config::CACHE_N_TABLE_CNT) return;  // Nothing to allocate beyond T3
//   std::vector<int> candidates;
    for (int i = selectedT; i < config::CACHE_N_TABLE_CNT; ++i) {
        auto& lines = T[i]->getSet(index[i]).lines;
        for (std::size_t j = 0; j < lines.size(); ++j) {
            if (!lines[j].valid) {  // If an invalid cache line is found, replace it
                lines[j].valid = true;
                lines[j].tag = tag;
                lines[j].ctr = (1 << (config::CACHE_N_CTR_WIDTH - 1)) + (resolveDir ? 0 : -1); // Set counter to weakly taken/not-taken based on resolveDir
                lines[j].u = 0;
                T[i]->updateCacheLinesLRU(index[i], tag);
                return;
            }
        }
        // If all lines are valid, enforce LRU replacement policy (implies all N-ways are occupied)
        //bool evictFound = false;
        //uint32_t evictWay = 0;
        for (int j = 0; j < lines.size(); j++) {
            //if (lines[j].u == 0) {
            //    if (!evictFound){
            //        evictFound = true;
            //        evictWay = j;
            //    }
            //    else {
            //        if (lines[j].lru > lines[evictWay].lru){
            //            evictWay = j;
            //        }
            //    }
            //}
        //}
            if (lines[j].valid && lines[j].u == 0 && (lines[j].lru == config::CACHE_N_LRU_MAX)) {
                lines[j].valid = true;
                lines[j].tag = tag;
                lines[j].ctr = (1 << (config::CACHE_N_CTR_WIDTH - 1)) + (resolveDir ? 0 : -1);  // Set counter to weakly taken/not-taken based on resolveDir
                lines[j].u = 0;
                T[i]->updateCacheLinesLRU(index[i], tag);
                return;
            }
        }
        //if (evictFound) {
        //    lines[evictWay].valid = true;
        //    lines[evictWay].tag = tag;
        //    lines[evictWay].ctr = (1 << (config::CACHE_N_CTR_WIDTH - 1)) + (resolveDir ? 0 : -1);  // Set counter to weakly taken/not-taken based on resolveDir
        //    lines[evictWay].u = 0;
        //    T[i]->updateCacheLinesLRU(index[i], tag);
        //    return;
        //}
    }

    // Step 4: No eligible line → decay u in T[selectedT+1] to T3
    for (int i = selectedT; i < 3; ++i) {
        auto& lines = T[i]->getSet(index[i]).lines;
        for (std::size_t j = 0; j < lines.size(); ++j) {
            if (lines[j].valid && lines[j].u > 0) {
                lines[j].u--;   
            }
        }
    }

//    // Step 1: Identify eligible components above the provider (T[selectedT] to T[2])
//    for (int i = selectedT; i < 3; ++i) {
//        auto& lines = T[i]->getSet(index[i]).lines;
//        for (std::size_t j = 0; j < lines.size(); ++j) {
//            if (lines[j].u == 0) {
//                candidates.push_back(i);  // i maps to T[i]
//                break;  // Only need to know that at least one u==0 exists
//            }
//        }
//    }
//
//    // Step 2: If candidates exist, apply weighted random selection
//    if (!candidates.empty()) {
//        int chosen = candidates.front();  // Default to first
//
//        if (candidates.size() > 1) {
//            // Assign weights based on component index: lower index = higher weight
//            std::vector<int> weights;
//            for (int idx : candidates) {
//                if (idx == 0) weights.push_back(4);  // T1
//                else if (idx == 1) weights.push_back(2);  // T2
//                else if (idx == 2) weights.push_back(1);  // T3
//            }
//
//            // Build cumulative distribution
//            std::vector<int> cumulative;
//            int total = 0;
//            for (int w : weights) {
//                total += w;
//                cumulative.push_back(total);
//            }
//
//            // Generate random number in [0, total-1]
//            std::random_device rd;
//            std::mt19937 gen(rd());
//            std::uniform_int_distribution<> dist(0, total - 1);
//            int r = dist(gen);
//
//            // Select component based on weighted interval
//            for (std::size_t i = 0; i < cumulative.size(); ++i) {
//                if (r < cumulative[i]) {
//                    chosen = candidates[i];
//                    break;
//                }
//            }
//        }
//
//        // Step 3: Allocate in chosen component, find first line with u == 0
//        auto& lines = T[chosen]->getSet(index[chosen]).lines;
//        for (std::size_t j = 0; j < lines.size(); ++j) {
//            if (lines[j].u == 0) {
//                lines[j].valid = true;
//                lines[j].tag = tag;
//                lines[j].ctr = (1 << (config::CACHE_N_CTR_WIDTH - 1));  // Weakly correct
//                lines[j].u = 0;
//                return;
//            }
//        }
//    }
//    else {
//        // Step 4: No eligible line → decay u in T[selectedT+1] to T3
//        for (int i = selectedT; i < 3; ++i) {
//            auto& lines = T[i]->getSet(index[i]).lines;
//            for (std::size_t j = 0; j < lines.size(); ++j) {
//                if (lines[j].valid && lines[j].u > 0) {
//                    lines[j].u--;   
//                }
//            }
//        }
//    }
}

/**
 * @brief Computes the index into the `use_alt_on_na` table.
 *
 * This index determines which counter to access based on the TAGE hit bank
 * and whether the alternate prediction is considered confident. This logic
 * allows for bank-aware and confidence-aware adaptation in using alternate predictions.
 *
 * @param hitBank The index of the bank that provided the longest matching prediction (1-based).
 * @param altConf Boolean flag indicating whether the alternate prediction is confident.
 * @return The computed index into the `use_alt_on_na` table.
 */
inline int getUseAltIndex(int hitBank, bool altConf) {
    return (((hitBank - 1) / 1) << 1) + (altConf ? 1 : 0);
}

/**
 * @brief Determines whether a saturating counter is in a strong state.
 *
 * A counter is considered "strong" if it is not in the weak mid-range zone,
 * i.e., not equal to the center or just below the center of its representable range.
 *
 * @param ctr The current value of the counter (unsigned).
 * @param nbits The width of the counter in bits (e.g., 2 for a 2-bit counter).
 * @return True if the counter is in a strong state, false if weak.
 */
inline bool isStrong(uint8_t ctr, int nbits) {
    int mid = 1 << (nbits - 1);  // midpoint
    return (ctr != mid) && (ctr != mid - 1);  // weak if near center
}

/**
 * @brief Performs a saturating update on an unsigned n-bit counter.
 *
 * If the branch outcome was taken, the counter is incremented (up to max value).
 * If not taken, it is decremented (down to zero). The counter is guaranteed to stay
 * within the range [0, 2^nbits - 1].
 *
 * @param ctr Reference to the counter variable to be updated.
 * @param taken The actual outcome of the branch (true if taken, false otherwise).
 * @param nbits The number of bits used to represent the counter.
 */
void ctrUpdate(int8_t& ctr, bool taken, int nbits) {
    unsigned int maxU = (1 << nbits) - 1;
    if(taken) {
        if(ctr < maxU) ctr++;
    }
    else{
        if(ctr > 0) ctr--;
    }
}