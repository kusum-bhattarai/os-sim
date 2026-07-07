#include "exp_utils.h"
#include "alloc/buddy_allocator.h"
#include <iostream>
#include <iomanip>
#include <string>

static constexpr int PAGES = 32;

static void print_stage(const std::string& stage, BuddyAllocator& buddy) {
    int probe = buddy.allocate(2);
    if (probe != -1) buddy.free(probe);
    std::cout << std::left  << std::setw(22) << stage
              << std::right << std::setw(6)  << buddy.free_pages()
                            << std::setw(9)  << buddy.largest_free_block()
                            << std::setw(9)  << (probe != -1 ? "ok" : "fails") << "\n";
}

int main() {
    print_exp_header(
        "Experiment N: Buddy Allocator Fragmentation",
        "32-page pool, 1-page churn, then coalescing"
    );

    std::cout << std::left  << std::setw(22) << "Stage"
              << std::right << std::setw(6)  << "Free"
                            << std::setw(9)  << "Largest"
                            << std::setw(9)  << "Alloc(2)" << "\n";
    print_separator('-');

    BuddyAllocator buddy(PAGES);
    print_stage("fresh pool", buddy);

    int bases[PAGES];
    for (int i = 0; i < PAGES; i++)
        bases[i] = buddy.allocate(1);
    print_stage("alloc 32 x 1 page", buddy);

    for (int i = 0; i < PAGES; i += 2)
        buddy.free(bases[i]);
    print_stage("free even pages", buddy);

    for (int i = 1; i < PAGES; i += 2)
        buddy.free(bases[i]);
    print_stage("free odd pages", buddy);

    std::cout << "\nAfter freeing every other page, half the pool is free but\n";
    std::cout << "no two free pages are adjacent — a 2-page request fails.\n";
    std::cout << "Freeing the odd pages lets every block merge with its buddy,\n";
    std::cout << "cascading back to one 32-page block.\n";
    print_exp_footer();
    return 0;
}
