#include "slab_allocator.h"
#include <algorithm>
#include <stdexcept>

int SlabAllocator::size_class_for(size_t size) {
    if (size == 0) {
        return -1;
    }
    for (size_t cls = 0; cls < SLAB_SIZE_CLASSES.size(); cls++) {
        if (size <= SLAB_SIZE_CLASSES[cls]) {
            return static_cast<int>(cls);
        }
    }
    return -1;
}

int SlabAllocator::allocate(size_t size) {
    int cls = size_class_for(size);
    if (cls == -1) {
        throw std::invalid_argument("No slab size class fits this allocation");
    }
    size_t object_size = SLAB_SIZE_CLASSES[cls];

    Slab* slab = nullptr;
    for (auto& s : caches[cls]) {
        if (!s.free_slots.empty()) {
            slab = &s;
            break;
        }
    }
    if (slab == nullptr) {
        int frame = pool.allocate();
        caches[cls].emplace_back(frame, static_cast<int>(PAGE_SIZE / object_size));
        frame_to_class[frame] = cls;
        slab = &caches[cls].back();
    }

    int slot = slab->free_slots.back();
    slab->free_slots.pop_back();
    slab->in_use++;
    int address = slab->frame_index * PAGE_SIZE + slot * object_size;
    requested[address] = size;
    return address;
}

void SlabAllocator::free(int address) {
    auto req = requested.find(address);
    if (req == requested.end()) {
        throw std::runtime_error("Address was not allocated");
    }
    int frame = address / PAGE_SIZE;
    int cls = frame_to_class.at(frame);
    size_t object_size = SLAB_SIZE_CLASSES[cls];
    int slot = (address % PAGE_SIZE) / object_size;

    auto& cache = caches[cls];
    auto it = std::find_if(cache.begin(), cache.end(), [&](const Slab& s){
        return s.frame_index == frame;
    });
    it->free_slots.push_back(slot);
    it->in_use--;
    requested.erase(req);

    if (it->in_use == 0) {
        pool.deallocate(frame);
        frame_to_class.erase(frame);
        cache.erase(it);
    }
}
