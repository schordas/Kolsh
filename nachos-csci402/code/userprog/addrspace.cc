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

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"
#include "synch.h"

extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if (table) {
    delete table;
    table = 0;
    }
    if (lock) {
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
    if ( i != -1)
    table[i] = f;
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if ( i >= 0 && i < size ) {
    lock->Acquire();
    if ( map.Test(i) ) {
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

static void 
SwapHeader (NoffHeader *noffH)
{
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
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------
AddrSpace::AddrSpace(OpenFile *executable, 
                        unsigned int in_process_id,
                        Thread* main_thread) : fileTable(MaxOpenFiles) {
    // function data
    unsigned int executable_pages;
    int executable_size;
    NoffHeader noffH;

    // initialize class private data
    address_space_mutex = new Lock("address_space_mutex");
    number_of_running_threads = 1;      // we start with the main thread running.
    process_id = in_process_id;


    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC)) {
        SwapHeader(&noffH);
    }
    
    // validate executable
    ASSERT(noffH.noffMagic == NOFFMAGIC);
    ASSERT(noffH.code.size > 0);

    code_vaddr_fence = noffH.code.size;

    executable_size = noffH.code.size + noffH.initData.size + noffH.uninitData.size;
   
    // Total number of physical pages required by the process to accommodate
    // code, initialized data, uninitialized data and one thread stack space.
    stack_vpn_offset = divRoundUp(executable_size, PageSize);
    numStacks = 1;
    numPages = stack_vpn_offset + (8 * numStacks);
    address_space_size = numPages * PageSize;

    // TODO remove once we having memory paging.
    ASSERT(numPages <= NumPhysPages);

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", numPages, address_space_size);
        
    pageTable = new TranslationEntry[numPages];
    memory_map_mutex->Acquire();
    for (unsigned int i = 0; i < numPages; i++) {
        int physical_page_number = memory_map->Find();   // Find an available physical memory page
        if(physical_page_number == -1) {
            // Error, all memory occupied -- this should be resolved when we
            //                                  implement paging so don't stress for now
            // TODO: release all requested memory back to the OS
            printf("FATAL ERROR:\n");
            printf("AddrSpace::AddrSpace out of system memory.\n");
            assert(FALSE);
        }

        pageTable[i].physicalPage = physical_page_number;
        pageTable[i].virtualPage = i;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;

        bzero(&machine -> mainMemory[PageSize * pageTable[i].physicalPage], PageSize);
    }
    memory_map_mutex -> Release();
    
    // read the executable to main memory
    executable_pages = divRoundUp(executable_size, PageSize);
    for(unsigned int i = 0; i < executable_pages; i++) {
        executable->ReadAt(&(machine -> mainMemory[pageTable[i].physicalPage * PageSize]),
        PageSize, noffH.code.inFileAddr + i * PageSize);
    }

    stackTable = new StackTableEntry[numStacks];
    for(unsigned int i = 0; i < numStacks; i++) {
        stackTable[i].thread_ptr = NULL;
        stackTable[i].vpn_stack_start = ((8*i)+1)+stack_vpn_offset;
        stackTable[i].vpn_stack_end = (8*(i+1))+stack_vpn_offset;
        stackTable[i].vaddr_stack_start = (stackTable[i].vpn_stack_end * PageSize) - 16;
        stackTable[i].in_use = FALSE;
    }

    stackTable[0].thread_ptr = main_thread;
    stackTable[0].in_use = TRUE;
}

/**
 * Find an available thread stack to use.
 */
int AddrSpace::allocate_new_thread_stack(Thread* thread_ptr) {
    if(thread_ptr == NULL) {
        return -2;
    }
    address_space_mutex->Acquire();

    if(number_of_running_threads == MAX_PROCESS_THREADS) {
        DEBUG('t', "OVER MAX PROCESS THREADS. FAILING REQUEST\n");
        address_space_mutex->Release();
        return -1;
    }

    if(number_of_running_threads == numStacks) {
        if(!allocate_additional_stack_spaces_()) {
            // this should never happen
            ASSERT(FALSE);
        }
    }

    unsigned int next_sSpace = 0;
    for(next_sSpace; next_sSpace < numStacks; next_sSpace++) {
        if(stackTable[next_sSpace].in_use) {
            continue;
        }
        break;
    }

    if(next_sSpace == numStacks) {
        // should never happen
        ASSERT(FALSE);
    }

    stackTable[next_sSpace].in_use = TRUE;
    stackTable[next_sSpace].thread_ptr = thread_ptr;

    number_of_running_threads++;

    address_space_mutex->Release();
    
    return stackTable[next_sSpace].vaddr_stack_start;
}

bool AddrSpace::allocate_additional_stack_spaces_() {
    const unsigned int num_additional_stacks = 8;
    const unsigned int num_additional_pages = 8 * num_additional_stacks;
    address_space_mutex->Acquire();
    
    if(numStacks + num_additional_stacks > MAX_PROCESS_THREADS) {
        // can't allocate additional space
        address_space_mutex->Release();
        return false;
    }


    TranslationEntry *new_page_table = new TranslationEntry[numPages + num_additional_pages];
    StackTableEntry *new_stack_table = new StackTableEntry[numStacks + num_additional_stacks];

    // copy the two tables
    memcpy(new_page_table, pageTable, numPages * sizeof(TranslationEntry));
    memcpy(new_stack_table, stackTable, numStacks * sizeof(StackTableEntry));

    // request RAM for new stack space
    memory_map_mutex->Acquire();
    for(unsigned int i = numPages; i < numPages + num_additional_pages; i++) {
        int physical_page_number = memory_map->Find();   // Find an available physical memory page
        if(physical_page_number == -1) {
            // Error, all memory occupied -- this should be resolved when we
            //                                  implement paging so don't stress for now
            // TODO: release all requested memory back to the OS
            printf("FATAL ERROR:\n");
            printf("AddrSpace::AddrSpace out of system memory.\n");
            assert(FALSE);
        }

        new_page_table[i].physicalPage = physical_page_number;
        new_page_table[i].virtualPage = i;
        new_page_table[i].valid = TRUE;
        new_page_table[i].use = FALSE;
        new_page_table[i].dirty = FALSE;
        new_page_table[i].readOnly = FALSE;

        bzero(&machine -> mainMemory[PageSize * new_page_table[i].physicalPage], PageSize);
    }
    memory_map_mutex->Release();
    
    // initialize the new stack table
    for(unsigned int i = numStacks; i < numStacks + num_additional_stacks; i++) {
        new_stack_table[i].thread_ptr = NULL;
        new_stack_table[i].vpn_stack_start = ((8*i)+1)+stack_vpn_offset;
        new_stack_table[i].vpn_stack_end = (8*(i+1))+stack_vpn_offset;
        new_stack_table[i].vaddr_stack_start = (new_stack_table[i].vpn_stack_end * PageSize) - 16;
        new_stack_table[i].in_use = FALSE;
    }

    // we really shouldn't be context switched in the following section
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    delete pageTable;
    delete stackTable;

    pageTable = new_page_table;
    stackTable = new_stack_table;
    (void) interrupt->SetLevel(oldLevel);

    numPages += num_additional_stacks * 8;
    numStacks += num_additional_stacks;
    address_space_size = numPages * PageSize;
    machine->pageTable=pageTable;

    address_space_mutex->Release();
    return TRUE;
}

bool AddrSpace::is_valid_code_vaddr(const unsigned int vaddr) {
    // we are making the assumption you cannot fork main
    // therefore vaddr 0 (NULL) is invalid
    return (vaddr > 0 && vaddr <= code_vaddr_fence);
}

bool AddrSpace::is_valid_data_vaddr(const unsigned int vaddr) {
    return (vaddr > code_vaddr_fence && vaddr < address_space_size);
}

/**
 *
 */
unsigned int AddrSpace::get_process_id() {
    return process_id;
}

/**
 *
 */
bool AddrSpace::release_thread_resources(Thread* thread_ptr) {
    if(address_space_mutex->isHeldByCurrentThread()) {
        return false;
    }
    address_space_mutex->Acquire();

    // find the stackTable entry that corresponds to thread_ptr
    unsigned int sTable_index = 0;
    for(sTable_index; sTable_index < numStacks; sTable_index++) {
        if(stackTable[sTable_index].thread_ptr != thread_ptr) {
            continue;
        }
        break;
    }

    if(sTable_index == numStacks) {
        printf("kernel inconsistencies: Thread does not exist in bond address space\n");
        ASSERT(FALSE);
    }

    stackTable[sTable_index].thread_ptr = NULL;
    stackTable[sTable_index].in_use = FALSE;
    number_of_running_threads--;

    // TODO release RAM back to system
    memory_map_mutex->Acquire();
    for(unsigned int i = stackTable[sTable_index].vpn_stack_start;
            i <= stackTable[sTable_index].vpn_stack_start; 
            i++) {
        memory_map->Clear(pageTable[i].physicalPage);
    }
    memory_map_mutex->Release();
    address_space_mutex->Release();
    return true;
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
//  Dealloate an address space.  release pages, page tables, files
//  and file tables
//----------------------------------------------------------------------
AddrSpace::~AddrSpace() {
    // TODO release allocated pages
    delete address_space_mutex;
    delete pageTable;
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

void
AddrSpace::InitRegisters()
{
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

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//  On a context switch, restore the machine state so that
//  this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
