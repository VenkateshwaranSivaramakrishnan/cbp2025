#include <iostream>
#include <vector>
#include <cmath>
#include <list>
#include <bitset>
#include <iomanip>
#include "config.hpp"


/**
 * CacheLine class represents a single cache line.
 * Each cache line contains:
 * - valid: A boolean flag to indicate whether the cache line contains valid data.
 * - tag: The tag associated with the cache line. The tag is a part of the memory address.
 * - data: The actual data stored in the cache line.
 */
struct CacheLine {
    bool valid;    // Valid bit: Indicates if the cache line holds valid data
    unsigned int tag;   // Tag: Part of the memory address used to identify the block
    unsigned int data;  // Data: The actual data stored in the cache line
    unsigned int lru;   // LRU: Bits indicating when was the line last used (0 being most recent)
    // TODO: Change data types
    // Include counters
    // LRU bits

    // Constructor to initialize CacheLine with default values
    CacheLine() : valid(false), tag(0), data(0), lru(0) {}
};

/**
 * CacheSet class represents a set in the cache.
 * Each set contains multiple cache lines. The set is indexed by a set index.
 * This class provides methods to find and replace cache lines.
 */
class CacheSet {
private:
    void updateCacheLinesLRU(unsigned int tag) {
        int hitLRU = -1;
        int invalid  = -1;
        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].valid && lines[i].tag == tag) {  // If an invalid cache line is found, replace it
                hitLRU = lines[i].lru;
                lines[i].lru = 0;
            }
            if (lines[i].valid == 0) invalid = 1;
        }

        if (!invalid) {
            for (int i = 0; i < lines.size(); i++) {
                if(lines[i].valid && (lines[i].lru <= hitLRU) && (lines[i].tag != tag)) {
                    lines[i].lru++;
                }
            }
        }
        else {
            for (int i = 0; i < lines.size(); i++) {
                if(lines[i].valid && (lines[i].tag != tag)) {
                    lines[i].lru++;
                }
            }
        }
    }
public:
    std::vector<CacheLine> lines;  // Vector of cache lines in the set
    unsigned int setIndex;  // The index of this set in the cache

    // Constructor to initialize CacheSet with the specified number of lines per set
    CacheSet(unsigned int numLines) {
        lines.resize(numLines);  // Initialize the set with the given number of cache lines
    }

    /**
     * Find a cache line that matches the given tag.
     * @param tag The tag to search for in the set.
     * @return The index of the cache line if found, otherwise -1 (cache miss).
     */
    int findCacheLine(unsigned int tag) {
        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].valid && lines[i].tag == tag) {
                updateCacheLinesLRU(tag); // Update cache lines (LRU)
                return i;  // Cache hit: return the index of the cache line
            }
        }
        return -1;  // Cache miss: Tag not found in the set
    }

    /**
     * Replace a cache line in the set with the given tag and data.
     * This function is called when a cache miss occurs, and the set is full.
     * @param tag The tag to store in the cache line.
     * @param data The data to store in the cache line.
     */
    void replaceCacheLine(unsigned int tag, unsigned int data) {
        for (int i = 0; i < lines.size(); i++) {
            if (!lines[i].valid) {  // If an invalid cache line is found, replace it
                lines[i].valid = true;
                lines[i].tag = tag;
                lines[i].data = data;
                updateCacheLinesLRU(tag);
                return;
            }
        }

        // If all lines are valid, enforce LRU replacement policy (implies all N-ways are occupied)
        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].valid && (lines[i].lru == config::CACHE_N_LRU_MAX)) {
                lines[i].valid = true;
                lines[i].tag = tag;
                lines[i].data = data;
                updateCacheLinesLRU(tag);
                return;
            }
        }
    }
};

/**
 * SetAssociativeCache class simulates a set-associative cache.
 * It consists of multiple sets and provides methods for cache access and updates.
 */
class SetAssociativeCache {
private:
    std::vector<CacheSet> sets;   // List of cache sets
    unsigned int numSets;     // Total number of sets in the cache
    unsigned int associativity;  // Number of cache lines per set

public:
    /**
     * Constructor to initialize the cache with the given number of sets and cache lines per set.
     * @param numSets The number of sets in the cache.
     * @param associativity The number of cache lines per set.
     */
    SetAssociativeCache(unsigned int numSets, unsigned int associativity) 
        : numSets(numSets), associativity(associativity) {
        sets.resize(numSets, CacheSet(associativity));  // Initialize each set with the specified number of lines
    }

    /**
     * Access the cache with the given memory address.
     * If the cache line is present (cache hit), return true.
     * If the cache line is not present (cache miss), replace an existing line and return false.
     * @param address The memory address to access.
     * @param data The data to store in the cache (used on cache miss).
     * @return true if the access was a cache hit, false if it was a cache miss.
     */
    bool accessCache(unsigned int index, unsigned int tag, unsigned int data) {

        // Check the cache set for a hit or miss
        int lineIndex = sets[index].findCacheLine(tag);

        if (lineIndex != -1) {  // Cache hit
            std::cout << "Cache Hit at Set " << index << ", Line " << lineIndex << std::endl;          
            return true;
        } else {  // Cache miss
            std::cout << "Cache Miss at Set " << index << std::endl;
            sets[index].replaceCacheLine(tag, data);  // Replace the cache line
            return false;
        }
    }

    std::string formatValue(unsigned int value, bool binary = false, std::size_t width = 8) {
        if (binary) {
            return std::bitset<32>(value).to_string().substr(32 - width);  // Keep only lower `width` bits
        } else {
            return std::to_string(value);
        }
    }
    
    void printLineHeader(bool binary) {
        std::size_t totalWidth = 0;
        totalWidth += 8 + 8 + 8;  // Set, Line, Valid
    
        if (binary) {
            totalWidth += config::CACHE_N_LRU_BITS + 4;
            totalWidth += config::CACHE_N_TAG_WIDTH + 4;
            totalWidth += config::CACHE_N_TAG_WIDTH + 4;
        } else {
            totalWidth += 12 + 12 + 12;
        }
        std::cout << std::left
                  << std::setw(8) << "Set"
                  << std::setw(8) << "Line"
                  << std::setw(8) << "Valid"
                  << std::setw(binary ? config::CACHE_N_LRU_BITS + 4 : 12) << "LRU"
                  << std::setw(binary ? config::CACHE_N_TAG_WIDTH + 4 : 12) << "Tag"
                  << std::setw(binary ? config::CACHE_N_TAG_WIDTH + 4 : 12) << "Data"
                  << "\n";
        std::cout << std::string(totalWidth, '-') << "\n";
    }

    void printCache(bool binary = false) {
        printLineHeader(binary);
    
        for (std::size_t setIdx = 0; setIdx < sets.size(); ++setIdx) {
            const auto& set = sets[setIdx];
            for (std::size_t lineIdx = 0; lineIdx < set.lines.size(); ++lineIdx) {
                const auto& line = set.lines[lineIdx];
    
                std::cout << std::left
                          << std::setw(8) << setIdx
                          << std::setw(8) << lineIdx
                          << std::setw(8) << (line.valid ? "Y" : "N")
                          << std::setw(binary ? config::CACHE_N_LRU_BITS + 4 : 12) << formatValue(line.lru, binary, config::CACHE_N_LRU_BITS)
                          << std::setw(binary ? config::CACHE_N_TAG_WIDTH + 4 : 12) << formatValue(line.tag, binary, config::CACHE_N_TAG_WIDTH)
                          << std::setw(binary ? config::CACHE_N_TAG_WIDTH + 4 : 12) << formatValue(line.data, binary, config::CACHE_N_TAG_WIDTH)
                          << "\n";
            }
        }
        std::size_t totalWidth = 0;
        totalWidth += 8 + 8 + 8;  // Set, Line, Valid
    
        if (binary) {
            totalWidth += config::CACHE_N_LRU_BITS + 4;
            totalWidth += config::CACHE_N_TAG_WIDTH + 4;
            totalWidth += config::CACHE_N_TAG_WIDTH + 4;
        } else {
            totalWidth += 12 + 12 + 12;
        }
        std::cout << std::string(totalWidth, '=') << "\n";
    }
};

/**
 * Main function that simulates accessing the cache with different memory addresses.
 */
int main() {
    if (config::DEBUG >= 1) {
        printConfig();
    }
    SetAssociativeCache cache(config::CACHE_N_SET, config::CACHE_N_ASSOC);  // Create the cache

    // Simulate cache accesses
    cache.accessCache(1, 1, 100);  // Access memory address 1 (store data 100)
    cache.printCache(config::PRINT_FORMAT_BINARY);
    cache.accessCache(1, 2, 200);  // Access memory address 2 (store data 200)
    cache.printCache(config::PRINT_FORMAT_BINARY);
    cache.accessCache(1, 3, 300);  // Access memory address 1 (cache hit)
    cache.printCache(config::PRINT_FORMAT_BINARY);
    cache.accessCache(1, 4, 400);  // Access memory address 3 (store data 400)
    cache.printCache(config::PRINT_FORMAT_BINARY);
    cache.accessCache(1, 1, 100);  // Access memory address 1 (store data 100)
    cache.printCache(config::PRINT_FORMAT_BINARY);
    cache.accessCache(1, 5, 500);  // Access memory address 4 (store data 500)
    //cache.accessCache(5, 600);  // Access memory address 5 (store data 600)

    if (config::DEBUG >= 1) {
        cache.printCache(config::PRINT_FORMAT_BINARY);  // pass binary/decimal flag
    }

    return 0;

}

// TODO: Logic to print the entire table

