#include "frame_pool.h"

FramePool::FramePool(int num_frames): capacity(num_frames){
    frames.resize(num_frames);
    for (size_t i = 0; i < num_frames; ++i) {
        free_frames.push(i);
    }
}

int FramePool::allocate() {
    if (free_frames.empty()) {
        throw std::runtime_error("No free frames available");
    }
    int index = free_frames.front();
    free_frames.pop();
    frames[index].in_use = true;
    frames[index].ref_count = 1;
    return index;
}

void FramePool::deallocate(int frame_index) {
    if (frame_index < 0 || static_cast<size_t>(frame_index) >= frames.size()) {
        throw std::out_of_range("Invalid frame index");
    }
    if (!frames[frame_index].in_use) {
        throw std::runtime_error("Frame is not in use");
    }
    frames[frame_index].in_use = false;
    frames[frame_index].ref_count = 0;
    frames[frame_index].data.fill(std::byte{0});
    free_frames.push(frame_index);
}

Frame& FramePool::get_frame(int frame_index) {
    if (frame_index < 0 || static_cast<size_t>(frame_index) >= frames.size()) {
        throw std::out_of_range("Invalid frame index");
    }
    if (!frames[frame_index].in_use) {
        throw std::runtime_error("Frame is not in use");
    }
    return frames[frame_index];
}
