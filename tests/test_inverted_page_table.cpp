#include <cassert>
#include <iostream>
#include "../src/page_table_inverted.h"
#include "../src/memory_manager.h"
#include "../src/policy/fifo.h"
#include "../src/constants.h"

void test_insert_lookup_per_pid() {
    InvertedPageTable ipt(8);
    ipt.insert(1, 5, 0);
    ipt.insert(2, 5, 1); // same vpn, different pid

    assert(ipt.lookup(1, 5)->frame_index == 0);
    assert(ipt.lookup(2, 5)->frame_index == 1);
    assert(ipt.lookup(3, 5) == nullptr);

    assert(ipt.entry_at(0).pid == 1);
    assert(ipt.entry_at(1).pid == 2);

    std::cout << "PASS: test_insert_lookup_per_pid\n";
}

void test_insert_replaces_previous_occupant() {
    InvertedPageTable ipt(8);
    ipt.insert(1, 5, 0);
    ipt.insert(2, 9, 0); // frame 0 reassigned

    assert(ipt.lookup(1, 5) == nullptr);
    assert(ipt.lookup(2, 9)->frame_index == 0);

    std::cout << "PASS: test_insert_replaces_previous_occupant\n";
}

void test_invalidate_clears_index() {
    InvertedPageTable ipt(8);
    ipt.insert(1, 5, 3);
    ipt.invalidate(1, 5);

    assert(ipt.lookup(1, 5) == nullptr);
    assert(ipt.entry_at(3).entry.valid == false);
    assert(ipt.translate(1, 5 * PAGE_SIZE) == -2);

    std::cout << "PASS: test_invalidate_clears_index\n";
}

void test_translate() {
    InvertedPageTable ipt(8);
    assert(ipt.translate(1, -1) == -1);
    assert(ipt.translate(1, VIRTUAL_ADDRESS_SPACE_SIZE) == -1);
    assert(ipt.translate(1, 5 * PAGE_SIZE) == -2);

    ipt.insert(1, 5, 3);
    assert(ipt.translate(1, 5 * PAGE_SIZE + 17) == 3 * (int)PAGE_SIZE + 17);
    assert(ipt.translate(2, 5 * PAGE_SIZE) == -2); // other pid sees nothing

    std::cout << "PASS: test_translate\n";
}

void test_linear_lookup_probe_cost() {
    InvertedPageTable ipt(64);
    ipt.insert(1, 5, 63); // last frame

    ipt.reset_probe_stats();
    assert(ipt.linear_lookup(1, 5)->frame_index == 63);
    assert(ipt.total_linear_probes() == 64); // scanned the whole table

    assert(ipt.lookup(1, 5)->frame_index == 63);
    assert(ipt.total_indexed_probes() == 1); // hash index: one probe

    std::cout << "PASS: test_linear_lookup_probe_cost\n";
}

void test_view_adapter() {
    InvertedPageTable ipt(8);
    InvertedPageTableView view(&ipt, 7);

    view.insert(3, 2);
    assert(view.lookup(3)->frame_index == 2);
    assert(view.translate(3 * PAGE_SIZE) == 2 * (int)PAGE_SIZE);
    assert(view.get_entries().size() == 1);
    assert(view.get_entries().count(3) == 1);

    view.invalidate(3);
    assert(view.lookup(3) == nullptr);

    std::cout << "PASS: test_view_adapter\n";
}

void test_memory_manager_integration() {
    MemoryManager mm(new FIFO(), 2);
    mm.create_process(1, PageTableType::INVERTED);

    mm.access(1, 0 * PAGE_SIZE, false); // fault
    mm.access(1, 1 * PAGE_SIZE, false); // fault
    mm.access(1, 0 * PAGE_SIZE, false); // hit
    mm.access(1, 2 * PAGE_SIZE, false); // fault + eviction

    const Metrics& m = mm.get_metrics();
    assert(m.page_faults == 3);
    assert(m.hits == 1);
    assert(m.evictions == 1);

    const InvertedPageTable* ipt = mm.get_inverted_table();
    assert(ipt != nullptr);
    assert(ipt->frame_count() == 2); // scales with physical memory

    mm.access(1, 0 * PAGE_SIZE, false); // refault after eviction
    assert(mm.get_metrics().page_faults == 4);

    std::cout << "PASS: test_memory_manager_integration\n";
}

void test_two_inverted_processes_share_global_table() {
    MemoryManager mm(new FIFO(), 4);
    mm.create_process(1, PageTableType::INVERTED);
    mm.create_process(2, PageTableType::INVERTED);

    mm.access(1, 0, false);
    mm.access(2, 0, false); // same vpn, own frame

    const InvertedPageTable* ipt = mm.get_inverted_table();
    int owners = 0;
    for (int f = 0; f < ipt->frame_count(); f++)
        if (ipt->entry_at(f).pid != -1 && ipt->entry_at(f).entry.valid) owners++;
    assert(owners == 2);
    assert(mm.get_metrics().page_faults == 2);

    std::cout << "PASS: test_two_inverted_processes_share_global_table\n";
}

void test_fork_rejected() {
    MemoryManager mm(new FIFO(), 4);
    mm.create_process(1, PageTableType::INVERTED);
    mm.access(1, 0, false);

    bool threw = false;
    try {
        mm.fork_process(1, 2);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    std::cout << "PASS: test_fork_rejected\n";
}

int main() {
    test_insert_lookup_per_pid();
    test_insert_replaces_previous_occupant();
    test_invalidate_clears_index();
    test_translate();
    test_linear_lookup_probe_cost();
    test_view_adapter();
    test_memory_manager_integration();
    test_two_inverted_processes_share_global_table();
    test_fork_rejected();

    std::cout << "\nAll InvertedPageTable tests passed.\n";
    return 0;
}
