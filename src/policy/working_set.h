#pragma once
#include "replacement_policy.h"
#include <unordered_map>
#include <vector>

// sliding-window working set: frames not accessed within the last
// `window` accesses fall out of the working set and become eviction
// candidates even when free frames remain
class WorkingSetPolicy : public ReplacementPolicy {
private:
    std::unordered_map<int, long long> last_access; // frame_index -> virtual time
    long long clock = 0;
    int window;

public:
    explicit WorkingSetPolicy(int window = 8) : window(window) {}

    void on_load(int frame_index, int vpn) override;
    void on_access(int frame_index) override;
    int evict() override;
    void reset() override;

    // frames whose last access is outside the window
    std::vector<int> expired_frames() const;

    // frames accessed within the window
    int working_set_size() const;

    long long current_time() const { return clock; }
    int get_window() const { return window; }

    // forget a frame trimmed outside the evict() path
    void remove(int frame_index) { last_access.erase(frame_index); }
};
