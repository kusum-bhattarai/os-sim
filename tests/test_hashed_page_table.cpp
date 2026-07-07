#include <cassert>
#include <iostream>
#include "../src/page_table.h"
#include "../src/page_table_hashed.h"
#include "../src/constants.h"

// bucket_index = vpn % 64, so VPNs 0, 64, 128 all collide in bucket 0

void test_insert_lookup_basic() {
    HashedPageTable pt;
    pt.insert(3, 7);

    PageTableEntry* e = pt.lookup(3);
    assert(e != nullptr);
    assert(e->valid == true);
    assert(e->frame_index == 7);
    assert(pt.lookup(4) == nullptr);

    std::cout << "PASS: test_insert_lookup_basic\n";
}

void test_colliding_vpns_chain_in_one_bucket() {
    HashedPageTable pt;
    pt.insert(0,   10);
    pt.insert(64,  20);
    pt.insert(128, 30);

    assert(pt.chain_length(0) == 3);
    assert(pt.max_chain_length() == 3);

    assert(pt.lookup(0)->frame_index   == 10);
    assert(pt.lookup(64)->frame_index  == 20);
    assert(pt.lookup(128)->frame_index == 30);

    std::cout << "PASS: test_colliding_vpns_chain_in_one_bucket\n";
}

void test_reinsert_overwrites_without_growing_chain() {
    HashedPageTable pt;
    pt.insert(5, 1);
    pt.insert(5, 2);

    assert(pt.chain_length(5) == 1);
    assert(pt.lookup(5)->frame_index == 2);

    std::cout << "PASS: test_reinsert_overwrites_without_growing_chain\n";
}

void test_invalidate_keeps_node_marks_invalid() {
    HashedPageTable pt;
    pt.insert(9, 4);
    pt.invalidate(9);

    PageTableEntry* e = pt.lookup(9);
    assert(e != nullptr);
    assert(e->valid == false);
    assert(pt.chain_length(9) == 1);

    std::cout << "PASS: test_invalidate_keeps_node_marks_invalid\n";
}

void test_translate_fault_codes() {
    HashedPageTable pt;
    assert(pt.translate(-1) == -1);
    assert(pt.translate(VIRTUAL_ADDRESS_SPACE_SIZE) == -1);
    assert(pt.translate(5 * PAGE_SIZE) == -2);

    pt.insert(5, 3);
    assert(pt.translate(5 * PAGE_SIZE + 17) == 3 * (int)PAGE_SIZE + 17);

    pt.invalidate(5);
    assert(pt.translate(5 * PAGE_SIZE) == -2);

    std::cout << "PASS: test_translate_fault_codes\n";
}

void test_probe_counting() {
    HashedPageTable pt;
    pt.insert(0,   1);
    pt.insert(64,  2);
    pt.insert(128, 3);
    pt.reset_probe_stats();

    // chain is front-inserted: 128 is first, 0 is last
    pt.lookup(128);
    assert(pt.total_probes() == 1);
    pt.lookup(0);
    assert(pt.total_probes() == 4);
    assert(pt.total_lookups() == 2);

    std::cout << "PASS: test_probe_counting\n";
}

void test_get_entries_matches_flat_page_table() {
    FlatPageTable flat;
    HashedPageTable hashed;

    for (int vpn : {0, 1, 63, 64, 128, 500}) {
        flat.insert(vpn, vpn + 100);
        hashed.insert(vpn, vpn + 100);
    }

    auto flat_entries   = flat.get_entries();
    auto hashed_entries = hashed.get_entries();

    assert(flat_entries.size() == hashed_entries.size());
    for (const auto& [vpn, fe] : flat_entries) {
        assert(hashed_entries.count(vpn) == 1);
        const PageTableEntry& he = hashed_entries.at(vpn);
        assert(he.frame_index == fe.frame_index);
        assert(he.valid       == fe.valid);
        assert(he.dirty       == fe.dirty);
        assert(he.writable    == fe.writable);
    }

    std::cout << "PASS: test_get_entries_matches_flat_page_table\n";
}

int main() {
    test_insert_lookup_basic();
    test_colliding_vpns_chain_in_one_bucket();
    test_reinsert_overwrites_without_growing_chain();
    test_invalidate_keeps_node_marks_invalid();
    test_translate_fault_codes();
    test_probe_counting();
    test_get_entries_matches_flat_page_table();

    std::cout << "\nAll HashedPageTable tests passed.\n";
    return 0;
}
