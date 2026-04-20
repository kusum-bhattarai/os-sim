#include "exp_utils.h"
#include "memory_manager.h"
#include "policy/lru.h"

// Compares the cost of writes before and after a fork.
// Pre-fork: writes go straight to the page (no CoW overhead).
// Post-fork: first write to each page triggers a copy.

int main() {
    print_exp_header(
        "Experiment E: Write Timing — CoW Copy Cost",
        "Writes before fork vs. first writes after fork"
    );
    print_phase_table_header();

    MemoryManager mm(new LRU(), 20);
    mm.create_process(1);

    // Phase 1: pre-fork writes (no CoW involved)
    for (int page = 0; page < 10; ++page)
        mm.access(1, page * PAGE_SIZE, true);
    print_phase_row("Pre-fork writes", mm.get_metrics());

    mm.fork_process(1, 2);
    mm.reset_metrics();

    // Phase 2: post-fork writes on child (each triggers a CoW copy)
    for (int page = 0; page < 10; ++page)
        mm.access(2, page * PAGE_SIZE, true);
    print_phase_row("Post-fork writes", mm.get_metrics());

    mm.reset_metrics();

    // Phase 3: parent re-writes after child has copied all pages
    // (child's copies restored parent pages to writable=true, so no CoW)
    for (int page = 0; page < 10; ++page)
        mm.access(1, page * PAGE_SIZE, true);
    print_phase_row("Parent re-writes", mm.get_metrics());

    print_exp_footer();
    return 0;
}
