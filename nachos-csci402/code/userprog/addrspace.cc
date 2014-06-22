// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
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
    int i;	// to find the next slot

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
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
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
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable) : fileTable(MaxOpenFiles) {
    NoffHeader noffH;
    unsigned int i, size;
	//### Declare virtual, physical page number to read file
	int vpn, ppn;
	int vpn_initData;
	//###Lock for bit map
	Lock bitmap_lock("bitmap_lock");
    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

	
    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;
    numPages = divRoundUp(size, PageSize) + divRoundUp(UserStackSize,PageSize);
                                                // we need to increase the size
						// to leave room for the stack
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new TranslationEntry[numPages];
/*     for (i = 0; i < numPages; i++) {
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = i;
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
						// a separate page, we could set its 
						// pages to be read-only
    } */
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
					printf("\nGoing to read into physical memory\n");
	bitmap_lock.Acquire();
    if (noffH.code.size > 0) {
		//### Save the size of the code
		size = noffH.code.size;
					printf("nnoffH.code.size : %d\n", noffH.code.size);
		//### Determine how many virtual pages will fit
		vpn = divRoundUp(size, PageSize);
					printf("vpn : %d, PageSize: %d\n", vpn, PageSize);
					printf("noffH.code.inFileAddr : %d\n", noffH.code.inFileAddr);
					printf("Initializing code segment, at 0x%x, size %d\n", noffH.code.virtualAddr, noffH.code.size);
		for(int counter = 0; counter < vpn; counter++){
			//### Find a Page Number
			ppn = memory_map->Find(); 
			if(ppn == -1){
				printf("Error, all memory occupied\n");
				//Error, all memory occupied
			}
			executable->ReadAt(&(machine->mainMemory[ppn*PageSize]),
				PageSize, noffH.code.inFileAddr + counter*PageSize);
				printf("\tSaving to pageTable[%d]\n", counter);
				printf("\tPageTable.physicalPage : %d\n", ppn);
			pageTable[counter].virtualPage = counter;
			pageTable[counter].physicalPage = ppn;
			pageTable[counter].valid = TRUE;
			pageTable[counter].use = FALSE;
			pageTable[counter].dirty = FALSE;
			pageTable[counter].readOnly = FALSE;
		}
    }
    if (noffH.initData.size > 0) {
			printf("\n");
		vpn_initData = vpn;
		//### Save the size of the initData
		size = noffH.initData.size;	
		//### Determine how many virtual pages will fit
		vpn = divRoundUp(size, PageSize);	
					printf("vpn : %d\n", vpn);
					printf("noffH.initData.inFileAddr : %d\n", noffH.initData.inFileAddr);
					printf("Initializing data segment, at 0x%x, size %d\n",
						noffH.initData.virtualAddr, noffH.initData.size);
		for(int counter = 0; counter < vpn; counter++){
			//### Find a Page Number
					printf("Inside for loop: %d\n", counter);
			ppn = memory_map->Find(); 
			if(ppn == -1){
				printf("Error, all memory occupied\n");
				//Error, all memory occupied
			}
			executable->ReadAt(&(machine->mainMemory[ppn*PageSize]),
				PageSize, noffH.initData.inFileAddr + counter*PageSize);
				printf("\tSaving to pageTable[%d]\n", counter + vpn_initData);
				printf("\tPageTable.physicalPage : %d\n", ppn);
			pageTable[counter + vpn_initData].virtualPage = counter + vpn_initData;
			pageTable[counter + vpn_initData].physicalPage = ppn;
			pageTable[counter + vpn_initData].valid = TRUE;
			pageTable[counter + vpn_initData].use = FALSE;
			pageTable[counter + vpn_initData].dirty = FALSE;
			pageTable[counter + vpn_initData].readOnly = FALSE;
		}

    }
	//
	//Allocate Stack Pages
	//
	int stack_starting_page;
	printf("Allocating pageTable for stack\n");
	size = noffH.code.size + noffH.initData.size;
	stack_starting_page = divRoundUp(size,PageSize);
	numPages = divRoundUp(size,PageSize) + divRoundUp(UserStackSize,PageSize);
	for(unsigned int counter = stack_starting_page; counter < numPages; counter++){
		ppn = memory_map->Find(); 
					printf("ppn : %d\n", ppn);
			if(ppn == -1){
				printf("Error, all memory occupied\n");
				//Error, all memory occupied
			}
		printf("\tSaving to pageTable[%d]\n", counter);
		printf("\tPageTable.physicalPage : %d\n", ppn);
		pageTable[counter].virtualPage = counter;
		pageTable[counter].physicalPage = ppn;
		pageTable[counter].valid = TRUE;
		pageTable[counter].use = FALSE;
		pageTable[counter].dirty = FALSE;
		pageTable[counter].readOnly = FALSE;
	}
	bitmap_lock.Release();

}

//------------------------
//Update the pageTable to include new stack pages
//------------------------

int AddrSpace::newStack(){
	//Update numPages to include 8 new pages of stack
	Lock newStackLock("NewStackLock");
	newStackLock.Acquire();
	int ppn;
	unsigned int newNumPages = numPages + 8;
	TranslationEntry *NewPageTable = new TranslationEntry[newNumPages];
	//Copy the old page table to the new one
	for(unsigned int i = 0; i < numPages; i++){
		NewPageTable[i].virtualPage = pageTable[i].virtualPage;
		NewPageTable[i].physicalPage = pageTable[i].physicalPage;
		NewPageTable[i].valid = pageTable[i].valid;
		NewPageTable[i].use = pageTable[i].use;
		NewPageTable[i].dirty = pageTable[i].dirty;
		NewPageTable[i].readOnly = pageTable[i].readOnly;
					printf("Copying pageTable[%d] to NewPageTable\n", i);
	}
	//Remove the old table to free up resources
	delete pageTable;
	//Assign new stack to the new table
	for(unsigned int i = numPages; i < newNumPages; i++){
		printf("Assigning new Stack Pages %d\n", i);
		ppn = memory_map->Find(); 
		printf("\tPageTable.physicalPage : %d\n", ppn);
		if(ppn == -1){
			printf("Error, all memory occupied\n");
			//Error, all memory occupied
		}
		NewPageTable[i].virtualPage = i;
		NewPageTable[i].physicalPage = ppn;
		NewPageTable[i].valid = pageTable[i].valid;
		NewPageTable[i].use = pageTable[i].use;
		NewPageTable[i].dirty = pageTable[i].dirty;
		NewPageTable[i].readOnly = pageTable[i].readOnly;
	}
	//update numPages and pageTable and store the starting position of stack
	numPages = newNumPages;
	pageTable = NewPageTable;
	newStackLock.Release();
	return numPages*PageSize;
	



}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
// 	Dealloate an address space.  release pages, page tables, files
// 	and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
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
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
