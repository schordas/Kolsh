
#ifndef KOLSH_CODE_USSERPROG_LOCK_LUT_H_
#define KOLSH_CODE_USSERPROG_LOCK_LUT_H_

#include "addrspace.h"
#include "synch.h"

#define MAX_SYSTEM_LOCKS 100

struct KernelLock {
    Lock *lock;
    AddrSpace *address_space;
    bool marked_for_destroy;
    bool in_use;
};

class LockLut {
public:
    LockLut();
    ~LockLut();

    int allocate_lock(char*);
	int release_lock(int index);	//Return 0 if successful, -1 if there is an error
    int acquire_lock(int);
    int free_lock(int lock_index);

private:
    KernelLock *lock_lookup_table[MAX_SYSTEM_LOCKS];
    Lock *table_lock;
    int next_available_lock_index;
    int allocated_locks;
    char *name;
};

#endif // KOLSH_CODE_USSERPROG_LOCK_LUT_H_
