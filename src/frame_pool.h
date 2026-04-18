#pragma once
#include <array>
#include <cstddef>
#include <queue>
#include <stdexcept>
#include "constants.h"

struct Frame {
    std::array<std::byte, PAGE_SIZE> data;
    bool in_use;
    int ref_count;
    Frame() : in_use(false), ref_count(0) {}
};

class FramePool {
private:
    std::vector<Frame> frames;
    std::queue<int> free_frames;
    int capacity;
    
public:
    FramePool(int num_frames = NUM_FRAMES);

    // allocate a frame and return its index
    int allocate();

    // free a frame by index
    void deallocate(int frame_index);

    // get a frame by index
    Frame& get_frame(int frame_index);
};
