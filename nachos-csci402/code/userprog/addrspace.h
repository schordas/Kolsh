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

#define UserStackSize       1024    // increase this as necessary!

#define MaxOpenFiles 256
#define MaxChildSpaces 256

#define MAX_PROCESS_THREADS 100

class Lock;

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable,                         // Create an address space,
        const unsigned int process_id);                     // initializing it with the program
                                                            // stored in the file "executable"
    
    ~AddrSpace();                                           // De-allocate an address space

    void InitRegisters();                                   // Initialize user-level CPU registers,
                                                            // before jumping to user code

    void SaveState();                                       // Save/restore address space-specific
    void RestoreState();                                    // info on a context switch
    Table fileTable;                                        // Table of openfiles
    
    bool is_valid_code_vaddr(const unsigned int vaddr);     // returns if passed in vaddr is within the code bounds
    bool is_valid_data_vaddr(const unsigned int vaddr);     // returns if vaddr is within the data bounds
    int allocate_new_thread_stack();                        // allocate a new thread stack
                                                            // returns the start address of the stack
    unsigned int get_process_id();                          // returns the process id
    void decrement_running_thread_count();                  // thread safe decrement of number_of_running_threads

 private:
    unsigned int address_space_size;            // returns numPages * PageSize
    unsigned int numPages;                      // Number of memory pages in the virtual address space
    unsigned int code_vaddr_fence;              // last virtual address of code in the address space
    unsigned int process_id;                    // process id
    int number_of_running_threads;              // number of running threads in address space

    TranslationEntry *pageTable;                // Assume linear page table translation for now!
    Lock *address_space_mutex;                  // mutex for address space operations
};

#endif // ADDRSPACE_H
