/**
 * ProcessTable
 * 
 * Data structure to keep track of running system processes.
 */

#ifndef KOLSH_CODE_USSERPROG_PROCESS_TABLE_H_
#define KOLSH_CODE_USSERPROG_PROCESS_TABLE_H_

#include "addrspace.h"
#include "synch.h"

#define MAX_SYSTEM_PROCESSES      10

struct ProcessTableEntry {
    AddrSpace *address_space;
    unsigned int process_id;
    bool address_space_bound;
};

class ProcessTable {
public:
    ProcessTable();
    ~ProcessTable();

    int acquire_new_process_id();
    bool bind_address_space(const unsigned int process_id, AddrSpace *address_space);
    unsigned int get_number_of_running_processes();
    bool release_process_id(const unsigned int process_id);

private:
    Lock *process_table_lock;
    ProcessTableEntry *process_table[MAX_SYSTEM_PROCESSES];

    unsigned int next_available_process_id;
    unsigned int number_of_running_processes;
};

#endif // KOLSH_CODE_USSERPROG_PROCESS_TABLE_H_
