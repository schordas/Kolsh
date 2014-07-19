// addrspace.h 
//  Data structures to keep track of executing user programs 
//  (address spaces).
//
//  For now, we don't keep any information about address spaces.
//  The user level CPU state is saved and restored in the thread
//  executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "table.h"
#include "synch.h"
#include "thread.h"

#define UserStackSize       1024    // increase this as necessary!

#define MaxOpenFiles        256
#define MaxChildSpaces      256

#define MAX_PROCESS_THREADS 105

class Lock;
class Thread;

struct StackTableEntry {
    Thread* thread_ptr;
    unsigned int vpn_stack_start;
    unsigned int vpn_stack_end;
    unsigned int vaddr_stack_start;
    bool in_use;
};

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable,                         // Create an address space,
        unsigned int in_process_id,                         // initializing it with the program
        Thread* main_thread);                               // stored in the file "executable"
    
    ~AddrSpace();                                           // De-allocate an address space

    void InitRegisters();                                   // Initialize user-level CPU registers,
                                                            // before jumping to user code

    void SaveState();                                       // Save/restore address space-specific
    void RestoreState();                                    // info on a context switch
    Table fileTable;                                        // Table of openfiles
    
    bool is_valid_code_vaddr(const unsigned int vaddr);     // returns if passed in vaddr is within the code bounds
    bool is_valid_data_vaddr(const unsigned int vaddr);     // returns if vaddr is within the data bounds
    int allocate_new_thread_stack(Thread* thread_ptr);      // allocate a new thread stack
                                                            // returns the start address of the stack
    unsigned int get_process_id();                          // returns the process id
    unsigned int get_running_thread_count();                // returns the number of running threads
    bool release_thread_resources(Thread* thread_ptr);      // thread safe decrement of number_of_running_threads

 private:
    unsigned int address_space_size;            // returns numPages * PageSize

    unsigned int numPages;                      // Number of memory pages in the virtual address space
    unsigned int numStacks;                     // number of allocated stacks for this address space
    unsigned int code_vaddr_fence;              // last virtual address of code in the address space
    unsigned int stack_vpn_offset;              // offset to the beginning of the first stack page
    unsigned int process_id;                    // process id
    unsigned int number_of_running_threads;     // number of running threads in address space
    

    StackTableEntry *stackTable;                // stack table
    TranslationEntry *pageTable;                // Assume linear page table translation for now!
    Lock *address_space_mutex;                  // mutex for address space operations

    bool allocate_additional_stack_spaces_();   // expand the page and stack table to accommodate 8 more threads

};

#endif // ADDRSPACE_H
