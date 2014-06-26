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
#include "translate.h"
#include "thread.h"


#define USER_STACK_SIZE         1024    // Stack size in bytes. With
                                        // current system settings,
                                        // translates to 8 pages.
#define MaxOpenFiles            256
#define MaxChildSpaces          256
#define MAX_PROCESS_THREADS     64
#define Ptable_MaxThread        100

class Thread;

struct ThreadEntry{
        int firstStackPage;
        Thread *myThread;
    };
        

class AddrSpace {
  public:

    
    AddrSpace(OpenFile *executable, int process_id);    // Create an address space,
                                                        // initializing it with the program
                                                        // stored in the file "executable"
    ~AddrSpace();                                       // De-allocate an address space

    void InitRegisters();                       // Initialize user-level CPU registers,
                                                // before jumping to user code

    void SaveState();                           // Save/restore address space-specific
    void RestoreState();                        // info on a context switch
    
    Table fileTable;                            // Table of openfiles

    int get_number_of_threads();                //return the number of active threads

    void decrement_number_of_threads();

    int get_process_id();                       // return the process id this address space is associated with.
    
    void removeStack(int stack);
    int newStack(Thread *thread);               //Allocate new stack pages for Fork syscall
	bool checkAddr(unsigned int vaddr);			//Check if the virtual address is within this addrSpace
    int getnumPages(){return numPages;}         //Return the numPages variable
 private:
    // Lock needed for manipulating/access number of threads
    Lock *manipulate_number_of_threads;
    TranslationEntry *pageTable;    // Assume linear page table translation
                                    // for now!
    unsigned int numPages;          // Number of pages in the virtual 
                                    // address space
    int process_id;                 // process_id for this address space
    int number_of_threads;          //keep track of active threads
    ThreadEntry threads[Ptable_MaxThread];
};

#endif // ADDRSPACE_H
