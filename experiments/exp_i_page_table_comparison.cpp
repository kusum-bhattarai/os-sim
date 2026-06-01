#include "exp_utils.h"
#include "memory_manager.h"
#include "page_table_two_level.h"
#include "policy/lru.h"
#include <iostream>
#include <iomanip>
#include <vector>

static constexpr int PID = 1;
static constexpr int FRAMES = 64;

struct Result {
    int page_faults;
    int hits;
    int flat_entries;
    int l2_tables;
    int l2_slots;
};

static Result run(const std::vector<int>& vpns, PageTableType pt_type) {
    MemoryManager mm(new LRU(), FRAMES);
    mm.create_process(PID, pt_type);
    for (int vpn : vpns)
        mm.access(PID, vpn * PAGE_SIZE, false);

    const Metrics& m = mm.get_metrics();
    const IPageTable& pt = mm.get_process(PID).get_page_table();

    int flat_entries = 0;
    int l2_tables = 0;

    if (pt_type == PageTableType::FLAT) {
        flat_entries = static_cast<int>(pt.get_entries().size());
    } else {
        l2_tables = dynamic_cast<const TwoLevelPageTable&>(pt).l2_table_count();
    }

    return { m.page_faults, m.hits, flat_entries, l2_tables, l2_tables * L2_SIZE };
}

static void print_header() {
    std::cout << std::left  << std::setw(14) << "Impl"
              << std::right << std::setw(8)  << "Faults"
                            << std::setw(6)  << "Hits"
                            << std::setw(10) << "Entries"
                            << std::setw(11) << "L2 tables"
                            << std::setw(10) << "L2 slots" << "\n";
    print_separator('-');
}

static void print_row(const std::string& name, const Result& r) {
    std::cout << std::left  << std::setw(14) << name
              << std::right << std::setw(8)  << r.page_faults
                            << std::setw(6)  << r.hits
                            << std::setw(10) << (r.flat_entries > 0 ? r.flat_entries : r.l2_tables * L2_SIZE)
                            << std::setw(11) << r.l2_tables
                            << std::setw(10) << r.l2_slots << "\n";
}

int main() {
    // Sparse: one VPN per L1 slot (VPNs 0, 32, 64, ... — each in a different L1 slot)
    std::vector<int> sparse;
    for (int i = 0; i < 16; i++) sparse.push_back(i * L2_SIZE);

    // Compact: 16 VPNs all within L1 slot 0 (VPNs 0..15)
    std::vector<int> compact;
    for (int i = 0; i < 16; i++) compact.push_back(i);

    print_exp_header(
        "Experiment I: Flat vs Two-Level Page Table",
        "16 pages accessed, 64 frames (no eviction)"
    );

    std::cout << "\nSparse workload (each VPN in a different L1 slot):\n";
    print_header();
    {
        auto r = run(sparse, PageTableType::FLAT);
        std::cout << std::left << std::setw(14) << "Flat"
                  << std::right << std::setw(8) << r.page_faults
                                << std::setw(6) << r.hits
                                << std::setw(10) << r.flat_entries
                                << std::setw(11) << "-"
                                << std::setw(10) << "-" << "\n";
    }
    {
        auto r = run(sparse, PageTableType::TWO_LEVEL);
        std::cout << std::left << std::setw(14) << "Two-Level"
                  << std::right << std::setw(8) << r.page_faults
                                << std::setw(6) << r.hits
                                << std::setw(10) << "-"
                                << std::setw(11) << r.l2_tables
                                << std::setw(10) << r.l2_slots << "\n";
    }

    std::cout << "\nCompact workload (all VPNs in L1 slot 0):\n";
    print_header();
    {
        auto r = run(compact, PageTableType::FLAT);
        std::cout << std::left << std::setw(14) << "Flat"
                  << std::right << std::setw(8) << r.page_faults
                                << std::setw(6) << r.hits
                                << std::setw(10) << r.flat_entries
                                << std::setw(11) << "-"
                                << std::setw(10) << "-" << "\n";
    }
    {
        auto r = run(compact, PageTableType::TWO_LEVEL);
        std::cout << std::left << std::setw(14) << "Two-Level"
                  << std::right << std::setw(8) << r.page_faults
                                << std::setw(6) << r.hits
                                << std::setw(10) << "-"
                                << std::setw(11) << r.l2_tables
                                << std::setw(10) << r.l2_slots << "\n";
    }

    std::cout << "\nSparse: two-level allocates 1 L2 table (32 slots) per VPN touched.\n";
    std::cout << "Compact: two-level allocates 1 L2 table for all 16 VPNs (shared slot).\n";
    print_exp_footer();
    return 0;
}
