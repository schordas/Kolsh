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


LockLut::release_lock(int index){
	//Check if the index is within boundary
	if(index >= 0 && index < MAX_SYSTEM_LOCKS){
		if(lock_lookup_table[index]->address_space == currentThread->Space){
			lock_lookup_table[index]->lock->Release();
		}
		else{
			printf("Lock Release Error: current Address Space does not equal to Lock's Address Space\n");
			return -1;	
		}
	}
	else{
		printf("Lock Release Error: Lock index is out of bounds\n");
		return -1;
	}
	return 0;
int LockLut::acquire_lock(int kernel_lock_index){
    KernelLock *kernel_lock_to_acquire;

    // bounds check on kernel_lock_index
    if((kernel_lock_index < 0)||(kernel_lock_index >= MAX_SYSTEM_LOCKS)){
        // kernel lock index is out of array bounds
        return -1;
    }

    // ensure that the the thread has permission to acquire this lock
    if(kernel_lock_to_acquire->address_space != currentThread->space) {
        // currentThread does not have permission to acquire this lock
        return -1;
    }

    // get kernel lock at index kernel_lock_index
    kernel_lock_to_acquire = lock_lookup_table[kernel_lock_index];

    if(kernel_lock_to_acquire == NULL){
        // kernel_lock_to_acquire is null, cannot be acquired
        return -1;
    }
    // mark the kernal_lock_to_acquire as busy
    kernel_lock_to_acquire->in_use = true;
    // acquire the kernal_lock_to_aquire's lock
    kernel_lock_to_acquire->lock->Acquire();

    return 0;
}


/**
 * De-allocate a lock. If the lock is currently in use,
 * the lock is marked for deletion at a future date.
 *
 * @return int: -2 Calling thread does not have permission to delete this lock
 *              -1 Supplied lock index is invalid
 *              0  The lock has been deleted or marked for deletion. No other guarantees are made.
 */
int LockLut::free_lock(int lock_index) {
    KernelLock *kernel_lock;

    // bounds check the input index
    if(lock_index < 0 || lock_index >= MAX_SYSTEM_LOCKS) {
        return -1;
    }

    kernel_lock = lock_lookup_table[lock_index];

    // ensure this thread has permissions to delete this lock
    if(kernel_lock->address_space != currentThread->space) {
        return -2;
    }

    // confirm the lock can be deleted. If it can't,
    // mark it for deletion.
    if(kernel_lock->in_use) {
        kernel_lock->marked_for_destroy = true;
        return 0;
    } else {
        delete kernel_lock->lock;
        delete kernel_lock;
        lock_lookup_table[lock_index] = NULL;
        allocated_locks --;
        return 0;
    }
}
