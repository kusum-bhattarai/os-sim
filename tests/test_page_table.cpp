#include <cassert>
#include <iostream>
#include "../src/page_table.h"
#include "../src/constants.h"

void test_insert_and_lookup() {
    FlatPageTable pt;
    pt.insert(0, 5);
    PageTableEntry* entry = pt.lookup(0);
    assert(entry != nullptr);
    assert(entry->valid == true);
    assert(entry->frame_index == 5);
    std::cout << "PASS: test_insert_and_lookup\n";
}

void test_lookup_missing_vpn_returns_nullptr() {
    FlatPageTable pt;
    PageTableEntry* entry = pt.lookup(99);
    assert(entry == nullptr);
    std::cout << "PASS: test_lookup_missing_vpn_returns_nullptr\n";
}

void test_insert_overwrites_existing_entry() {
    FlatPageTable pt;
    pt.insert(0, 5);
    pt.insert(0, 10);
    PageTableEntry* entry = pt.lookup(0);
    assert(entry->frame_index == 10);
    std::cout << "PASS: test_insert_overwrites_existing_entry\n";
}

void test_invalidate_marks_entry_invalid() {
    FlatPageTable pt;
    pt.insert(0, 5);
    pt.invalidate(0);
    PageTableEntry* entry = pt.lookup(0);
    assert(entry != nullptr);        // entry still exists
    assert(entry->valid == false);   // but marked invalid
    std::cout << "PASS: test_invalidate_marks_entry_invalid\n";
}

void test_invalidate_missing_vpn_does_nothing() {
    FlatPageTable pt;
    pt.invalidate(99); // should not throw or crash
    std::cout << "PASS: test_invalidate_missing_vpn_does_nothing\n";
}

void test_translate_valid_address() {
    FlatPageTable pt;
    pt.insert(2, 3); // vpn 2 → frame 3
    // virtual address = vpn * PAGE_SIZE + offset
    int virtual_addr = 2 * PAGE_SIZE + 7;
    int physical_addr = pt.translate(virtual_addr);
    int expected = 3 * PAGE_SIZE + 7;
    assert(physical_addr == expected);
    std::cout << "PASS: test_translate_valid_address\n";
}

void test_translate_offset_arithmetic() {
    FlatPageTable pt;
    pt.insert(0, 0);
    // offset 0 → first byte of frame
    assert(pt.translate(0) == 0);
    // offset PAGE_SIZE - 1 → last byte of frame 0
    assert(pt.translate(PAGE_SIZE - 1) == PAGE_SIZE - 1);

    pt.insert(1, 4);
    // first byte of vpn 1 → first byte of frame 4
    assert(pt.translate(PAGE_SIZE) == 4 * PAGE_SIZE);
    std::cout << "PASS: test_translate_offset_arithmetic\n";
}

void test_translate_missing_entry_returns_page_fault() {
    FlatPageTable pt;
    int result = pt.translate(PAGE_SIZE * 5);
    assert(result == -2);
    std::cout << "PASS: test_translate_missing_entry_returns_page_fault\n";
}

void test_translate_invalidated_entry_returns_page_fault() {
    FlatPageTable pt;
    pt.insert(1, 2);
    pt.invalidate(1);
    int result = pt.translate(PAGE_SIZE * 1);
    assert(result == -2);
    std::cout << "PASS: test_translate_invalidated_entry_returns_page_fault\n";
}

void test_translate_out_of_bounds_returns_invalid_address() {
    FlatPageTable pt;
    assert(pt.translate(-1) == -1);
    assert(pt.translate(VIRTUAL_ADDRESS_SPACE_SIZE) == -1);
    assert(pt.translate(VIRTUAL_ADDRESS_SPACE_SIZE + 100) == -1);
    std::cout << "PASS: test_translate_out_of_bounds_returns_invalid_address\n";
}

void test_entry_defaults() {
    FlatPageTable pt;
    pt.insert(0, 3);
    PageTableEntry* entry = pt.lookup(0);
    assert(entry->dirty == false);
    assert(entry->referenced == false);
    assert(entry->writable == false);
    std::cout << "PASS: test_entry_defaults\n";
}

int main() {
    test_insert_and_lookup();
    test_lookup_missing_vpn_returns_nullptr();
    test_insert_overwrites_existing_entry();
    test_invalidate_marks_entry_invalid();
    test_invalidate_missing_vpn_does_nothing();
    test_translate_valid_address();
    test_translate_offset_arithmetic();
    test_translate_missing_entry_returns_page_fault();
    test_translate_invalidated_entry_returns_page_fault();
    test_translate_out_of_bounds_returns_invalid_address();
    test_entry_defaults();

    std::cout << "\nAll tests passed.\n";
    return 0;
}