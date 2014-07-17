// system.h 
//  All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

// used to toggle debug print statements
// a submission build should be compiled
// with DEBUG_BUILD set to false.
// DEBUG_VERBOSITY_LEVEL is an int in the
// range 1 to 3 inclusive. 1 is the least
// verbose, and 3 is the most verbose.
#define DEBUG_BUILD             true
#define DEBUG_VERBOSITY_LEVEL   3

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "../userprog/addrspace.h"
#include "timer.h"

// Initialization and cleanup routines
extern void Initialize(int argc, char **argv);  // Initialization,
                                                // called before anything else

extern void Cleanup();                  // Cleanup, called when
                                        // Nachos is done.

//Struct for ProcessTable
#define Ptable_MaxProcess 10
#define Ptable_MaxThread 100

struct ThreadEntry{
	int firstStackPage;
	Thread *myThread;
};
struct ProcessEntry{
    int threadCount;
    AddrSpace *as;
    ThreadEntry threads[Ptable_MaxThread];
};

//--------------------------
// Virtual Memory Management
//--------------------------
class InvertedPageTable {
  public:
    int virtualPage;        // The page number in virtual memory.
    int physicalPage;       // The page number in real memory (relative to the
                            //  start of "mainMemory"
    bool valid;             // If this bit is set, the translation is ignored.
                            // (In other words, the entry hasn't been initialized.)
    int ProcessID;
    
    bool use;               // This bit is set by the hardware every time the
                            // page is referenced or modified.
    bool dirty;             // This bit is set by the hardware every time the
                            // page is modified.
};

extern Thread *currentThread;           // the thread holding the CPU
extern Thread *threadToBeDestroyed;     // the thread that just finished
extern Scheduler *scheduler;            // the ready list
extern Interrupt *interrupt;            // interrupt status
extern Statistics *stats;               // performance metrics
extern Timer *timer;                    // the hardware alarm clock

#ifdef USER_PROGRAM
#include "machine.h"
#include "bitmap.h"
#include "synchronization_lut.h"
extern bool isFIFO;
extern Machine *machine;            // user program memory and registers
extern BitMap *memory_map;
extern ProcessEntry *ProcessTable;
extern SynchronizationLut *synchronization_lut; // user program synchronization lock lookup table
extern int Process_counter;
// Virtual Memory Management
extern InvertedPageTable *IPT;
extern int currentTLB;
extern List *FIFO_list;
extern OpenFile *swap_file;
extern BitMap *swap_map;
extern Lock *memFullLock;
extern Lock *forkLock;
extern Lock *IPTLock;
#endif


#ifdef FILESYS_NEEDED                   // FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#endif // SYSTEM_H
