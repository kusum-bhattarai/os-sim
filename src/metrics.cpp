#include "metrics.h"
#include <iostream>

void Metrics::print() const {
    std::cout << "Metrics Summary:" << std::endl;
    std::cout << "- Page Faults: " << page_faults << std::endl;
    std::cout << "- Hits: " << hits << std::endl;
    std::cout << "- Evictions: " << evictions << std::endl;
}

void Metrics::reset() {
    page_faults = 0;
    hits = 0;
    evictions = 0;
}

const Metrics& Metrics::get_metrics() const {
    return *this;
}