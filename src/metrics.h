#pragma once

struct Metrics {
    int page_faults = 0;
    int hits = 0;
    int evictions = 0;
    int cow_forks = 0;
    int cow_copies = 0;
    int cow_copies_avoided = 0;
    int tlb_misses = 0;
    int tlb_hits = 0;
    int ws_trims = 0;

    void print() const;  // prints a summary table
    void reset();
    const Metrics& get_metrics() const;
};