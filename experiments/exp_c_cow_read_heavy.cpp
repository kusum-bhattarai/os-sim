#include "exp_utils.h"
#include "memory_manager.h"
#include "policy/lru.h"

// Parent loads 10 pages, forks a child, then child reads all 10.
// Demonstrates CoW copies are avoided when the child only reads.

int main() {
    print_exp_header(
        "Experiment C: CoW Read-Heavy",
        "Parent loads 10 pages, child reads all 10 (no writes)"
    );

    MemoryManager mm(new LRU(), 20);
    mm.create_process(1);

    for (int page = 0; page < 10; ++page)
        mm.access(1, page * PAGE_SIZE, false);

    mm.fork_process(1, 2);
    mm.reset_metrics();

    for (int page = 0; page < 10; ++page)
        mm.access(2, page * PAGE_SIZE, false);

    print_cow_metrics(mm.get_metrics());
    print_exp_footer();
    return 0;
}
