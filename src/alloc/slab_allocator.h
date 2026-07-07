#pragma once
#include <array>
#include <unordered_map>
#include <vector>
#include "../frame_pool.h"

// fixed-size object caches carved out of physical frames: one slab = one frame
static constexpr std::array<size_t, 5> SLAB_SIZE_CLASSES = {16, 32, 64, 128, 256};

struct Slab {
    int frame_index;
    std::vector<int> free_slots; // LIFO stack of free object slots
    int in_use;
    Slab(int frame_index, int objects_per_slab) : frame_index(frame_index), in_use(0) {
        for (int i = objects_per_slab - 1; i >= 0; i--) free_slots.push_back(i);
    }
};

class SlabAllocator {
private:
    FramePool& pool;
    std::array<std::vector<Slab>, SLAB_SIZE_CLASSES.size()> caches;
    std::unordered_map<int, size_t> requested;      // address -> requested bytes
    std::unordered_map<int, int> frame_to_class;    // frame -> size class index

public:
    explicit SlabAllocator(FramePool& pool) : pool(pool) {}

    // returns the physical address of the object
    int allocate(size_t size);
    void free(int address);

    // smallest size class holding `size`, -1 if it exceeds the largest class
    static int size_class_for(size_t size);

    size_t class_size(int cls) const { return SLAB_SIZE_CLASSES[cls]; }
    int slab_count(int cls) const { return static_cast<int>(caches[cls].size()); }

    int objects_in_use(int cls) const {
        int n = 0;
        for (const auto& slab : caches[cls]) n += slab.in_use;
        return n;
    }

    int frames_used() const {
        int n = 0;
        for (const auto& cache : caches) n += static_cast<int>(cache.size());
        return n;
    }

    // internal fragmentation: reserved bytes minus what callers asked for
    size_t bytes_reserved() const {
        size_t total = 0;
        for (size_t cls = 0; cls < caches.size(); cls++)
            for (const auto& slab : caches[cls])
                total += slab.in_use * SLAB_SIZE_CLASSES[cls];
        return total;
    }

    size_t bytes_requested() const {
        size_t total = 0;
        for (const auto& [addr, size] : requested) total += size;
        return total;
    }
};
