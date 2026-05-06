#include "exp_utils.h"
#include "memory_manager.h"
#include "policy/lru.h"
#include <iomanip>
#include <vector>

// 10-page cyclic trace: pages 0,1,...,9,0,1,...
// With enough frames there are no evictions and the TLB stays warm.
// With a tight frame count every eviction shoots down a TLB entry,
// forcing a miss the next time that page comes back around.
static constexpr int PAGES    = 10;
static constexpr int ACCESSES = 100;

int main() {
    print_exp_header(
        "Experiment H: TLB Shootdown Impact on Hit Rate",
        "10-page cyclic trace, 100 accesses"
    );

    std::vector<int> seq;
    for (int i = 0; i < ACCESSES; i++) seq.push_back(i % PAGES);

    std::cout << std::left  << std::setw(24) << "Scenario"
              << std::right << std::setw(10) << "Evictions"
                            << std::setw(10) << "TLB Hits"
                            << std::setw(12) << "TLB Misses"
                            << std::setw(12) << "Hit Rate" << "\n";
    std::cout << std::string(68, '-') << "\n";

    auto run = [&](const std::string& label, int frames) {
        MemoryManager mm(new LRU(), frames);
        mm.create_process(1);
        for (int vpn : seq) mm.access(1, vpn * PAGE_SIZE, false);
        const auto& m = mm.get_metrics();
        int total = m.tlb_hits + m.tlb_misses;
        double rate = total > 0 ? 100.0 * m.tlb_hits / total : 0.0;
        std::cout << std::left  << std::setw(24) << label
                  << std::right << std::setw(10) << m.evictions
                                << std::setw(10) << m.tlb_hits
                                << std::setw(12) << m.tlb_misses
                                << std::setw(11) << std::fixed << std::setprecision(1) << rate << "%\n";
    };

    run("Generous (10 frames)", 10);
    run("Moderate (6 frames)",   6);
    run("Tight (3 frames)",      3);

    print_exp_footer();
    return 0;
}
