#ifndef KOLSH_CODE_USSERPROG_PROCESS_TABLE_H_
#define KOLSH_CODE_USSERPROG_PROCESS_TABLE_H_

#include "addrspace.h"
#include "synch.h"

#define MAX_SYSTEM_PROCESSES      10

struct ProcessTableEntry {
    AddrSpace *address_space;
    int process_id;
    bool address_space_bound;
};

class ProcessTable {
public:
    ProcessTable();
    ~ProcessTable();
    int assign_new_process_id();
    bool bind_address_space(int process_id, AddrSpace *address_space);
    bool release_process_id(int process_id);
    int get_number_of_running_processes();

private:
    Lock *process_table_lock;
    ProcessTableEntry *process_table[MAX_SYSTEM_PROCESSES];

    int next_available_process_index;
    int running_proccesses;
};

#endif // KOLSH_CODE_USSERPROG_PROCESS_TABLE_H_