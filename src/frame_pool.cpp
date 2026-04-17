#include "frame_pool.h"

class FramePool {
private:
    std::array<Frame, NUM_FRAMES> frames;
    std::queue<int> free_frames;
public:
    FramePool() {
        for (size_t i = 0; i < frames.size(); ++i) {
            free_frames.push(i);
        }
    }

    // allocate a frame and return its index
    int allocate(){
        if (free_frames.empty()) {
            throw std::runtime_error("No free frames available");
        }
        int index = free_frames.front();
        free_frames.pop();
        frames[index].in_use = true;
        frames[index].ref_count = 1;
        return index;
    }

    // free a frame by index
    void deallocate(int frame_index){
        if (frame_index < 0 || frame_index >= frames.size()) {
            throw std::out_of_range("Invalid frame index");
        }
        if (!frames[frame_index].in_use) {
            throw std::runtime_error("Frame is not in use");
        }
        frames[frame_index].in_use = false;
        frames[frame_index].ref_count = 0;
        free_frames.push(frame_index);
        frames[frame_index].data.fill(std::byte{0});
    }

    // get a frame by index
    Frame& get_frame(int frame_index){
        if (frame_index < 0 || frame_index >= frames.size()) {
            throw std::out_of_range("Invalid frame index");
        }
        if (!frames[frame_index].in_use) {
            throw std::runtime_error("Frame is not in use");
        }
        return frames[frame_index];
    }
};
