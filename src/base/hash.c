#include <iostream>
#include <vector>
#include <cstdint> // Include this for uint8_t

using namespace std;

// Function to extract bits from the simulated PHR register
uint8_t extractPHR(int start, int end, int step, const vector<uint8_t>& PHR) {
    uint8_t result = 0;
    int index = 0;

    for (int i = start; i >= end; i -= step) {
        if (i >= 0 && i < PHR.size()) { // Ensure within bounds
            result |= (PHR[i] << index);  // Extract bits and shift
        }
        index++;
    }

    return result;
}

int main() {
    // Example PHR array (filled with random values for demonstration)
    vector<uint8_t> PHR(256, 0xA5);  // Simulated register with 256 entries

    cout << "PHT #3 Index values:\n";

    // Iterate over all i and j values
    for (int i = 11; i >= 1; --i) {
        for (int j = 11; j >= 0; --j) {
            uint8_t PHR_i = extractPHR(16 * i + 8, 16 * i - 6, 2, PHR);
            uint8_t PHR_j = extractPHR(16 * j + 1, 16 * j - 13, 2, PHR);

            uint8_t index = PHR_i ^ PHR_j; // XOR operation

            cout << "i = " << i << ", j = " << j
                 << " -> Index[7:0] = 0x" << hex << (int)index << dec << endl;
        }
    }

    return 0;
}
