#include "exp_utils.h"
#include "page_table_hashed.h"
#include "constants.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

struct Result {
    int entries;
    int max_chain;
    double avg_probes;
};

static Result run(const std::vector<int>& vpns) {
    HashedPageTable pt;
    for (int vpn : vpns)
        pt.insert(vpn, vpn % NUM_FRAMES);

    pt.reset_probe_stats();
    for (int vpn : vpns)
        pt.lookup(vpn);

    double avg = static_cast<double>(pt.total_probes()) / pt.total_lookups();
    return { static_cast<int>(vpns.size()), pt.max_chain_length(), avg };
}

static void print_row(const std::string& name, const Result& r) {
    std::cout << std::left  << std::setw(22) << name
              << std::right << std::setw(9)  << r.entries
                            << std::setw(11) << r.max_chain
                            << std::setw(12) << std::fixed << std::setprecision(2) << r.avg_probes << "\n";
}

int main() {
    // Sequential: VPNs 0..15 — each lands in its own bucket
    std::vector<int> sequential;
    for (int i = 0; i < 16; i++) sequential.push_back(i);

    // Strided: VPNs 0, 64, 128, ... — all collide in bucket 0
    std::vector<int> strided;
    for (int i = 0; i < 16; i++) strided.push_back(i * HASHED_BUCKETS);

    // Full: every virtual page mapped — average chain = pages / buckets
    std::vector<int> full;
    for (int i = 0; i < static_cast<int>(NUM_VIRTUAL_PAGES); i++) full.push_back(i);

    print_exp_header(
        "Experiment J: Hashed Page Table Collisions",
        "64 buckets, chained; avg probes per lookup"
    );

    std::cout << std::left  << std::setw(22) << "Workload"
              << std::right << std::setw(9)  << "Entries"
                            << std::setw(11) << "Max chain"
                            << std::setw(12) << "Avg probes" << "\n";
    print_separator('-');

    print_row("Sequential (0..15)",  run(sequential));
    print_row("Strided (x64)",       run(strided));
    print_row("Full (1024 pages)",   run(full));

    std::cout << "\nSequential VPNs spread across buckets: 1 probe per lookup.\n";
    std::cout << "A stride equal to the bucket count is pathological: every\n";
    std::cout << "entry chains in bucket 0 and lookups scan half the chain.\n";
    print_exp_footer();
    return 0;
}
