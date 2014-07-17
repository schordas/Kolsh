#include <assert.h>
#include "process_table.h"


ProcessTable::ProcessTable() {
    process_table_lock = new Lock("Process table lock");

    next_available_process_id = 0;
    number_of_running_processes = 0;

    // initialize process table
    for(unsigned int i = 0;  i < MAX_SYSTEM_PROCESSES; i++) {
        process_table[i] == NULL;
    }
}

// The system is shutting down at this point, we can afford
// to be sloppy in our cleanup process.
ProcessTable::~ProcessTable() {

    for(unsigned int c = 0; c < MAX_SYSTEM_PROCESSES; c++) {
        ProcessTableEntry *loop_process_table_entry = process_table[c];
        if(loop_process_table_entry != NULL) {
            delete loop_process_table_entry;
        }
    }

    delete process_table_lock;
}

/**
 * Acquire a new process id for a pending process. On return of this
 * function, the process id has only been reserved for the process.
 * Before the table entry can be used, an address space must be bound
 * to the process entry using 
 * ProcessTable::bind_address_space(int process_id, AddrSpace *address_space)
 *
 * @return int  >= 0    new process id
 *              -1      max system process reached
 */
int ProcessTable::acquire_new_process_id() {
    unsigned int next_available_table_index = 0;

    process_table_lock->Acquire();
    if(number_of_running_processes == MAX_SYSTEM_PROCESSES) {
        // we have reached the system limit for max processes
        printf("MAX SYSTEM PROCESSES REACHED\n");
        process_table_lock->Release();
        return -1;
    }


    ProcessTableEntry *new_process_table_entry = process_table[next_available_table_index];
    while(new_process_table_entry != NULL && next_available_table_index != MAX_SYSTEM_PROCESSES) {
        next_available_table_index++;
        new_process_table_entry = process_table[next_available_table_index];
    }

    if(next_available_table_index == MAX_SYSTEM_PROCESSES) {
        // we have reached the system limit for max processes
        // this should never execute since we've already checked
        // to see if there is space for a new process.
        process_table_lock->Release();
        ASSERT(FALSE);
        return -1;
    }

    new_process_table_entry = new ProcessTableEntry();
    new_process_table_entry->address_space = NULL;
    new_process_table_entry->address_space_bound = FALSE;
    new_process_table_entry->process_id = next_available_process_id;
    process_table[next_available_table_index] = new_process_table_entry;
    
    number_of_running_processes++;
    next_available_process_id++;

    process_table_lock->Release();
    return new_process_table_entry->process_id;
}

/**
 * Bind an address space with a running process. This function MUST be called
 * in conjunction with EVERY assign_new_process_id() request. The process table
 * will be corrupted if this is not done properly.
 *
 * @param process_id    The process id to which the address space should be bound
 * @param address_space The address space to bind to the given process id
 *
 * @return bool     TRUE if the address space was bound successfully
 *                  FALSE if there was an error.
 */
bool ProcessTable::bind_address_space(const unsigned int process_id, AddrSpace *address_space) {
    if(process_id < 0) {
        // invalid process id
        printf("@ProcessTable::bind_address_space\nInvalid process id [%d]\n", process_id);
        return false;
    }

    if(address_space == NULL) {
        printf("@ProcessTable::bind_address_space\nAddress space to bind cannot be null.\n");
        return false;
    }

    // find the ProcessEntry this process id belongs to
    unsigned int process_table_index = 0;
    process_table_lock->Acquire();
    ProcessTableEntry *process_table_entry = process_table[process_table_index];
    while(process_table_entry->process_id != process_id && process_table_index != MAX_SYSTEM_PROCESSES) {
        process_table_index++;
        process_table_entry = process_table[process_table_index];
    }

    if(process_table_index == MAX_SYSTEM_PROCESSES) {
        // invalid process id
        printf("@ProcessTable::bind_address_space\nInvalid process id [%d]\n", process_id);
        process_table_lock->Release();
        return false;
    }

    if(process_table_entry->address_space_bound ) {
        printf("@ProcessTable::bind_address_space\nAn address space cannot be re-bound with a running process.\n");
        process_table_lock->Release();
        return false;
    }

    process_table_entry->address_space = address_space;
    process_table_entry->address_space_bound = TRUE;
    process_table_lock->Release();

    return true;
}

/**
 * Get the current number of running process
 * 
 * @return int The current number of running process.
 */
unsigned int ProcessTable::get_number_of_running_processes() {
    unsigned int return_value = 0;
    
    process_table_lock->Acquire();
    return_value = number_of_running_processes;
    process_table_lock->Release();
    
    return return_value;
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
bool ProcessTable::release_process_id(const unsigned int process_id) {
    // find the ProcessEntry this process id belongs to
    unsigned int process_table_index = 0;
    
    process_table_lock->Acquire();
    ProcessTableEntry *process_table_entry = process_table[process_table_index];
    while(process_table_entry->process_id != process_id && process_table_index != MAX_SYSTEM_PROCESSES) {
        process_table_index++;
        process_table_entry = process_table[process_table_index];
    }

    if(process_table_index == MAX_SYSTEM_PROCESSES) {
        // invalid process id
        printf("@ProcessTable::release_process_id\nInvalid process id [%d] supplied.\n", process_id);
        process_table_lock->Release();
        return false;
    }

    delete process_table_entry;
    number_of_running_processes--;

    process_table_lock->Release();
    return true;
}
