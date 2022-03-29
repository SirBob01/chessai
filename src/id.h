#ifndef ID_H_
#define ID_H_

#include <cstdint>
#include <vector>

/**
 * Generates a unique ID and reuses old ones
 */
class IDGen {
    std::vector<uint64_t> _unused;
    uint64_t _id_counter = 0;

  public:
    /**
     * Get a unique ID
     */
    uint64_t get_id();

    /**
     * Unregister an ID so it can be recycled in the future
     */
    void unregister_id(uint64_t id);
};

#endif