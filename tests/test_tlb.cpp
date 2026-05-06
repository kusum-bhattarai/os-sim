#include <cassert>
#include <iostream>
#include "../src/tlb.h"

// ─── lookup ──────────────────────────────────────────────────────────

void test_lookup_empty() {
    TLB tlb;
    assert(tlb.lookup(0)  == -1);
    assert(tlb.lookup(42) == -1);
    std::cout << "PASS: test_lookup_empty\n";
}

void test_insert_then_hit() {
    TLB tlb;
    tlb.insert(5, 12, true);
    assert(tlb.lookup(5) == 12);
    std::cout << "PASS: test_insert_then_hit\n";
}

void test_miss_on_absent_vpn() {
    TLB tlb;
    tlb.insert(3, 7, false);
    assert(tlb.lookup(4) == -1); // different VPN, not inserted
    std::cout << "PASS: test_miss_on_absent_vpn\n";
}

// ─── insert / update ─────────────────────────────────────────────────

void test_update_existing_entry() {
    TLB tlb;
    tlb.insert(5, 10, false);
    assert(tlb.lookup(5) == 10);
    tlb.insert(5, 20, true);    // same VPN, new frame
    assert(tlb.lookup(5) == 20);
    std::cout << "PASS: test_update_existing_entry\n";
}

void test_multiple_distinct_entries() {
    TLB tlb;
    for (int i = 0; i < 4; i++) tlb.insert(i, i * 10, true);
    for (int i = 0; i < 4; i++) assert(tlb.lookup(i) == i * 10);
    std::cout << "PASS: test_multiple_distinct_entries\n";
}

// ─── LRU eviction ────────────────────────────────────────────────────

void test_lru_eviction_evicts_oldest() {
    TLB tlb;
    // fill TLB — vpn 0 is oldest, vpn TLB_SIZE-1 is newest
    for (int i = 0; i < (int)TLB_SIZE; i++) tlb.insert(i, i, true);
    // refresh vpn 0 so it is now the MRU
    assert(tlb.lookup(0) == 0);
    // one more insert — vpn 1 is now LRU and should be evicted
    tlb.insert(TLB_SIZE, 99, true);
    assert(tlb.lookup(1)        == -1); // evicted
    assert(tlb.lookup(TLB_SIZE) == 99); // newly inserted
    assert(tlb.lookup(0)        ==  0); // refreshed, still present
    std::cout << "PASS: test_lru_eviction_evicts_oldest\n";
}

void test_lru_eviction_respects_recency() {
    TLB tlb;
    for (int i = 0; i < (int)TLB_SIZE; i++) tlb.insert(i, i, true);
    // access all entries except vpn 0 — vpn 0 becomes LRU
    for (int i = 1; i < (int)TLB_SIZE; i++) tlb.lookup(i);
    // insert one more — vpn 0 must be evicted
    tlb.insert(TLB_SIZE, 99, true);
    assert(tlb.lookup(0) == -1); // LRU evicted
    std::cout << "PASS: test_lru_eviction_respects_recency\n";
}

// ─── invalidate ──────────────────────────────────────────────────────

void test_invalidate_removes_entry() {
    TLB tlb;
    tlb.insert(3, 7, true);
    tlb.invalidate(3);
    assert(tlb.lookup(3) == -1);
    std::cout << "PASS: test_invalidate_removes_entry\n";
}

void test_invalidate_leaves_others_intact() {
    TLB tlb;
    tlb.insert(3, 7, true);
    tlb.insert(4, 8, true);
    tlb.invalidate(3);
    assert(tlb.lookup(4) == 8);
    std::cout << "PASS: test_invalidate_leaves_others_intact\n";
}

void test_invalidate_absent_vpn_is_safe() {
    TLB tlb;
    tlb.insert(2, 5, false);
    tlb.invalidate(99); // not in TLB — should not crash
    assert(tlb.lookup(2) == 5); // unaffected
    std::cout << "PASS: test_invalidate_absent_vpn_is_safe\n";
}

// ─── flush ───────────────────────────────────────────────────────────

void test_flush_clears_all_entries() {
    TLB tlb;
    tlb.insert(0, 1, false);
    tlb.insert(1, 2, true);
    tlb.insert(2, 3, false);
    tlb.flush();
    assert(tlb.lookup(0) == -1);
    assert(tlb.lookup(1) == -1);
    assert(tlb.lookup(2) == -1);
    std::cout << "PASS: test_flush_clears_all_entries\n";
}

void test_flush_allows_reinsert() {
    TLB tlb;
    for (int i = 0; i < (int)TLB_SIZE; i++) tlb.insert(i, i, true);
    tlb.flush();
    // after flush, TLB is empty so inserting fresh entries should work
    tlb.insert(0, 42, true);
    assert(tlb.lookup(0) == 42);
    std::cout << "PASS: test_flush_allows_reinsert\n";
}

int main() {
    test_lookup_empty();
    test_insert_then_hit();
    test_miss_on_absent_vpn();
    test_update_existing_entry();
    test_multiple_distinct_entries();
    test_lru_eviction_evicts_oldest();
    test_lru_eviction_respects_recency();
    test_invalidate_removes_entry();
    test_invalidate_leaves_others_intact();
    test_invalidate_absent_vpn_is_safe();
    test_flush_clears_all_entries();
    test_flush_allows_reinsert();

    std::cout << "\nAll TLB tests passed.\n";
    return 0;
}
