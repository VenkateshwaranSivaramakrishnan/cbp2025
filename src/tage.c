#include "config.hpp"

int main(){

    int32_t PC = 0xA5B3; // TODO
    int32_t target = 0xA; // TODO

    bool branchOutcome = false; // TODO: Replace
    
    PatternHistoryRegister PHR(config::PHR_WIDTH);
    BimodalPredictor T0(config::BM_INDEX_WIDTH, config::BM_CTR_WIDTH);
    PatternHistoryTable T1(config::CACHE_N_SET, config::CACHE_N_ASSOC);
    PatternHistoryTable T2(config::CACHE_N_SET, config::CACHE_N_ASSOC);
    PatternHistoryTable T3(config::CACHE_N_SET, config::CACHE_N_ASSOC);

    unsigned int index1 = static_cast<unsigned int>(PHR.foldPHR_1(PC).to_ulong());
    unsigned int index2 = static_cast<unsigned int>(PHR.foldPHR_2(PC).to_ulong());
    unsigned int index3 = static_cast<unsigned int>(PHR.foldPHR_3(PC).to_ulong());

    unsigned int tag = static_cast<unsigned int>(PHR.generateTag().to_ulong());
    
    std::bitset<4> results;
    results[0] = T0.predict(PC); // Taken/Not Taken
    results[1] = T1.accessCache(index1, tag); // Hit/Miss
    results[2] = T2.accessCache(index2, tag); // Hit/Miss
    results[3] = T3.accessCache(index3, tag); // Hit/Miss

    int selectedT = 0;  // Default to T0 if none are active
    int selectedAlt = 0; // Alternate prediction
    
    // Priority check: highest index with result 1
    // Step 1: Find selectedT (highest hit)
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

    bool prediction;
    bool altPred;

    switch (selectedT) {
        case 0:
            prediction = results[0];
            break;
        case 1:
            prediction = T1.accessCtrOnHit(index1, tag);
            break;
        case 2:
            prediction = T2.accessCtrOnHit(index2, tag);
            break;
        case 3:
            prediction = T3.accessCtrOnHit(index3, tag);
            break;
        default:
            std::cerr << "Invalid predictor selected: T" << selectedT << std::endl;
            assert(false);  // Optional: terminate on unexpected case
            break;
    }

    switch (selectedAlt) {
        case 0:
            altPred = results[0];
            break;
        case 1:
            altPred = T1.accessCtrOnHit(index1, tag);
            break;
        case 2:
            altPred = T2.accessCtrOnHit(index2, tag);
            break;
        default:
            std::cerr << "Invalid alternate predictor selected: T" << selectedAlt << std::endl;
            assert(false);  // Optional: terminate on unexpected case
            break;
    }

    // Update Policy
    switch (selectedT) {
        case 0:
            T0.update(PC, branchOutcome);
            break;
        case 1:
            T1.updateCtr(index1, tag, branchOutcome);
            T1.updateUCtr(index1, tag, prediction, altPred, branchOutcome);
            break;
        case 2:
            T2.updateCtr(index2, tag, branchOutcome);
            T2.updateUCtr(index2, tag, prediction, altPred, branchOutcome);
            break;
        case 3:
            T3.updateCtr(index3, tag, branchOutcome);
            T3.updateUCtr(index3, tag, prediction, altPred, branchOutcome);
            break;
        default:
            std::cerr << "Invalid predictor selected: T" << selectedT << std::endl;
            assert(false);  // Optional: terminate on unexpected case
            break;
    }

    PatternHistoryTable* T[] = { &T1, &T2, &T3 };
    uint32_t index[] = { index1, index2, index3 };
    tryAllocate(selectedT, tag, prediction, T, index);

    // Reset counter every n-branch lookup
    T1.resetUCtr();
    T2.resetUCtr();
    T3.resetUCtr();
    
    printConfig();
    printBudget();
    // T0.printBimodal();
    // T1.printCache();
    // T2.printCache();
    // T3.printCache();
    // std::cout << tag << std::endl;
    // std::cout << prediction << std::endl;
    // std::cout << selectedT << std::endl;
    // std::cout << results << std::endl;
    // std::cout << index1 << std::endl;
    // T1.printCacheSet(index1);
    // T2.printCacheSet(index2);
    // T3.printCacheSet(index3);

    return 0;
}