#ifndef KOLSH_CODE_USSERPROG_SYNCHRONIZATION_LUT_H_
#define KOLSH_CODE_USSERPROG_SYNCHRONIZATION_LUT_H_

#include "addrspace.h"
#include "synch.h"

#define MAX_SYSTEM_LOCKS        100
#define MAX_SYSTEM_CONDITIONS   100

struct KernelLock {
    Lock *lock;
    AddrSpace *address_space;
    bool marked_for_destroy;
    int in_use_count;
};

struct KernelCondition {
    Condition *condition;
    AddrSpace *address_space;
    bool marked_for_destroy;
    int in_use_count;
};

class SynchronizationLut {
public:
    SynchronizationLut();
    ~SynchronizationLut();

    // for all functions return 0 on success
    // Lock functions
    int lock_create(char*);
    int lock_acquire(int);
    int lock_release(int);
    int lock_delete(int);

    // Condition functions
    int condition_create(char*);
    int condition_wait(int, int);
    int condition_signal(int, int);
    int condition_broadcast(int, int);
    int condition_delete(int);

private:
    Lock *lock_table_mutex;
    Lock *lock_entry_mutex;
    Lock *condition_table_mutex;
    Lock *condition_entry_mutex;

    KernelLock *lock_lookup_table[MAX_SYSTEM_LOCKS];
    KernelCondition *condition_lookup_table[MAX_SYSTEM_CONDITIONS];

    char *name;

    int next_available_lock_index;
    int next_available_condition_index;
    int allocated_locks;
    int allocated_conditions;

    bool attempt_lock_delete(int);
    bool attempt_condition_delete(int);

    bool validate_lock_request(int);
    bool validate_condition_request(int);
};

#endif // KOLSH_CODE_USSERPROG_SYNCHRONIZATION_LUT_H_
