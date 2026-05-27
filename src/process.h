#pragma once
#include <memory>
#include "page_table.h"
#include "page_table_two_level.h"
#include "tlb.h"

class Process {
private:
    std::unique_ptr<IPageTable> pt;
    TLB tlb;
    int pid;
    PageTableType pt_type;

public:
    Process(int pid, size_t tlb_size = TLB_SIZE, PageTableType pt_type = PageTableType::FLAT)
        : pid(pid), tlb(tlb_size), pt_type(pt_type) {
        if (pt_type == PageTableType::TWO_LEVEL)
            pt = std::make_unique<TwoLevelPageTable>();
        else
            pt = std::make_unique<FlatPageTable>();
    }
    int get_pid() const { return pid; }
    PageTableType get_pt_type() const { return pt_type; }
    IPageTable& get_page_table() { return *pt; }
    const IPageTable& get_page_table() const { return *pt; }
    TLB& get_tlb() { return tlb; }
    const TLB& get_tlb() const { return tlb; }
};
