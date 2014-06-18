#include "lock_lut.h"
#include "system.h"

LockLut::LockLut() {
    table_lock = new Lock("Lock lookup table lock");
    next_available_lock_index = 0;
    allocated_locks = 0;
    name = "Lock lookup table";
}

// In theory this function should only be called when the system is shutting down
// so we don't really care about properly cleaning up, but let's do it anyway.
LockLut::~LockLut() {
    // deallocate all the locks stored in the lookup table

    for(int c = 0; c < MAX_SYSTEM_LOCKS; c++) {
        KernelLock *loop_kernel_lock = lock_lookup_table[c];
        if(loop_kernel_lock != NULL) {
            delete loop_kernel_lock->lock;
            delete loop_kernel_lock;
        }

        delete table_lock;
    }

}

/**
 * Allocate a new lock and store a reference to it in the lookup table.
 * If MAX_SYSTEM_LOCKS has been reached, the request is rejected.
 * 
 * @return int: If allocation is successful, an int representing the
 *              index of the lock. Otherwise -1 to represent an error.
 */
int LockLut::allocate_lock(char *lock_name) {
    int allocated_lock_index = next_available_lock_index;
    int watchdog_counter = 0;

    table_lock->Acquire();

    // check to see if there is space to allocate a new lock
    if(allocated_locks == (MAX_SYSTEM_LOCKS - 1)) {
        return -1;
    }

    // find available index in lookup table
    KernelLock *loop_kernel_lock = lock_lookup_table[next_available_lock_index];

    // this will turn into an infinite loop if allocated_locks isn't maintained properly
    // that is why we have the watchdog counter
    while(loop_kernel_lock != NULL && watchdog_counter != MAX_SYSTEM_LOCKS) {
        next_available_lock_index ++;

        // to ensure we don't go out of bounds on our lookup table
        if(next_available_lock_index == MAX_SYSTEM_LOCKS) {
            next_available_lock_index = 0;
        }

        loop_kernel_lock = lock_lookup_table[next_available_lock_index];
        watchdog_counter ++;
    }

    // we have our next available lock index in the table
    KernelLock *new_kernel_lock = new KernelLock();
    Lock *new_lock = new Lock(lock_name);

    // initialize the KernelLock struct
    new_kernel_lock->lock = new_lock;
    new_kernel_lock->address_space = currentThread->space;
    new_kernel_lock->marked_for_destroy = false;
    new_kernel_lock->in_use = false;

    // store the new_kernel_lock in the lookup table
    lock_lookup_table[next_available_lock_index] = new_kernel_lock;
    allocated_lock_index = next_available_lock_index;

    next_available_lock_index ++;
    allocated_locks ++;

    table_lock->Release();

    return allocated_lock_index;
}

