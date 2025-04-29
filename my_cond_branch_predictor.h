#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include <stdlib.h>

struct history {
    PatternHistoryRegister PHR;
    uint64_t branchCount;

    // Constructor
    history()
        : PHR(config::PHR_WIDTH),  // Initialize PHR with specified width
        branchCount(0)            // Default to 0 
    {}
};

// Strictly for debug purpose
// Create and open the trace file (append mode)
/**
 * @brief Logs branch prediction trace to file.
 * 
 * @param PC         Branch instruction address (hex)
 * @param target     Next PC / target address (hex)
 * @param predDir    Predicted direction (bool: 0/1)
 * @param resolveDir Actual outcome (bool: 0/1)
 */
 void printTrace(std::ofstream& trace_file, uint64_t PC, uint64_t target, bool predDir, bool resolveDir) {
    if (trace_file.is_open()) {
        trace_file << "0x" << std::hex << PC
                   << ",0x" << target
                   << "," << std::dec << static_cast<int>(predDir)
                   << "," << static_cast<int>(resolveDir)
                   << "\n";
    }
}

class SampleCondPredictor
{       
    public:
        BimodalPredictor T0;
        PatternHistoryTable* T[config::CACHE_N_TABLE_CNT];

        unsigned int index[config::CACHE_N_TABLE_CNT];

        int8_t useAltOnNa[config::ALT_PRED_CONF_T_SIZE];
        history active_hist;
        std::unordered_map<uint64_t/*key*/, history/*val*/> pred_time_histories;

        // Debug trace file
        std::ofstream trace_file;
        
    public:

        SampleCondPredictor()
        : T0(config::BM_INDEX_WIDTH, config::BM_CTR_WIDTH)
        {
            // Instantiate the Pattern History tables
            for (int i = 0; i < config::CACHE_N_TABLE_CNT; i++) {
                T[i] = new PatternHistoryTable(config::CACHE_N_SET, config::CACHE_N_ASSOC);
            }

            // Initialize Alternate Predictor Confidence Table
            for (int i = 0; i < config::ALT_PRED_CONF_T_SIZE; i++){
                useAltOnNa[i] = 0;
            }

            // Generate branch trace log
            if (config::GEN_INSTR_TRACE) {
                // Open trace_debug.txt separately for trace logging
                trace_file.open("trace_debug.txt", std::ios::app);
            }
        }

        void setup()
        {
            printConfig();
            printBudget();
        }

        void terminate()
        {
        }
        
        // sample function to get unique instruction id
        uint64_t get_unique_inst_id(uint64_t seq_no, uint8_t piece) const
        {
            assert(piece < 16);
            return (seq_no << 4) | (piece & 0x000F);
        }
        
        bool predict (uint64_t seq_no, uint8_t piece, uint64_t PC)
        {   
            active_hist.branchCount++;
            pred_time_histories.emplace(get_unique_inst_id(seq_no, piece), active_hist);

            bool prediction;
            uint32_t predictionCtr;

            for (int i = 0; i < config::CACHE_N_TABLE_CNT; i++) {
                index[i] = static_cast<unsigned int>(active_hist.PHR.foldHistory(config::TAGE_HISTORY_LENGTHS[i], config::CACHE_N_INDEX_WIDTH).to_ulong());
            }
     
            unsigned int tag = active_hist.PHR.generateTag(PC);
                
            std::bitset<config::TOTAL_TABLES> results;
            results[0] = T0.predict(PC); // Taken/Not Taken
            for (int i = 0; i < config::CACHE_N_TABLE_CNT; i++) {
                results[i+1] = T[i]->accessCache(index[i], tag); // Hit/Miss
            }
            
            // Priority check: highest index with result 1
            // Step 1: Find selectedT (highest hit)
            uint32_t selectedT = 0;
            uint32_t selectedAlt = 0;

            for (int i = config::CACHE_N_TABLE_CNT; i >= 1; --i) {
                if (results[i]) {
                    selectedT = i;
                    break;
                }
            }

             // Step 2: Find selectedAlt (next-highest hit below selectedT among the PHTs)
             for (int i = selectedT - 1; i >= 1; --i) {
                if (results[i]) {
                    selectedAlt = i;
                    break;
                }
            }

            uint8_t altPredCtr;
            bool altPred;

            // Alternate prediction selection
            if (selectedAlt == 0) {
                altPred = results[0];
                altPredCtr = T0.getCtr(PC);
            } else if (selectedAlt < config::CACHE_N_TABLE_CNT) {
                altPredCtr = T[selectedAlt - 1]->accessCtrOnHit(index[selectedAlt - 1], tag);
                altPred = (altPredCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
            }
            else {
                std::cerr << "Invalid alternate predictor selected: T" << selectedAlt << std::endl;
                assert(false);  // Optional: terminate on unexpected case
            }

            // Main prediction selection
            if (selectedT == 0) {
                prediction = results[0];
            } else if (selectedT <= config::CACHE_N_TABLE_CNT) {
                predictionCtr = T[selectedT - 1]->accessCtrOnHit(index[selectedT - 1], tag);
                prediction = (predictionCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
            }
            else {
                std::cerr << "Invalid predictor selected: T" << selectedT << std::endl;
                assert(false);  // Optional: terminate on unexpected case
            }
            
            // Select between main and alternate prediction based on confidence levels.
            // If the alternate prediction is confident and the current prediction is weak,
            // prefer the alternate prediction.
            if (selectedT > 0) {
                bool altPredConf = isStrong(altPredCtr, config::CACHE_N_CTR_WIDTH);
                uint8_t altIndex = getUseAltIndex(selectedT, altPredConf);
                bool hUseAltOnNa = (useAltOnNa[altIndex] >= (1 << (config::ALT_PRED_CONF_T_WIDTH - 1)));

                if (hUseAltOnNa && !isStrong(predictionCtr, config::CACHE_N_CTR_WIDTH)) {
                    prediction = altPred;
                }
            }

            //if(PC == 0x449cd4){
            //    DEBUG_PRINT(
            //        "PC=0x%08x, Hash=%lu, Pred=%d, Index1=%d, Index2=%d, Index3=%d, Tag=%u, SelectedT=%d\n",
            //        PC,
            //        get_unique_inst_id(seq_no, piece),
            //        prediction,
            //        index1,
            //        index2,
            //        index3,
            //        tag,
            //        selectedT
            //    );
            //        T1.printCacheSet(index1);
            //        T2.printCacheSet(index2);
            //        T3.printCacheSet(index3);
            //}

            return prediction;
        }

        void history_update (uint64_t seq_no, uint8_t piece, uint64_t PC, bool resolveDir, uint64_t nextPC)
        {
            active_hist.PHR.updatePHR(PC, nextPC, resolveDir);
        }

        void update (uint64_t seq_no, uint8_t piece, uint64_t PC, bool resolveDir, bool predDir, int64_t target)
        {
            // Update Policy
            const auto pred_hist_key = get_unique_inst_id(seq_no, piece);
            const auto& hist_to_use = pred_time_histories.at(pred_hist_key);
            
            if (resolveDir != predDir) {
                active_hist = hist_to_use;
                active_hist.PHR.updatePHR(PC, target, resolveDir);
            }

            for (int i = 0; i < config::CACHE_N_TABLE_CNT; i++) {
                index[i] = static_cast<unsigned int>(hist_to_use.PHR.foldHistory(config::TAGE_HISTORY_LENGTHS[i], config::CACHE_N_INDEX_WIDTH).to_ulong());
            }
           
            unsigned int tag = hist_to_use.PHR.generateTag(PC);

            std::bitset<config::TOTAL_TABLES> results;
            results[0] = T0.predict(PC); // Taken/Not Taken
            for (int i = 0; i < config::CACHE_N_TABLE_CNT; i++) {
                results[i+1] = T[i]->accessCache(index[i], tag); // Hit/Miss
            }
            
            // Priority check: highest index with result 1
            // Step 1: Find selectedT (highest hit)
            uint32_t selectedT = 0;
            uint32_t selectedAlt = 0;

            for (int i = config::CACHE_N_TABLE_CNT; i >= 1; --i) {
                if (results[i]) {
                    selectedT = i;
                    break;
                }
            }
        
            // Step 2: Find selectedAlt (next-highest hit below selectedT among the PHTs)
            for (int i = selectedT - 1; i >= 1; --i) {
                if (results[i]) {
                    selectedAlt = i;
                    break;
                }
            }

            uint8_t altPredCtr;
            bool altPred;

            // Alternate prediction selection
            if (selectedAlt == 0) {
                altPred = results[0];
                altPredCtr = T0.getCtr(PC);
            } else if (selectedAlt < config::CACHE_N_TABLE_CNT) {
                altPredCtr = T[selectedAlt - 1]->accessCtrOnHit(index[selectedAlt - 1], tag);
                altPred = (altPredCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
            }
            else {
                std::cerr << "Invalid alternate predictor selected: T" << selectedAlt << std::endl;
                assert(false);  // Optional: terminate on unexpected case
            }
            
            // Main prediction selection
            if (selectedT == 0) {
                T0.update(PC, resolveDir);
            } else if (selectedT <= config::CACHE_N_TABLE_CNT) {
                T[selectedT - 1]->updateCtr(index[selectedT - 1], tag, resolveDir);
                T[selectedT - 1]->updateUCtr(index[selectedT - 1], tag, predDir, altPred, resolveDir);
                if (resolveDir == predDir) {
                    T[selectedT - 1]->updateCacheLinesLRU(index[selectedT - 1], tag);
                }
            }
            else {
                std::cerr << "Invalid predictor selected: T" << selectedT << std::endl;
                assert(false);  // Optional: terminate on unexpected case
            }

            if (resolveDir != predDir) {
                tryAllocate(selectedT, tag, predDir, resolveDir, T, index);
            }

            // Reset counter every n-branch lookup
            for (int i = 0; i < config::CACHE_N_TABLE_CNT; i++) {
                T[i]->resetUCtr(hist_to_use.branchCount);
            }

            int branchCount = hist_to_use.branchCount;
            pred_time_histories.erase(pred_hist_key);
            if (config::GEN_INSTR_TRACE){
                printTrace(trace_file, PC, target, predDir, resolveDir);
            }
            
            // Update alternate prediction confidence table if alternate prediction differs
            // from the prediction used (i.e., disagreement between altPred and predDir)
            // This allows the predictor to adaptively choose between the main and alternate
            // predictions in the future
            uint8_t altIndex = 0;
            if (selectedT > 0) {
                bool altPredConf = isStrong(altPredCtr, config::CACHE_N_CTR_WIDTH);
                altIndex = getUseAltIndex(selectedT, altPredConf);
                if (predDir != altPred) {
                    ctrUpdate(useAltOnNa[altIndex], (altPred == resolveDir), config::ALT_PRED_CONF_T_WIDTH);
                }
            }
            if (config::DEBUG) {
                if (PC == 0x449cd4 && resolveDir != predDir & 0) {
                    dPrint(
                        "PC=0x%08x, Hash=%lu, Pred=%d, Actual=%d, Tag=%u, SelectedT=%d, SelectedAlt=%d, AltPred=%d, Target=%ld, Count=%ld, AltPredTable=%d\n",
                        PC,
                        get_unique_inst_id(seq_no, piece),
                        predDir,
                        resolveDir,
                        tag,
                        selectedT,
                        selectedAlt,
                        altPred,
                        target,
                        branchCount,
                        static_cast<int>(useAltOnNa[altIndex])
                    );
                    dPrint("T[0] Bimodal = %d\n", T0.getCtr(PC));
                    for (int i = 0; i < config::CACHE_N_TABLE_CNT; i++) {
                        dPrint("T[%d] Set Index = %d\n", i, index[i]);
                        T[i]->printCacheSet(index[i]);
                    }
                    for (int i = 0; i < config::ALT_PRED_CONF_T_SIZE; i++) {
                        dPrint("USE_ALT_NA = %d\n", useAltOnNa[i]);
                    }
                }        
            }
        }
};
// =================
// Predictor End
// =================

#endif
static SampleCondPredictor cond_predictor_impl;