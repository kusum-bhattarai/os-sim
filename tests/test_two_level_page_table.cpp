#include <cassert>
#include <iostream>
#include "../src/page_table.h"
#include "../src/page_table_two_level.h"
#include "../src/constants.h"

// VPN 0 → l1_index=0, VPN 32 → l1_index=1 (L2_BITS=5, so boundary is at multiples of 32)

void test_insert_lookup_different_l1_slots_no_alias() {
    TwoLevelPageTable pt;
    pt.insert(0,  10); // l1 slot 0
    pt.insert(32, 20); // l1 slot 1

    PageTableEntry* e0 = pt.lookup(0);
    PageTableEntry* e1 = pt.lookup(32);

    assert(e0 != nullptr && e0->frame_index == 10);
    assert(e1 != nullptr && e1->frame_index == 20);
    assert(e0 != e1); // different memory locations — no aliasing

    std::cout << "PASS: test_insert_lookup_different_l1_slots_no_alias\n";
}

void test_two_vpns_same_l1_slot_share_one_l2_table() {
    TwoLevelPageTable pt;
    pt.insert(0, 5); // l1 slot 0, l2 index 0
    pt.insert(1, 6); // l1 slot 0, l2 index 1

    assert(pt.l2_table_count() == 1); // only one L2 table allocated

    PageTableEntry* e0 = pt.lookup(0);
    PageTableEntry* e1 = pt.lookup(1);
    assert(e0 != nullptr && e0->frame_index == 5);
    assert(e1 != nullptr && e1->frame_index == 6);

    std::cout << "PASS: test_two_vpns_same_l1_slot_share_one_l2_table\n";
}

void test_invalidate_keeps_l2_table_allocated() {
    TwoLevelPageTable pt;
    pt.insert(0, 3);
    pt.invalidate(0);

    assert(pt.l1_allocated(0) == true); // L2 table still exists
    PageTableEntry* e = pt.lookup(0);
    assert(e != nullptr);        // entry still present
    assert(e->valid == false);   // but marked invalid

    std::cout << "PASS: test_invalidate_keeps_l2_table_allocated\n";
}

void test_translate_unallocated_l1_slot_returns_page_fault() {
    TwoLevelPageTable pt;
    // VPN 5 (l1 slot 0) — nothing inserted
    int result = pt.translate(5 * PAGE_SIZE);
    assert(result == -2); // page fault, not -1 (invalid address)

    // Also verify a completely untouched L1 slot
    result = pt.translate(32 * PAGE_SIZE); // l1 slot 1
    assert(result == -2);

    std::cout << "PASS: test_translate_unallocated_l1_slot_returns_page_fault\n";
}

void test_get_entries_matches_flat_page_table() {
    FlatPageTable flat;
    TwoLevelPageTable two;

    // Insert across different L1 slots to exercise the tree structure
    for (int vpn : {0, 1, 31, 32, 63, 512}) {
        flat.insert(vpn, vpn + 100);
        two.insert(vpn, vpn + 100);
    }

    auto flat_entries = flat.get_entries();
    auto two_entries  = two.get_entries();

    assert(flat_entries.size() == two_entries.size());
    for (const auto& [vpn, fe] : flat_entries) {
        assert(two_entries.count(vpn) == 1);
        const PageTableEntry& te = two_entries.at(vpn);
        assert(te.frame_index == fe.frame_index);
        assert(te.valid       == fe.valid);
        assert(te.dirty       == fe.dirty);
        assert(te.writable    == fe.writable);
    }

    std::cout << "PASS: test_get_entries_matches_flat_page_table\n";
}

int main() {
    test_insert_lookup_different_l1_slots_no_alias();
    test_two_vpns_same_l1_slot_share_one_l2_table();
    test_invalidate_keeps_l2_table_allocated();
    test_translate_unallocated_l1_slot_returns_page_fault();
    test_get_entries_matches_flat_page_table();

    std::cout << "\nAll TwoLevelPageTable tests passed.\n";
    return 0;
}
