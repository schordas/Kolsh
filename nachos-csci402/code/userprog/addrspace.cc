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

AddrSpace::AddrSpace(OpenFile *executable) : fileTable(MaxOpenFiles) {
    NoffHeader noffH;
    //### Declare virtual, physical page number to read file
    int vpn, ppn, size;
    unsigned int NotStackPages;
    //###Lock for bit map
    newStackLock = new Lock("NewStackLock");
    stackLock = new Lock("RemoveStackLock");

    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    file_ptr = executable; //Store the executable pointer
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if((noffH.noffMagic != NOFFMAGIC) && (WordToHost(noffH.noffMagic) == NOFFMAGIC)) {
        SwapHeader(&noffH);
    }
    ASSERT(noffH.noffMagic == NOFFMAGIC);
    printf("Code: %d bytes, initData: %d bytes, uninitData: %d bytes.\n", 
    noffH.code.size, noffH.initData.size, noffH.uninitData.size) ;
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;

    numPages = divRoundUp(size, PageSize) + divRoundUp(UserStackSize,PageSize);
    file_size = divRoundUp(size, PageSize);
    ExPageTable = new ExtendedTranslationEntry[numPages];
    
    NotStackPages = divRoundUp(size, PageSize);
                                                // we need to increase the size
                                                // to leave room for the stack
    size = numPages * PageSize;

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
                    numPages, size);
    printf("NumPage: %d, NotStackPage: %d\n", numPages, NotStackPages);
    for (unsigned int i = 0; i < numPages; i++) {
        ExPageTable[i].virtualPage = i;
        ExPageTable[i].valid = TRUE;
        ExPageTable[i].use = FALSE;
        ExPageTable[i].dirty = FALSE;
        ExPageTable[i].readOnly = FALSE;
        ExPageTable[i].diskLocation = 0;
        if(i > NotStackPages - 1){
            printf("StackPage[%d]\n",i);
            ExPageTable[i].diskLocation = 2;
        }
    }

}

//------------------------
//Update the pageTable to include new stack pages
//------------------------

int AddrSpace::newStack(){
    //Update numPages to include 8 new pages of stack
    printf("allocating new stack pages\n");
    newStackLock->Acquire();
    int ppn;
    unsigned int newNumPages = numPages + 8;
    ExtendedTranslationEntry *NewPageTable = new ExtendedTranslationEntry[newNumPages];
    //Copy the old page table to the new one
    for(unsigned int i = 0; i < newNumPages; i++){
        if (i < numPages){
            NewPageTable[i].virtualPage = ExPageTable[i].virtualPage;
            NewPageTable[i].physicalPage = ExPageTable[i].physicalPage;
            NewPageTable[i].valid = ExPageTable[i].valid;
            NewPageTable[i].use = ExPageTable[i].use;
            NewPageTable[i].dirty = ExPageTable[i].dirty;
            NewPageTable[i].readOnly = ExPageTable[i].readOnly;
            NewPageTable[i].byteoffset = ExPageTable[i].byteoffset;
            NewPageTable[i].diskLocation = ExPageTable[i].diskLocation;
        }
        else{
            //New Stack Pages Here
            //printf("Assigning new Stack Pages [%d] with ppn : [%d]\n", i, ppn);
            NewPageTable[i].virtualPage = i;
            NewPageTable[i].valid = TRUE;
            NewPageTable[i].use = FALSE;
            NewPageTable[i].dirty = FALSE;
            NewPageTable[i].readOnly = FALSE;
            NewPageTable[i].diskLocation = 2;
        }
    }
    //Remove the old table to free up resources
    delete ExPageTable;
    //update numPages and pageTable and store the starting position of stack
    numPages = newNumPages;
    ExPageTable = NewPageTable;
    newStackLock->Release();
    return newNumPages*PageSize;
}

//------------------------
// Remove the stack pages specified
//------------------------

void AddrSpace::removeStack(int stack){
    stackLock->Acquire();
    unsigned int stack_page = stack;
        printf("Deleting stack pages: %d in Thread: %s\n", stack_page, currentThread->getName());
    unsigned int newNumPages = numPages - 8;
    ExtendedTranslationEntry *NewPageTable = new ExtendedTranslationEntry[newNumPages];
    //Copy the section before the stack
    for(unsigned int i = 0; i < stack_page - 8; i++){
        NewPageTable[i].virtualPage = ExPageTable[i].virtualPage;
        NewPageTable[i].physicalPage = ExPageTable[i].physicalPage;
        NewPageTable[i].valid = ExPageTable[i].valid;
        NewPageTable[i].use = ExPageTable[i].use;
        NewPageTable[i].dirty = ExPageTable[i].dirty;
        NewPageTable[i].readOnly = ExPageTable[i].readOnly;
           // printf("Copying pageTable[%d] to NewPageTable with ppn: %d\n", i, pageTable[i].physicalPage);
    }
    //Free up physical memory space
    for(unsigned int i = stack_page - 8; i < stack_page; i++){
        for(int j = 0; j <TLBSize; i++){
            if(machine->tlb[j].virtualPage == ExPageTable[i].virtualPage){
                machine->tlb[j].valid = FALSE;
                //Remove it from the memory map
                memory_map->Clear(ExPageTable[i].physicalPage);
                //Update FIFO list
                List *newList = new List;
                int currentFIFO = 0;
                while(!FIFO_list->IsEmpty() ) {
                    currentFIFO = (int) FIFO_list->Remove();
                    if(currentFIFO != ExPageTable[i].physicalPage){
                        newList->Append( (void*) currentFIFO );
                    }
                }
                delete FIFO_list;
                FIFO_list = newList;
            }
        }
        for(int j = 0; j < NumPhysPages; j++){
            if(IPT[j].virtualPage == ExPageTable[i].virtualPage){
                IPT[j].valid = FALSE;
            }
        }
    }
        // int pa = ExPageTable[i].physicalPage;
        //     printf("Freeing physical page: %d\n", pa);
        // memory_map->Clear(pa);
    //Copy the section after the stack
    for(unsigned int i = stack_page - 8; i < newNumPages; i++){
        NewPageTable[i].virtualPage = ExPageTable[stack_page].virtualPage;
        NewPageTable[i].physicalPage = ExPageTable[stack_page].physicalPage;
        NewPageTable[i].valid = ExPageTable[stack_page].valid;
        NewPageTable[i].use = ExPageTable[stack_page].use;
        NewPageTable[i].dirty = ExPageTable[stack_page].dirty;
        NewPageTable[i].readOnly = ExPageTable[stack_page].readOnly;
        stack_page++;
           // printf("Copying pageTable[%d] to NewPageTable[%d] with ppn: %d\n", stack_page, i, pageTable[i].physicalPage);
    }
    //Remove the old table
    delete ExPageTable;
    numPages = newNumPages;
    ExPageTable = NewPageTable;
    stackLock->Release();
}

//----------------------------------------------------------------------
// AddrSpace::returnMemory
//
//  Deallocate all memory assigned to physical pages
//----------------------------------------------------------------------

void AddrSpace::returnMemory(){
    printf("AddrSpace returnMemory:");
    stackLock->Acquire();
    for(unsigned int i = 0; i < numPages; i++){
        for(int j = 0; j < NumPhysPages; j++){
            if(ExPageTable[i].virtualPage == IPT[j].virtualPage){
                IPT[j].valid == FALSE;
                //Remove it from the memory map
                memory_map->Clear(ExPageTable[i].physicalPage);
                //Update FIFO list
                List *newList = new List;
                int currentFIFO = 0;
                while(!FIFO_list->IsEmpty() ) {
                    currentFIFO = (int) FIFO_list->Remove();
                    if(currentFIFO != ExPageTable[i].physicalPage){
                        newList->Append( (void*) currentFIFO );
                    }
                }
                delete FIFO_list;
                FIFO_list = newList;
            }

    
        }
        for(int j = 0; j <TLBSize; i++){
            if(machine->tlb[j].virtualPage == ExPageTable[i].virtualPage){
                machine->tlb[j].valid = FALSE;
            }
        }
        //printf("Freeing physical page: %d\n", ExPageTable[i].physicalPage);
    }
    stackLock->Release();
}


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
//  Deallocate an address space.  release pages, page tables, files
//  and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace() {
    delete ExPageTable;
}

//----------------------------------------------------------------------
// AddrSpace::checkAddr
//
//  Check the virtual address passed in is within the boundary
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
    //machine->pageTable = pageTable;
    //invalidate all the tlb entries on a context switch
    for (int i = 0; i < TLBSize; i++){
        if(machine->tlb[i].valid == TRUE){
            IPT[machine->tlb[i].physicalPage].dirty = TRUE;
            ExPageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
        }
        machine->tlb[i].valid = FALSE;
    }
    machine->pageTableSize = numPages;
}
