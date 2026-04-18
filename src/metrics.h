struct Metrics {
    int page_faults = 0;
    int hits = 0;
    int evictions = 0;

    void print() const;  // prints a summary table
    void reset();
};