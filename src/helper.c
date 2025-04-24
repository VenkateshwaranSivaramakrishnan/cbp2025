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
void tryAllocate(int selectedT, unsigned int tag, bool prediction,
                  PatternHistoryTable* T[], uint32_t index[]) {
    if (selectedT >= 3) return;  // Nothing to allocate beyond T3
 
//   std::vector<int> candidates;

    for (int i = selectedT; i < 3; ++i) {
        auto& lines = T[i]->getSet(index[i]).lines;
        for (std::size_t j = 0; j < lines.size(); ++j) {
            if (!lines[j].valid) {  // If an invalid cache line is found, replace it
                lines[j].valid = true;
                lines[j].tag = tag;
                lines[j].ctr = (1 << (config::CACHE_N_CTR_WIDTH - 1));  // Weakly correct
                lines[j].u = 0;
                T[i]->updateCacheLinesLRU(index[i], tag);
                return;
            }
        }
        // If all lines are valid, enforce LRU replacement policy (implies all N-ways are occupied)
        bool evictFound = false;
        uint32_t evictWay = 0;
        for (int j = 0; j < lines.size(); j++) {
            if (lines[j].u == 0) {
                if (!evictFound){
                    evictFound = true;
                    evictWay = j;
                }
                else {
                    if (lines[j].lru > lines[evictWay].lru){
                        evictWay = j;
                    }
                }
            }
        }
            //if (lines[j].valid && lines[j].u == 0 && (lines[j].lru == config::CACHE_N_LRU_MAX)) {
            //    lines[j].valid = true;
            //    lines[j].tag = tag;
            //    lines[j].ctr = (1 << (config::CACHE_N_CTR_WIDTH - 1));  // Weakly correct
            //    lines[j].u = 0;
            //    T[i]->updateCacheLinesLRU(index[i], tag);
            //    return;
            //}
        if (evictFound) {
            lines[evictWay].valid = true;
            lines[evictWay].tag = tag;
            lines[evictWay].ctr = (1 << (config::CACHE_N_CTR_WIDTH - 1));  // Weakly correct
            lines[evictWay].u = 0;
            T[i]->updateCacheLinesLRU(index[i], tag);
            return;
        }
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