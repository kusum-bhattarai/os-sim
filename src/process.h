#pragma once
#include <memory>
#include "page_table.h"
#include "tlb.h"

class Process {
private:
    std::unique_ptr<IPageTable> pt;
    TLB tlb;
    int pid;

public:
    Process(int pid, size_t tlb_size = TLB_SIZE)
        : pid(pid), tlb(tlb_size), pt(std::make_unique<FlatPageTable>()) {}
    int get_pid() const { return pid; }
    IPageTable& get_page_table() { return *pt; }
    const IPageTable& get_page_table() const { return *pt; }
    TLB& get_tlb() { return tlb; }
    const TLB& get_tlb() const { return tlb; }
};
