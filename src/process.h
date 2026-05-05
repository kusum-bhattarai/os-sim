#pragma once
#include "page_table.h"
#include "tlb.h"

class Process{
private:
    PageTable page_table;
    TLB tlb;
    int pid;  //process ID

public:
    Process(int pid): pid(pid){}
    int get_pid() const {return pid;}
    PageTable& get_page_table() {return page_table;}
    TLB& get_tlb() { return tlb; }
};