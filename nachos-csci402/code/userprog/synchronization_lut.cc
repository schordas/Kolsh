#include <assert.h>
#include "synchronization_lut.h"
#include "system.h"


SynchronizationLut::SynchronizationLut() {
    lock_table_mutex = new Lock("Lock table mutex");
    lock_entry_mutex = new Lock("Lock entry mutex");
    condition_table_mutex = new Lock("Condition table mutex");
    condition_entry_mutex = new Lock("Condition entry mutex");

    next_available_lock_index = 0;
    next_available_condition_index = 0;
    allocated_locks = 0;
    allocated_conditions = 0;
    name = "Synchronization lookup table";
}


// In theory this function should only be called when the system is shutting down
// so we don't really care about properly cleaning up, but let's do it anyway.
SynchronizationLut::~SynchronizationLut() {
    
    // delete all the locks stored in the lock lookup table
    for(int c = 0; c < MAX_SYSTEM_LOCKS; c++) {
        KernelLock *loop_kernel_lock = lock_lookup_table[c];
        if(loop_kernel_lock != NULL) {
            delete loop_kernel_lock->lock;
            delete loop_kernel_lock;
        }
    }

    // delete all the conditions stored in the condition lookup table
    for(int c = 0; c < MAX_SYSTEM_CONDITIONS; c++) {
        KernelCondition *loop_kernel_condition = condition_lookup_table[c];
        if(loop_kernel_condition != NULL) {
            delete loop_kernel_condition->condition;
            delete loop_kernel_condition;
        }
    }

    delete lock_table_mutex;
    delete lock_entry_mutex;
    delete condition_table_mutex;
    delete condition_entry_mutex;
}


/**
 * Allocate a new lock and store a reference to it in the lookup table.
 * If MAX_SYSTEM_LOCKS has been reached, the request is rejected.
 * 
 * @return int: If allocation is successful, an int representing the
 *              index of the lock. Otherwise -1 to represent an error.
 */
int SynchronizationLut::lock_create(char *lock_name) {
    int allocated_lock_index = -1;
    int watchdog_counter = 0;

    lock_table_mutex->Acquire();

    // check to see if there is space to allocate a new lock
    if(allocated_locks == (MAX_SYSTEM_LOCKS - 1)) {
        lock_table_mutex->Release();
        return -1;
    }

    // find available index in lookup table
    KernelLock *loop_kernel_lock = lock_lookup_table[next_available_lock_index];

    // this will turn into an infinite loop if allocated_locks isn't maintained properly
    // the watchdog counter will prevent an infinite and fail with -1.
    while(loop_kernel_lock != NULL && watchdog_counter != MAX_SYSTEM_LOCKS) {
        next_available_lock_index++;

        // to ensure we don't go out of bounds on our lookup table
        if(next_available_lock_index == MAX_SYSTEM_LOCKS) {
            next_available_lock_index = 0;
        }

        loop_kernel_lock = lock_lookup_table[next_available_lock_index];
        watchdog_counter++;
    }

    if(watchdog_counter == MAX_SYSTEM_LOCKS) {
        // something has gone horribly wrong
        // in theory, this should never execute.
        DEBUG('e', "Quit loop on watchdog_counter LockLut::lock_create");
        lock_table_mutex->Release();
        assert(false);
    }

    // we have our next available lock index in the table
    KernelLock *new_kernel_lock = new KernelLock();
    Lock *new_lock = new Lock(lock_name);

    // initialize the KernelLock struct
    new_kernel_lock->lock = new_lock;
    new_kernel_lock->address_space = currentThread->space;
    new_kernel_lock->marked_for_destroy = false;
    new_kernel_lock->in_use_count = 0;

    // store the new_kernel_lock in the lookup table
    lock_lookup_table[next_available_lock_index] = new_kernel_lock;
    allocated_lock_index = next_available_lock_index;

    allocated_locks ++;

    lock_table_mutex->Release();
    return allocated_lock_index;
}


int SynchronizationLut::lock_acquire(int lock_index){
    if(!validate_lock_request(lock_index)) {
        return -1;
    }
    
    KernelLock *kernel_lock = lock_lookup_table[lock_index];

    kernel_lock->lock->Acquire();
    kernel_lock->in_use_count++;        // this statement must come after 
                                        // kernel_lock->lock->Acquire(); to
                                        // ensure only one thread can alter
                                        // this value at a time.
    return 0;
}


int SynchronizationLut::lock_release(int lock_index) {
    if(!validate_lock_request(lock_index)) {
        return -1;
    }
    
    KernelLock *kernel_lock = lock_lookup_table[lock_index];

    // we need to ensure that the lock is actually released before
    // decrementing the in_use_count value. (consider the case if
    // the current thread isn't the lock owner, if we didn't have this
    // check, the in_use_count field could get corrupted).
    if(kernel_lock->lock->Release()) {
        lock_entry_mutex->Acquire();
        kernel_lock->in_use_count--;
        if(kernel_lock->marked_for_destroy){
            attempt_lock_delete(lock_index);
        }
        lock_entry_mutex->Release();
    }

    return 0;
}


/**
 * De-allocate a lock. If the lock is currently in use,
 * the lock is marked for deletion at a future date.
 *
 * @return int: -1 Invalid request, index out of bounds or permission denied.
 *              0  The lock has been deleted or marked for deletion. No other guarantees are made.
 */
int SynchronizationLut::lock_delete(int lock_index) {
    if(!validate_lock_request(lock_index)) {
        return -1;
    }

    KernelLock *kernel_lock = lock_lookup_table[lock_index];
    
    kernel_lock->marked_for_destroy = true;
    attempt_lock_delete(lock_index);

    return 0;
}


/**
 * Allocate a new condition and store a reference to it in the lookup table.
 * If MAX_SYSTEM_CONDITIONS has been reached, the request is rejected.
 * 
 * @return int: If allocation is successful, an int representing the
 *              index of the lock. Otherwise -1 to represent an error.
 */
int SynchronizationLut::condition_create(char* condition_name) {
    int allocated_condition_index = next_available_condition_index;
    int watchdog_counter = 0;

    condition_table_mutex->Acquire();

    // check to see if there is space to allocate a new condition
    if(allocated_conditions == (MAX_SYSTEM_CONDITIONS - 1)) {
        condition_table_mutex->Release();
        return -1;
    }

    // find available index in lookup table
    KernelCondition *loop_kernel_condition = condition_lookup_table[next_available_condition_index];

    // this will turn into an infinite loop if allocated_conditions isn't maintained properly
    // the watchdog counter will prevent an infinite and fail with -1.
    while(loop_kernel_condition != NULL && watchdog_counter != MAX_SYSTEM_CONDITIONS) {
        next_available_condition_index ++;

        // to ensure we don't go out of bounds on our lookup table
        if(next_available_condition_index == MAX_SYSTEM_CONDITIONS) {
            next_available_condition_index = 0;
        }

        loop_kernel_condition = condition_lookup_table[next_available_condition_index];
        watchdog_counter ++;
    }

    if(watchdog_counter == MAX_SYSTEM_CONDITIONS) {
        // something has gone horribly wrong
        // in theory, this should never execute.
        DEBUG('e', "Quit loop on watchdog_counter LockLut::condition_create");
        condition_table_mutex->Release();
        assert(false);
    }

    // we have our next available condition index
    KernelCondition *new_kernel_condition = new KernelCondition();
    Condition *new_condition = new Condition(condition_name);

    // initialize the KernelCondition struct
    new_kernel_condition->condition = new_condition;
    new_kernel_condition->address_space = currentThread->space;
    new_kernel_condition->marked_for_destroy = false;
    new_kernel_condition->in_use_count = 0;

    // store the new_kernel_condition in the lookup table
    condition_lookup_table[next_available_condition_index] = new_kernel_condition;
    allocated_condition_index = next_available_condition_index;
    
    allocated_conditions ++;

    condition_table_mutex->Release();
    return allocated_condition_index;
}


/**
 * Wait on a condition. The condition and lock must be within the calling
 * address space.
 *
 * @param condition_index - index of condition to wait on
 * @param lock_index - index of lock to use in the condition
 *
 * @return int - 0 on success -1 otherwise
 */
int SynchronizationLut::condition_wait(int condition_index, int lock_index) {
    if(!validate_condition_request(condition_index) || !validate_lock_request(lock_index)) {
        return -1;
    }

    KernelCondition *kernel_condition = condition_lookup_table[condition_index];
    KernelLock *kernel_lock = lock_lookup_table[lock_index];

    // bookkeeping to ensure locks aren't deleted in use
    condition_entry_mutex->Acquire();
    lock_entry_mutex->Acquire();
    kernel_condition->in_use_count ++;
    kernel_lock->in_use_count ++;
    lock_entry_mutex->Release();
    condition_entry_mutex->Release();
    
    kernel_condition->condition->Wait(kernel_lock->lock);
    
    condition_entry_mutex->Acquire();
    lock_entry_mutex->Acquire();
    kernel_condition->in_use_count --;
    kernel_lock->in_use_count --;
    lock_entry_mutex->Release();

    if(kernel_condition->marked_for_destroy) {
        attempt_condition_delete(condition_index);
    }
    condition_entry_mutex->Release();

    return 0;
}


/**
 * Signal on a condition. The condition and lock must be within the calling
 * address space.
 *
 * @param condition_index - index of condition to signal on
 * @param lock_index - index of lock to use in the condition
 *
 * @return int - 0 on success -1 otherwise
 */
int SynchronizationLut::condition_signal(int condition_index, int lock_index) {
    if(!validate_condition_request(condition_index) || !validate_lock_request(lock_index)) {
        return -1;
    }

    KernelCondition *kernel_condition = condition_lookup_table[condition_index];
    KernelLock *kernel_lock = lock_lookup_table[lock_index];

    kernel_condition->condition->Signal(kernel_lock->lock);
    return 0;
}


/**
 * Broadcast on a condition. The condition and lock must be within the calling
 * address space.
 *
 * @param condition_index - index of condition to broadcast on
 * @param lock_index - index of lock to use in the condition
 *
 * @return int - 0 on success -1 otherwise
 */
int SynchronizationLut::condition_broadcast(int condition_index, int lock_index) {
    if(!validate_condition_request(condition_index) || !validate_lock_request(lock_index)) {
        return -1;
    }

    KernelCondition *kernel_condition = condition_lookup_table[condition_index];
    KernelLock *kernel_lock = lock_lookup_table[lock_index];

    kernel_condition->condition->Signal(kernel_lock->lock);
    return 0;
}


/**
 * De-allocate a condition. If the condition is currently in use,
 * the condition is marked for deletion at a future date.
 *
 * @return int: -1 Invalid request, index out of bounds or permission denied.
 *              0  The condition has been deleted or marked for deletion. No other guarantees are made.
 */
int SynchronizationLut::condition_delete(int condition_index) {
    if(!validate_condition_request(condition_index)) {
        return -1;
    }

    KernelCondition *kernel_condition = condition_lookup_table[condition_index];
    
    kernel_condition->marked_for_destroy = true;
    attempt_condition_delete(condition_index);

    return 0;
}


/**
 * Attempt to delete a lock based on the marked_for_destory and in_use_count
 * fields in the KernelLock struct. This function does no input validation so
 * it is critical that the calling function validates the request before calling.
 *
 * @param lock_index - index of the lock to be deleted
 *
 * @return bool - True if the lock no longer exists, False otherwise.
 */
bool SynchronizationLut::attempt_lock_delete(int lock_index) {
    KernelLock *kernel_lock = lock_lookup_table[lock_index];

    if(kernel_lock == NULL) {
        // kernel inconsistencies, we should never be executing this
        assert(false);
        return false;
    }

    if(!kernel_lock->marked_for_destroy) {
        // attempting to delete a lock that wasn't marked for delete
        return false;
    } else {
        if(kernel_lock->in_use_count > 0) {
            return false;
        }else {
            // let's delete the lock
            lock_table_mutex->Acquire();
            delete kernel_lock->lock;
            delete kernel_lock;
            lock_lookup_table[lock_index] = NULL;
            allocated_locks --;
            lock_table_mutex->Release();
            return true;
        }
    }

    assert(false);
    return false;
}


/**
 * Attempt to delete a condition based on the marked_for_destory and in_use_count
 * fields in the KernelCondtion struct. This function does no input validation so
 * it is critical that the calling function validates the request before calling.
 *
 * @param condition_index - index of the condition to be deleted
 *
 * @return bool - True if the condition no longer exists, False otherwise.
 */
bool SynchronizationLut::attempt_condition_delete(int condition_index) {
    KernelCondition *kernel_condition = condition_lookup_table[condition_index];

    if(kernel_condition == NULL) {
        // kernel inconsistencies, we should never be executing this
        assert(false);
        return false;
    }

    if(!kernel_condition->marked_for_destroy) {
        // attempting to delete a condition that wasn't marked for delete
        return false;
    } else {
        if(kernel_condition->in_use_count > 0) {
            return false;
        }else {
            // let's delete the lock
            condition_table_mutex->Acquire();
            delete kernel_condition->condition;
            delete kernel_condition;
            condition_lookup_table[condition_index] = NULL;
            allocated_conditions --;
            condition_table_mutex->Release();
            return true;
        }
    }

    assert(false);
    return false;
}


/**
 * Ensure the given lock index is within the table bounds, the
 * requesting thread has permission to operate on the lock, and
 * the table entry is not NULL. 
 *
 * @return bool True if it is a valid lock request
 *              False otherwise
 */
bool SynchronizationLut::validate_lock_request(int lock_index) {
    // bounds check the input index
    if(lock_index < 0 || lock_index >= MAX_SYSTEM_LOCKS) {
        return false;
    }

    KernelLock *kernel_lock = lock_lookup_table[lock_index];
    if(kernel_lock == NULL) {
        return false;
    }

    // ensure this thread has permissions to operate on this lock
    if(kernel_lock->address_space != currentThread->space) {
        return false;
    }

    // we should never reach this point
    assert(false);
    return true;
}


/**
 * Ensure the given condition index is within the table bounds, the
 * requesting thread has permission to operate on the condition, and
 * the table entry is not NULL.
 *
 * @return bool True if it is a valid condition request
 *              False otherwise
 */
bool SynchronizationLut::validate_condition_request(int condition_index) {
    // bounds check the input index
    if(condition_index < 0 || condition_index >= MAX_SYSTEM_CONDITIONS) {
        return false;
    }

    KernelCondition *kernel_condition = condition_lookup_table[condition_index];
    if(kernel_condition == NULL) {
        return false;
    }

    // ensure this thread has permissions to operate on this condition
    if(kernel_condition->address_space != currentThread->space) {
        return false;
    }

    return true;
}
