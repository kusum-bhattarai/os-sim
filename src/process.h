#pragma once
#include "page_table.h"

class Process{
private:
    PageTable page_table;
    int pid;  //process ID

public:
    Process(int pid): pid(pid){}
    int get_pid() const {return pid;}
    PageTable& get_page_table() {return page_table;}
};