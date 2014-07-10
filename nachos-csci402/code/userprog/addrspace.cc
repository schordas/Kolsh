// addrspace.cc 
//  Routines to manage address spaces (executing user programs).
//
//  In order to run a user program, you must:
//
//  1. link with the -N -T 0 option 
//  2. run coff2noff to convert the object file to Nachos format
//      (Nachos object code format is essentially just a simpler
//      version of the UNIX executable object code format)
//  3. load the NOFF file into the Nachos file system
//      (if you haven't implemented the file system yet, you
//      don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include <assert.h>
#include <stdio.h>
#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "table.h"


extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if(table) {
       delete table;
       table = 0;
    }
    if(lock) {
       delete lock;
       lock = 0;
    }
}

void *Table::Get(int i) {
    // Return the element associated with the given if, or 0 if
    // there is none.

    return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f) {
    // Put the element in the table and return the slot it used.  Use a
    // lock so 2 files don't get the same space.
    int i;  // to find the next slot

    lock->Acquire();
    i = map.Find();
    lock->Release();
    if(i != -1) {
       table[i] = f;
    }
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if(i >= 0 && i < size) {
       lock->Acquire();
       if(map.Test(i)) {
           map.Clear(i);
           f = table[i];
           table[i] = 0;
       }
       lock->Release();
    }
    return f;
}

//----------------------------------------------------------------------
// SwapHeader
//  Do little endian to big endian conversion on the bytes in the 
//  object file header, in case the file was generated on a little
//  endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void SwapHeader(NoffHeader *noffH) {
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//  Create an address space to run a user program.
//  Load the program from a file "executable", and set everything
//  up so that we can start executing user instructions.
//
//  Assumes that the object code file is in NOFF format.
//
//  "executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely constructed address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable, int in_process_id) : fileTable(MaxOpenFiles) {
    NoffHeader noffH;
    int ppn;
    int process_address_space_size_in_bytes;

    // we can't construct an address space without a valid process_id
    // we take it that the caller has done their due diligence
    // to ensure that is a valid process id. We have no way of checking.
    this->process_id = in_process_id;

    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC)) {
        SwapHeader(&noffH);
    }
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    
    printf("Code: %d bytes, initData: %d bytes, uninitData: %d bytes.\n", 
        noffH.code.size, noffH.initData.size, noffH.uninitData.size);
    
    if(noffH.code.size <= 0) {
        // ERROR, how can you have an executable with 0 code?
        // TODO: no code executable error handler
        printf("How can you have 0 code in your executable?\n");
        assert(FALSE);
    }

    // we will not allocate a new stack space for the process threads directly
    // in the constructor. There is a separate allocate and de-allocate function
    // to handle AddressSpace Stack management.
    process_address_space_size_in_bytes = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
    numPages = divRoundUp(process_address_space_size_in_bytes, PageSize);

    ASSERT(numPages <= NumPhysPages);       // check we're not trying
                                            // to run anything too big --
                                            // at least until we have
                                            // virtual memory

    DEBUG('a', "Initializing address space, num pages %d, process_address_space_size_in_bytes %d\n with one thread stack.",
                    numPages, process_address_space_size_in_bytes);
                    
        
    // request all the physical pages required by the user-program
    // if we don't have enough physical memory to load the program,
    // release the pages we have already requested, and notify the
    // caller that we can't complete their request due to a lack of
    // memory. Initialize all the page entries to their initial conditions.
    pageTable = new TranslationEntry[numPages];
    memory_map_mutex->Acquire();
    for(unsigned int i = 0; i < numPages; i++) {
        ppn = memory_map->Find();   // Find an available physical memory page
        if(ppn == -1) {
            // Error, all memory occupied
            // TODO: release all requested memory back to the OS
            printf("FATAL ERROR:\n");
            printf("AddrSpace::AddrSpace out of system memory.\n");
            assert(FALSE);
        }
        
        bzero((machine->mainMemory) + (PageSize * ppn), PageSize); // clear the returned page
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = ppn;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;
    }
    memory_map_mutex->Release();
    
    // we have now allocated all the pages we need in memory.
    // we can go ahead and copy the relevant sections from
    // the executable into main memory.
    const int num_executable_pages = divRoundUp(noffH.code.size + noffH.initData.size, PageSize);
    const int last_code_page = divRoundUp(noffH.code.size, PageSize);
    for(int vpn = 0; vpn < num_executable_pages; vpn++) {
        ppn = pageTable[vpn].physicalPage;
        // we can't mark the last code page as read only because
        // it might be split with the data section.
        if(vpn < last_code_page) {
            pageTable[vpn].readOnly = TRUE;
        }
        executable->ReadAt(&(machine->mainMemory[ppn*PageSize]), PageSize, noffH.code.inFileAddr + vpn*PageSize);
        
        // print out the executable read from file
        // this does not print the executable header.
        if(DEBUG_BUILD && (DEBUG_VERBOSITY_LEVEL >= 3)) {
            char *process_memory = (machine->mainMemory)+(ppn*PageSize);
            for(int c = 0; c < PageSize; c+=2) {
                if(c % 16 == 0) {
                    printf("\n");
                }
                printf("%02x", *(process_memory+c) & 0xff);
                printf("%02x ", *(process_memory+c+1) & 0xff);
            }
        }

    }
    printf("\n\n");

    DEBUG('t', "pre stack address space, num pages %d, process_address_space_size_in_bytes %d\n with one thread stack.\n",
                    numPages, process_address_space_size_in_bytes);
    addrspace_mutex = new Lock("AddressSpace Lock");
    newStack();

    DEBUG('t', "Initialized address space, num pages %d, process_address_space_size_in_bytes %d\n with one thread stack.\n",
                    numPages, process_address_space_size_in_bytes);
    printf("Address space constructed\n");
}

/**
 * Allocate a new stack space for the current process. Thread safe.
 *
 * @return int - the starting virtual address of the new stack space
 */
int AddrSpace::newStack() {
    addrspace_mutex->Acquire();

    const unsigned int number_of_new_stack_pages = 8;
    const unsigned int numPages_updated = this->numPages + number_of_new_stack_pages;
    const unsigned int last_allocated_virtual_page = this->numPages;
    TranslationEntry *new_pageTable = new TranslationEntry[numPages_updated];

    // Copy old page table to new one
    const unsigned int size_of_page_table = sizeof(TranslationEntry) * this->numPages;
    memcpy(new_pageTable, pageTable, size_of_page_table);

    //Assign new stack to the new table
    memory_map_mutex->Acquire();
    for(unsigned int i = last_allocated_virtual_page; 
            i < last_allocated_virtual_page + number_of_new_stack_pages; 
            i++) {
        
        const int ppn = memory_map->Find();   // Find an available physical memory page
        if(ppn == -1) {
            // Error, all memory occupied
            // TODO: release all requested memory back to the OS
            printf("FATAL ERROR:\n");
            printf("AddrSpace::newStack out of system memory.\n");
            assert(FALSE);
        }
        
        bzero((machine->mainMemory) + (PageSize * ppn), PageSize); // clear the returned page
        new_pageTable[i].virtualPage = i;
        new_pageTable[i].physicalPage = ppn;
        new_pageTable[i].valid = TRUE;
        new_pageTable[i].use = FALSE;
        new_pageTable[i].dirty = FALSE;
        new_pageTable[i].readOnly = FALSE;
    }
    memory_map_mutex->Release();

    // update class values
    this->numPages = numPages_updated;
    this->pageTable = new_pageTable;

    addrspace_mutex->Release();
    return (this->numPages * PageSize) - 16;
}


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
//  Deallocate an address space.  release pages, page tables, files
//  and file tables
//----------------------------------------------------------------------
AddrSpace::~AddrSpace() {
    delete pageTable;
    delete addrspace_mutex;
}

//----------------------------------------------------------------------
// AddrSpace::checkAddr
//
//	Check the virtual address passed in is within the boundary
//----------------------------------------------------------------------
bool AddrSpace::checkAddr(unsigned int vaddr){
	if(vaddr < numPages*PageSize && vaddr >= 0){
		//vaddr is in boundary
		return true;
	}else{
	
		return false;
	}


}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//  Set the initial values for the user-level register set.
//
//  We write these directly into the "machine" registers, so
//  that we can immediately jump to user code.  Note that these
//  will be saved/restored into the currentThread->userRegisters
//  when this thread is context switched out.
//----------------------------------------------------------------------

void AddrSpace::InitRegisters() {
    int i;

    for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);   

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %x\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//  On a context switch, save any machine state, specific
//  to this address space, that needs saving.
//
//  For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() {}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//  On a context switch, restore the machine state so that
//  this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() {
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

int AddrSpace::get_process_id() {
    return 0;
}

bool AddrSpace::is_invalid_code_address(unsigned int vaddr) {
    return FALSE;
}

