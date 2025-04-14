/**
 * CacheLine class represents a single cache line.
 * Each cache line contains:
 * - valid: A boolean flag to indicate whether the cache line contains valid data.
 * - tag: The tag associated with the cache line. The tag is a part of the memory address.
 * - ctr: The branch counter stored in the cache line.
 * - u: The usefulness counter of the cache line.
 */
struct CacheLine {
    bool valid;    // Valid bit: Indicates if the cache line holds valid data
    unsigned int tag;   // Tag: Part of the memory address used to identify the block
    unsigned int ctr;  // Data: The actual data stored in the cache line
    unsigned int u;     // u: Usefulness counter

    // Constructor to initialize CacheLine with default values
    CacheLine() : valid(false), tag(0), ctr(0), u(0) {}
};

/**
 * CacheSet class represents a set in the cache.
 * Each set contains multiple cache lines. The set is indexed by a set index.
 * This class provides methods to find and replace cache lines.
 */
class CacheSet {
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
     * @return The way of the cache line if found im the set, otherwise -1 (cache miss).
     */
    int findCacheLine(unsigned int tag) {
        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].valid && lines[i].tag == tag) {
                return i;  // Cache hit: return the way of the cache line in the set
            }
        }
        return -1;  // Cache miss: Tag not found in the set
    }
};

/**
 * SetAssociativeCache class simulates a set-associative cache.
 * It consists of multiple sets and provides methods for cache access and updates.
 */
class SetAssociativeCache {
protected:
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

    public:
    /**
     * @brief Provides access to the cache set at the specified index.
     *
     * @param index Index of the set.
     * @return Reference to the CacheSet.
     */
    CacheSet& getSet(std::size_t index) {
        return sets.at(index);
    }

    const CacheSet& getSet(std::size_t index) const {
        return sets.at(index);
    }

    /**
     * Access the cache with the given memory address.
     * If the cache line is present (cache hit), return true.
     * If the cache line is not present (cache miss), replace an existing line and return false.
     * @param index The index of cache set to access.
     * @param tag The tag to search for in the set.
     * @return true if the access was a cache hit, false if it was a cache miss.
     */
    bool accessCache(unsigned int index, unsigned int tag) {

        // Check the cache set for a hit or miss
        int lineIndex = sets[index].findCacheLine(tag);

        if (lineIndex != -1) {  // Cache hit
            //std::cout << "Cache Hit at Set " << index << ", Line " << lineIndex << std::endl;          
            return true;
        } else {  // Cache miss
            //std::cout << "Cache Miss at Set " << index << std::endl;
            return false;
        }
    }

    /**
     * Find and return the ctr value on Cache Hit.
     * @param index The index of cache set to access.
     * @param tag The tag to search for in the set.
     * @return The ctr value of the cache line if found, otherwise -1 (cache miss).
     */
     int accessCtrOnHit(unsigned int index, unsigned int tag) {
        for (int i = 0; i < sets[index].lines.size(); i++) {
            if (sets[index].lines[i].valid && sets[index].lines[i].tag == tag) {
                return sets[index].lines[i].ctr;  // Cache hit: return the index of the cache line
            }
        }
        // No hit found
        std::cerr << "accessCtrOnHit invoked without Cache Hit: No matching tag found in any cache line.\n";
        assert(false);  // Triggers program termination
        return -1;      // Optional: suppress compiler warning
    }

    /**
     * Find and return the u value on Cache Hit.
     * @param index The index of cache set to access.
     * @param tag The tag to search for in the set.
     * @return The u value of the cache line if found, otherwise -1 (cache miss).
     */
    int accessUOnHit(unsigned int index, unsigned int tag) {
        for (int i = 0; i < sets[index].lines.size(); i++) {
            if (sets[index].lines[i].valid && sets[index].lines[i].tag == tag) {
                return sets[index].lines[i].u;  // Cache hit: return the index of the cache line
            }
        }
        // No hit found
        std::cerr << "accessUOnHit invoked without Cache Hit: No matching tag found in any cache line.\n";
        assert(false);  // Triggers program termination
        return -1;      // Optional: suppress compiler warning
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
            totalWidth += config::CACHE_N_TAG_WIDTH + 4;
            totalWidth += config::CACHE_N_CTR_WIDTH + 4;
            totalWidth += config::CACHE_N_U_WIDTH + 4;
        } else {
            totalWidth += 12 + 12 + 12;
        }
        std::cout << std::left
                  << std::setw(8) << "Set"
                  << std::setw(8) << "Line"
                  << std::setw(8) << "Valid"
                  << std::setw(binary ? config::CACHE_N_TAG_WIDTH + 4 : 12) << "Tag"
                  << std::setw(binary ? config::CACHE_N_CTR_WIDTH + 4 : 12) << "Ctr"
                  << std::setw(binary ? config::CACHE_N_U_WIDTH + 4 : 12) << "u"
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
                          << std::setw(binary ? config::CACHE_N_TAG_WIDTH + 4 : 12) << formatValue(line.tag, binary, config::CACHE_N_TAG_WIDTH)
                          << std::setw(binary ? config::CACHE_N_CTR_WIDTH + 4 : 12) << formatValue(line.ctr, binary, config::CACHE_N_CTR_WIDTH)
                          << std::setw(binary ? config::CACHE_N_U_WIDTH + 4 : 12) << formatValue(line.u, binary, config::CACHE_N_U_WIDTH)
                          << "\n";
            }
        }
        std::size_t totalWidth = 0;
        totalWidth += 8 + 8 + 8;  // Set, Line, Valid
    
        if (binary) {
            totalWidth += config::CACHE_N_TAG_WIDTH + 4;
            totalWidth += config::CACHE_N_CTR_WIDTH + 4;
            totalWidth += config::CACHE_N_U_WIDTH + 4;
        } else {
            totalWidth += 12 + 12 + 12;
        }
        std::cout << std::string(totalWidth, '=') << "\n";
    }

    /**
     * @brief Prints the contents of a specific cache set by index.
     *
     * This function displays all the cache lines within the set identified by `setIdx`.
     * It supports binary and decimal formatting modes based on the `binary` flag.
     *
     * @param setIdx The index of the set to print.
     * @param binary If true, displays values in binary format; otherwise, in decimal format.
     */
    void printCacheSet(std::size_t setIdx, bool binary = false) {
        if (setIdx >= sets.size()) {
            std::cerr << "[ERROR] Invalid set index: " << setIdx << std::endl;
            return;
        }

        printLineHeader(binary);

        const auto& set = sets[setIdx];
        for (std::size_t lineIdx = 0; lineIdx < set.lines.size(); ++lineIdx) {
            const auto& line = set.lines[lineIdx];

            std::cout << std::left
                      << std::setw(8) << setIdx
                      << std::setw(8) << lineIdx
                      << std::setw(8) << (line.valid ? "Y" : "N")
                      << std::setw(binary ? config::CACHE_N_TAG_WIDTH + 4 : 12)
                      << formatValue(line.tag, binary, config::CACHE_N_TAG_WIDTH)
                      << std::setw(binary ? config::CACHE_N_CTR_WIDTH + 4 : 12)
                      << formatValue(line.ctr, binary, config::CACHE_N_CTR_WIDTH)
                      << std::setw(binary ? config::CACHE_N_U_WIDTH + 4 : 12)
                      << formatValue(line.u, binary, config::CACHE_N_U_WIDTH)
                      << "\n";
        }

        std::size_t totalWidth = 8 + 8 + 8;
        totalWidth += binary
            ? config::CACHE_N_TAG_WIDTH + 4 + config::CACHE_N_CTR_WIDTH + 4 + config::CACHE_N_U_WIDTH + 4
            : 12 + 12 + 12;

        std::cout << std::string(totalWidth, '=') << "\n";
    }
};