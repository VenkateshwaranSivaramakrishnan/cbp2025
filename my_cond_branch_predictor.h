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
        PatternHistoryTable T1;
        PatternHistoryTable T2;
        PatternHistoryTable T3;

        int8_t useAltOnNa[config::ALT_PRED_CONF_T_SIZE];
        history active_hist;
        std::unordered_map<uint64_t/*key*/, history/*val*/> pred_time_histories;

        // Debug trace file
        std::ofstream trace_file;
        
    public:

        SampleCondPredictor()
        : T0(config::BM_INDEX_WIDTH, config::BM_CTR_WIDTH),
        T1(config::CACHE_N_SET, config::CACHE_N_ASSOC),
        T2(config::CACHE_N_SET, config::CACHE_N_ASSOC),
        T3(config::CACHE_N_SET, config::CACHE_N_ASSOC)
        {
            if (config::GEN_INSTR_TRACE) {
                trace_file.open("trace_debug.txt", std::ios::app);
            }
            for (int i = 0; i < config::ALT_PRED_CONF_T_SIZE; i++){
                useAltOnNa[i] = 0;
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

            unsigned int index1 = static_cast<unsigned int>(active_hist.PHR.foldPHR_1(PC).to_ulong());
            unsigned int index2 = static_cast<unsigned int>(active_hist.PHR.foldPHR_2(PC).to_ulong());
            unsigned int index3 = static_cast<unsigned int>(active_hist.PHR.foldPHR_3(PC).to_ulong());
                
            unsigned int tag = active_hist.PHR.generateTag(PC);
                
            std::bitset<4> results;
            results[0] = T0.predict(PC); // Taken/Not Taken
            results[1] = T1.accessCache(index1, tag); // Hit/Miss
            results[2] = T2.accessCache(index2, tag); // Hit/Miss
            results[3] = T3.accessCache(index3, tag); // Hit/Miss
            
            // Priority check: highest index with result 1
            // Step 1: Find selectedT (highest hit)
            uint32_t selectedT = 0;
            uint32_t selectedAlt = 0;

            for (int i = 3; i >= 1; --i) {
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

            switch (selectedAlt) {
                case 0:
                    altPredCtr = results[0];
                    altPred =  (altPredCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
                    break;
                case 1:
                    altPredCtr = T1.accessCtrOnHit(index1, tag);
                    altPred =  (altPredCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
                    break;
                case 2:
                    altPredCtr = T2.accessCtrOnHit(index2, tag);
                    altPred =  (altPredCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
                    break;
                default:
                    std::cerr << "Invalid alternate predictor selected: T" << selectedAlt << std::endl;
                    assert(false);  // Optional: terminate on unexpected case
                    break;
            }
          

            switch (selectedT) {
                case 0:
                    prediction = results[0];
                    break;
                case 1:
                    predictionCtr = T1.accessCtrOnHit(index1, tag);
                    prediction =  (predictionCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
                    break;
                case 2:
                    predictionCtr = T2.accessCtrOnHit(index2, tag);
                    prediction =  (predictionCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
                    break;
                case 3:
                    predictionCtr = T3.accessCtrOnHit(index3, tag);
                    prediction =  (predictionCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
                    break;
                default:
                    std::cerr << "Invalid predictor selected: T" << selectedT << std::endl;
                    assert(false);  // Optional: terminate on unexpected case
                    break;
            }
            
            // Select between main and alternate prediction based on confidence levels.
            // If the alternate prediction is confident and the current prediction is weak,
            // prefer the alternate prediction.
            if (selectedT > 0) {
                bool altPredConf = isStrong(altPredCtr, config::CACHE_N_CTR_WIDTH);
                uint8_t altIndex = getUseAltIndex(selectedT, altPredConf);
                bool hUseAltOnNa = (useAltOnNa[altIndex]);

                if (hUseAltOnNa && !isStrong(predictionCtr, config::CACHE_N_CTR_WIDTH)) {
                    prediction = altPred;
                }
            }

            //if(PC == 0x449cd4){
            //    std::cout << "PC=0x" << std::hex << std::setw(8) << PC
            //        << ", Hash="<< get_unique_inst_id(seq_no, piece)
            //        << std::dec << ", Pred=" << prediction
            //        << ", Index1=" << index1
            //        << ", Index2=" << index2
            //        << ", Index3=" << index3 
            //        << ", Tag=" << tag
            //        << ", SelectedT=" << selectedT
            //        << std::endl;
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

            unsigned int index1 = static_cast<unsigned int>(hist_to_use.PHR.foldPHR_1(PC).to_ulong());
            unsigned int index2 = static_cast<unsigned int>(hist_to_use.PHR.foldPHR_2(PC).to_ulong());
            unsigned int index3 = static_cast<unsigned int>(hist_to_use.PHR.foldPHR_3(PC).to_ulong());
                
            unsigned int tag = hist_to_use.PHR.generateTag(PC);

            std::bitset<4> results;
            results[0] = T0.predict(PC); // Taken/Not Taken
            results[1] = T1.accessCache(index1, tag); // Hit/Miss
            results[2] = T2.accessCache(index2, tag); // Hit/Miss
            results[3] = T3.accessCache(index3, tag); // Hit/Miss
            
            // Priority check: highest index with result 1
            // Step 1: Find selectedT (highest hit)
            uint32_t selectedT = 0;
            uint32_t selectedAlt = 0;

            for (int i = 3; i >= 1; --i) {
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

            switch (selectedAlt) {
                case 0:
                    altPredCtr = results[0];
                    altPred =  (altPredCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
                    break;
                case 1:
                    altPredCtr = T1.accessCtrOnHit(index1, tag);
                    altPred =  (altPredCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
                    break;
                case 2:
                    altPredCtr = T2.accessCtrOnHit(index2, tag);
                    altPred =  (altPredCtr >= (1 << (config::CACHE_N_CTR_WIDTH - 1)));
                    break;
                default:
                    std::cerr << "Invalid alternate predictor selected: T" << selectedAlt << std::endl;
                    assert(false);  // Optional: terminate on unexpected case
                    break;
            }
            
            switch (selectedT) {
                case 0:
                    T0.update(PC, resolveDir);
                    break;
                case 1:
                    T1.updateCtr(index1, tag, resolveDir);
                    T1.updateUCtr(index1, tag, predDir, altPred, resolveDir);
                    if (resolveDir == predDir) {
                        T1.updateCacheLinesLRU(index1, tag);
                    }
                    break;
                case 2:
                    T2.updateCtr(index2, tag, resolveDir);
                    T2.updateUCtr(index2, tag, predDir, altPred, resolveDir);
                    if (resolveDir == predDir) {
                        T2.updateCacheLinesLRU(index2, tag);
                    }
                    break;
                case 3:
                    T3.updateCtr(index3, tag, resolveDir);
                    T3.updateUCtr(index3, tag, predDir, altPred, resolveDir);
                    if (resolveDir == predDir) {
                        T3.updateCacheLinesLRU(index3, tag);
                    }
                    break;
                default:
                    std::cerr << "Invalid predictor selected: T" << selectedT << std::endl;
                    assert(false);  // Optional: terminate on unexpected case
                    break;
            }

            PatternHistoryTable* T[] = { &T1, &T2, &T3 };
            uint32_t index[] = { index1, index2, index3 };
            if (resolveDir != predDir) {
                tryAllocate(selectedT, tag, predDir, resolveDir, T, index);
            }

            // Reset counter every n-branch lookup
            T1.resetUCtr(hist_to_use.branchCount);
            T2.resetUCtr(hist_to_use.branchCount);
            T3.resetUCtr(hist_to_use.branchCount);

            pred_time_histories.erase(pred_hist_key);
            if (config::GEN_INSTR_TRACE){
                printTrace(trace_file, PC, target, predDir, resolveDir);
            }
            
            // Update alternate prediction confidence table if alternate prediction differs
            // from the prediction used (i.e., disagreement between altPred and predDir)
            // This allows the predictor to adaptively choose between the main and alternate
            // predictions in the future
            if (selectedT > 0) {
                bool altPredConf = isStrong(altPredCtr, config::CACHE_N_CTR_WIDTH);
                uint8_t altIndex = getUseAltIndex(selectedT, altPredConf);

                if (predDir != altPred) {
                    ctrUpdate(useAltOnNa[altIndex], (altPred == resolveDir), config::ALT_PRED_CONF_T_WIDTH);
                }
            }

            //if(PC == 0x449cd4 && resolveDir != predDir){
            //    std::cout << "PC=0x" << std::hex << std::setw(8) << PC
            //        << ", Hash=" << get_unique_inst_id(seq_no, piece)
            //        << std::dec << ", Pred=" << predDir
            //        << ", Actual=" << resolveDir
            //        << ", Index1=" << index1
            //        << ", Index2=" << index2
            //        << ", Index3=" << index3 
            //        << ", Tag=" << tag
            //        << ", SelectedT=" << selectedT
            //        << ", SelectedAlt=" << selectedAlt
            //        << ", AltPred=" << altPred
            //        << std::endl;
            //        T1.printCacheSet(index1);
            //        T2.printCacheSet(index2);
            //        T3.printCacheSet(index3);
            //}

        }

};
// =================
// Predictor End
// =================

#endif
static SampleCondPredictor cond_predictor_impl;