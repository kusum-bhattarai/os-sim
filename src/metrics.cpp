#include "metrics.h"
#include <iostream>

void Metrics::print() const {
    int total_tlb = tlb_hits + tlb_misses;
    double tlb_hit_rate = total_tlb > 0 ? 100.0 * tlb_hits / total_tlb : 0.0;
    std::cout << "Page Faults:     " << page_faults       << "\n"
              << "Hits:            " << hits               << "\n"
              << "Evictions:       " << evictions          << "\n"
              << "CoW Forks:       " << cow_forks          << "\n"
              << "CoW Copies:      " << cow_copies         << "\n"
              << "Copies Avoided:  " << cow_copies_avoided << "\n"
              << "TLB Hits:        " << tlb_hits           << "\n"
              << "TLB Misses:      " << tlb_misses         << "\n"
              << "TLB Hit Rate:    " << tlb_hit_rate       << "%\n"
              << "WS Trims:        " << ws_trims           << "\n";
}

void Metrics::reset() {
    page_faults = 0;
    hits = 0;
    evictions = 0;
    cow_forks = 0;
    cow_copies = 0;
    cow_copies_avoided = 0;
    tlb_hits = 0;
    tlb_misses = 0;
    ws_trims = 0;
}

const Metrics& Metrics::get_metrics() const {
    return *this;
}