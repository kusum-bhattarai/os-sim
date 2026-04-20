#include "metrics.h"
#include <iostream>

void Metrics::print() const {
    std::cout << "Page Faults:     " << page_faults      << "\n"
              << "Hits:            " << hits              << "\n"
              << "Evictions:       " << evictions         << "\n"
              << "CoW Forks:       " << cow_forks         << "\n"
              << "CoW Copies:      " << cow_copies        << "\n"
              << "Copies Avoided:  " << cow_copies_avoided << "\n";
}

void Metrics::reset() {
    page_faults = 0;
    hits = 0;
    evictions = 0;
    cow_forks = 0;
    cow_copies = 0;
    cow_copies_avoided = 0;
}

const Metrics& Metrics::get_metrics() const {
    return *this;
}