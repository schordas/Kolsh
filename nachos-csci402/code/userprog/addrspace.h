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
#include "../filesys/filesys.h"
#include "table.h"
#include "translate.h"

#define UserStackSize       1024    // increase this as necessary!

#define MaxOpenFiles 256
#define MaxChildSpaces 256

class ExtendedTranslationEntry {
  public:
    int virtualPage;        // The page number in virtual memory.
    int physicalPage;       // The page number in real memory (relative to the
                            //  start of "mainMemory"
    bool valid;             // If this bit is set, the translation is ignored.
                            // (In other words, the entry hasn't been initialized.)
    bool readOnly;          // If this bit is set, the user program is not allowed
                            // to modify the contents of the page.
    bool use;               // This bit is set by the hardware every time the
                            // page is referenced or modified.
    bool dirty;             // This bit is set by the hardware every time the
                            // page is modified.
    int diskLocation;       // The location for the current page
                            // 0 = execuatable, 1 = swap file, 2 = neither
    int byteoffset;
};

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);    // Create an address space,
                                        // initializing it with the program
                                        // stored in the file "executable"
    ~AddrSpace();                       // De-allocate an address space

    void InitRegisters();       // Initialize user-level CPU registers,
                                // before jumping to user code
    void SaveState();           // Save/restore address space-specific
    void RestoreState();        // info on a context switch
    Table fileTable;            // Table of openfiles
    
    int newStack();             //Allocate new stack pages for Fork syscall
    bool checkAddr(unsigned int vaddr);         //Check if the virtual address is within this addrSpace
    void returnMemory();
    void removeStack(int stack);
    int getnumPages(){return numPages;} //Return the numPages variable

    
    int ProcessID;
    int file_size;                  // The size of the file opened
    OpenFile * file_ptr;            // The executable, or the file opened
    ExtendedTranslationEntry *ExPageTable;    // Assume linear page table translation
 private:
    unsigned int numPages;          // Number of pages in the virtual 
                                    // address space
    Lock *newStackLock;
    Lock *stackLock;
};
#endif  // ADDRSPACE_H
