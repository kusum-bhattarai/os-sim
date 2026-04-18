#pragma once

// Pure virtual base class for page replacement algorithms.
class ReplacementPolicy {
public:
    virtual ~ReplacementPolicy() = default;

    // called when a page is loaded into a frame
    virtual void on_load(int frame_index, int vpn) = 0;

    // called on every page access, hit, or fault
    virtual void on_access(int frame_index) = 0;

    // returns the frame index to evict
    virtual int evict() = 0;

    // reset internal state for clean experiments
    virtual void reset() = 0;
};