#include <assert.h>
#include "process_table.h"


ProcessTable::ProcessTable() {
    process_table_lock = new Lock("Process table lock");

    next_available_process_index = 0;
    running_proccesses = 0;

    // initialize process table
    for(int i = 0;  i < MAX_SYSTEM_PROCESSES; i++) {
        process_table[i] == NULL;
    }
}

// The system is shutting down at this point, we can afford
// to be sloppy in our cleanup process.
ProcessTable::~ProcessTable() {

    for(int c = 0; c < MAX_SYSTEM_PROCESSES; c++) {
        ProcessTableEntry *loop_process_table_entry = process_table[c];
        if(loop_process_table_entry != NULL) {
            delete loop_process_table_entry;
        }
    }

    delete process_table_lock;
}

/**
 * Assign a new process id for a pending process. On return of this
 * function, the process id has only been reserved for the process.
 * Before the table entry can be used, an address space must be bound
 * to the process entry using 
 * ProcessTable::bind_address_space(int process_id, AddrSpace *address_space)
 *
 * @return int  >= 0    new process id
                -1      max system process reached
 */
int ProcessTable::assign_new_process_id() {
    int new_process_id = -1;
    int watchdog_counter = 0;

    process_table_lock->Acquire();

    if(running_proccesses == MAX_SYSTEM_PROCESSES) {
        // we have reached the system limit for max processes
        process_table_lock->Release();
        return -1;
    }

    ProcessTableEntry *loop_process_table_entry = process_table[next_available_process_index];
    while(loop_process_table_entry != NULL && watchdog_counter != MAX_SYSTEM_PROCESSES) {
        next_available_process_index++;

        // to ensure we don't go out of bounds on our process table
        if(next_available_process_index == MAX_SYSTEM_PROCESSES) {
            next_available_process_index = 0;
        }

        loop_process_table_entry = process_table[next_available_process_index];
        watchdog_counter++;
    }

    if(watchdog_counter == MAX_SYSTEM_PROCESSES) {
        // something has gone horribly wrong
        // in theory, this should never execute.
        DEBUG('e', "Quit loop on watchdog_counter ProcessTable::assign_new_process_id");
        process_table_lock->Release();
        assert(false);
    }

    new_process_id = next_available_process_index;
    
    // create a new process table entry and store it in the table
    ProcessTableEntry *new_process_table_entry = new ProcessTableEntry();
    new_process_table_entry->process_id = new_process_id;
    new_process_table_entry->address_space_bound = FALSE;
    process_table[new_process_id] = new_process_table_entry;

    // update process_table bookkeeping data
    running_proccesses++;
    process_table_lock->Release();

    return new_process_id;
}

/**
 * Bind an address space with a running process. This function MUST be called
 * in conjunction with EVERY assign_new_process_id() request. The process table
 * will be corrupted if this is not done properly.
 *
 * @param process_id    The process id to which the address space should be bound
 * @param address_space The address space to bind to the given process id
 *
 * @return bool     TRUE if the address space was bound successfully/
 *                  FALSE if there was an error.
 */
bool ProcessTable::bind_address_space(int process_id, AddrSpace *address_space) {
    if(process_id < 0 || process_id >= MAX_SYSTEM_PROCESSES) {
        // invalid process id
        printf("@ProcessTable::bind_address_space\nInvalid process id [%d]\n", process_id);
        return false;
    }

    if(address_space == NULL) {
        printf("@ProcessTable::bind_address_space\nAddress space to bind cannot be null.\n");
        return false;
    }

    ProcessTableEntry *process_table_entry = process_table[process_id];
    if(process_table_entry->address_space_bound) {
        printf("@ProcessTable::bind_address_space\nAn address space cannot be re-bound with a running process.\n");
        return false;
    }

    process_table_entry->address_space = address_space;
    process_table_entry->address_space_bound = TRUE;

    return true;
}

/**
 * Get the current number of running process
 * 
 * @return int The current number of running process.
 */
int ProcessTable::get_number_of_running_processes() {
    int current_number_of_running_process = 0;
    
    process_table_lock->Acquire();
    current_number_of_running_process = running_proccesses;
    process_table_lock->Release();
    
    return current_number_of_running_process;
}

/**
 * Release a process from the process table. This frees all resources associated
 * with the process including the address space object.
 *
 * @param process_id The process id to be release
 *
 * return bool  TRUE if the release operation was successful
 *              FALSE on error
 */
bool ProcessTable::release_process_id(int process_id) {
    if(process_id < 0 || process_id >= MAX_SYSTEM_PROCESSES) {
        // invalid process id
        printf("@ProcessTable::release_process_id\nInvalid process id [%d] supplied.\n", process_id);
        return false;
    }

    process_table_lock->Acquire();
    // TODO: is the process table responsible for freeing the address space object?
    // at the moment we will say yes
    ProcessTableEntry *process_entry_to_be_deleted = process_table[process_id];
    if(process_entry_to_be_deleted->address_space_bound) {
        delete process_entry_to_be_deleted->address_space;
    }
    delete process_entry_to_be_deleted;
    process_table[process_id] = NULL;

    running_proccesses--;
    process_table_lock->Release();

    return true;
}

