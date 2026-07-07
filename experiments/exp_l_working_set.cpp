#include "exp_utils.h"
#include "memory_manager.h"
#include "policy/working_set.h"
#include "policy/lru.h"
#include <iostream>
#include <iomanip>

static constexpr int PID = 1;
static constexpr int FRAMES = 32;
static constexpr int WINDOW = 12;
static constexpr int ROUNDS = 10;
static constexpr int PAGES_PER_PHASE = 8;

static int frames_in_use(const MemoryManager& mm) {
    const FramePool& fp = mm.get_frame_pool();
    int n = 0;
    for (int i = 0; i < fp.get_capacity(); i++)
        if (fp.peek_frame(i).in_use) n++;
    return n;
}

int main() {
    MemoryManager ws_mm(new WorkingSetPolicy(WINDOW), FRAMES);
    MemoryManager lru_mm(new LRU(), FRAMES);
    ws_mm.create_process(PID);
    lru_mm.create_process(PID);

    print_exp_header(
        "Experiment L: Working Set Model",
        "two 8-page phases, 32 frames, window=12"
    );

    std::cout << std::right << std::setw(7)  << "Round"
              << std::left  << "  " << std::setw(10) << "Phase"
              << std::right << std::setw(10) << "WS held"
                            << std::setw(11) << "LRU held"
                            << std::setw(9)  << "Trims" << "\n";
    print_separator('-');

    for (int round = 0; round < ROUNDS; round++) {
        int base = round < ROUNDS / 2 ? 0 : PAGES_PER_PHASE;
        for (int i = 0; i < PAGES_PER_PHASE; i++) {
            ws_mm.access(PID, (base + i) * PAGE_SIZE, false);
            lru_mm.access(PID, (base + i) * PAGE_SIZE, false);
        }
        ws_mm.trim_working_set();

        std::cout << std::right << std::setw(7)  << round + 1
                  << std::left  << "  " << std::setw(10)
                  << (base == 0 ? "A (0-7)" : "B (8-15)")
                  << std::right << std::setw(10) << frames_in_use(ws_mm)
                                << std::setw(11) << frames_in_use(lru_mm)
                                << std::setw(9)  << ws_mm.get_metrics().ws_trims << "\n";
    }

    print_separator('-');
    std::cout << "Faults  WS: " << ws_mm.get_metrics().page_faults
              << "   LRU: "     << lru_mm.get_metrics().page_faults << "\n";

    std::cout << "\nBoth policies fault identically — frames are never scarce.\n";
    std::cout << "After the phase change the working set model trims the cold\n";
    std::cout << "phase-A pages even though free frames remain; LRU holds all\n";
    std::cout << "16 pages in memory until eviction pressure forces a choice.\n";
    print_exp_footer();
    return 0;
}
