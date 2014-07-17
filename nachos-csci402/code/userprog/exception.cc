// exception.cc 
//  Entry point into the Nachos kernel from user programs.
//  There are two kinds of things that can cause control to
//  transfer back to here from user code:
//
//  syscall -- The user code explicitly requests to call a procedure
//  in the Nachos kernel.  Right now, the only function we support is
//  "Halt".
//
//  exceptions -- The user code does something that the CPU can't handle.
//  For instance, accessing memory that doesn't exist, arithmetic errors,
//  etc.  
//
//  Interrupts (which can also cause control to transfer from user
//  code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <iostream>

using namespace std;
extern "C" { int bzero(char *, int); };


int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;            // The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
        result = machine->ReadMem( vaddr, 1, paddr );
        while(!result) { // FALL 09 CHANGES
            result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
        } 
      
        buf[n++] = *paddr;
     
        if(!result) {
            //translation failed
            return -1;
        }
        vaddr++;
    }

    delete paddr;
    return len;
}

char* read_into_new_buffer(unsigned int vaddr, int len) {
    char *buf = new char[len+1];            // Kernel buffer to put string in

    if(!buf) {
        return NULL;
    }

    if(copyin(vaddr, len, buf) == -1) {
        printf("%s","Read into buffer received an invalid character array.\n");
        delete buf;
        return NULL;
    }

    // null terminate the string
    buf[len]='\0';

    return buf;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;            // The number of bytes copied in

    while ( n >= 0 && n < len) {
        // Note that we check every byte's address
        result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );
        if ( !result ) {
            //translation failed
            return -1;
        }
        vaddr++;
    }

    return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];    // Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
        printf("%s","Bad pointer passed to Create\n");
        delete buf;
        return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];    // Kernel buffer to put the name in
    OpenFile *f;            // The new open file
    int id;             // The openfile id

    if (!buf) {
        printf("%s","Can't allocate kernel buffer in Open\n");
        return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
        printf("%s","Bad pointer passed to Open\n");
        delete[] buf;
        return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if(f) {
        if((id = currentThread->space->fileTable.Put(f)) == -1 ) {
            delete f;
        }
        return id;
    }
    else {
        return -1;
    }
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.
    
    char *buf;      // Kernel buffer for output
    OpenFile *f;    // Open file for output

    if ( id == ConsoleInput) return;
    
    if ( !(buf = new char[len]) ) {
    printf("%s","Error allocating kernel buffer for write!\n");
    return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
            printf("%s","Bad pointer passed to to write: data not written\n");
            delete[] buf;
            return;
        }
    }

    if ( id == ConsoleOutput) {
      for (int ii=0; ii<len; ii++) {
        printf("%c",buf[ii]);
      }

    } else {
    if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
        f->Write(buf, len);
    } else {
        printf("%s","Bad OpenFileId passed to Write\n");
        len = -1;
    }
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;      // Kernel buffer for input
    OpenFile *f;    // Open file for output

    if(id == ConsoleOutput){
        return -1;
    }
    
    if(!(buf = new char[len])) {
        printf("%s","Error allocating kernel buffer in Read\n");
        return -1;
    }

    if(id == ConsoleInput) {
        //Reading from the keyboard
        scanf("%s", buf);

        if(copyout(vaddr, len, buf) == -1) {
            printf("%s","Bad pointer passed to Read: data not copied\n");
        }
    }else {
        if((f = (OpenFile *) currentThread->space->fileTable.Get(id))) {
            len = f->Read(buf, len);
            if(len > 0) {
                //Read something from the file. Put into user's address space
                if(copyout(vaddr, len, buf) == -1) {
                    printf("%s","Bad pointer passed to Read: data not copied\n");
                }
            }
        }else {
            printf("%s","Bad OpenFileId passed to Read\n");
            len = -1;
        }
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if(f) {
        delete f;
    }else {
        printf("%s","Tried to close an unopen file\n");
    }
}

void Yield_Syscall(){
    printf("Yielding the current thread\n");
    currentThread->Yield();
}

// **************************************************************************************
// LOCK SYSTEM CALLS
// User-level lock operations: Create, Acquire, Release, Delete.
// Input validation is done in target functions
//

int lock_create_syscall(unsigned int vaddr, int length) {
    char* lock_name = read_into_new_buffer(vaddr, length);
    if(lock_name == NULL) {
        lock_name = "Synchronization lock";
    }

    return synchronization_lut->lock_create(lock_name);
}

int lock_acquire_syscall(int lock_index) {
    return synchronization_lut->lock_acquire(lock_index);
}

int lock_release_syscall(int lock_index) {
    return synchronization_lut->lock_release(lock_index);
}

int lock_delete_syscall(int lock_index) {
    return synchronization_lut->lock_delete(lock_index);
}

// **************************************************************************************
// CONDITION INTERFACE
// User-level condition operations: Create, Wait, Signal, Broadcast, Delete.
// Input validation is done in target functions
//

int condition_create_syscall(unsigned int vaddr, int length) {
    char* condition_name = read_into_new_buffer(vaddr, length);
    if(condition_name == NULL) {
        condition_name = "Synchronization condition";
    }
    return synchronization_lut->condition_create(condition_name);
}

int condition_wait_syscall(int condition_index, int lock_index) {
    return synchronization_lut->condition_wait(condition_index, lock_index);
}

int condition_signal_syscall(int condition_index, int lock_index) {
    return synchronization_lut->condition_signal(condition_index, lock_index);
}

int condition_broadcast_syscall(int condition_index, int lock_index) {
    return synchronization_lut->condition_broadcast(condition_index, lock_index);
}

int condition_delete_syscall(int condition_index) {
    return synchronization_lut->condition_delete(condition_index);
}

int print_f_syscall(unsigned int vaddr, int length) {
    char* output_buffer = read_into_new_buffer(vaddr, length);
    printf("%s", output_buffer);
    delete output_buffer;
    return 0;
}

class func_info_class {
    public:
        void (*func);
        char *name;
};

//-----------------------
//
//   Fork SysCall
//
//----------------------------


void kernel_thread(int value){
        printf("\t######   Fork SysCall kernel_thread:   ######\n");
    //translate the pseudo int to a valid pointer to a class that stores function and name
    func_info_class *my_info = (func_info_class*) value;
    int start_point = currentThread->space->newStack(); //Allocate new stack for this addrSpace
    currentThread->space->RestoreState();
    //=========Process Table===============
    //Find an available thread spot
    bool found_T = false;
    int P_ID = currentThread->space->ProcessID;
    int ptr;
    for(int t = 0; t < Ptable_MaxThread; t++){
        if(ProcessTable[P_ID].threads[t].myThread == NULL){
            ProcessTable[P_ID].threadCount++;
            ptr = t;
            found_T = true;
            break;
        }
    }
    if(found_T = false){
        //All Thread spot are taken
        printf("Error: No Thread spot avaiable in process table[%d]\n", P_ID);
        interrupt->Halt();
    } 
        printf("Setting ProcessTable[%d]: Thread[%d], threadcount:%d\n", P_ID, ptr, ProcessTable[P_ID].threadCount);
    currentThread->thread_ID = ptr;
    ProcessTable[P_ID].threads[ptr].myThread = currentThread;
    ProcessTable[P_ID].threads[ptr].firstStackPage = (machine->pageTableSize) * PageSize;
    //======================================
    machine->WriteRegister(PCReg, (int) my_info->func);
    machine->WriteRegister(NextPCReg, (int) my_info->func + 4);
        printf("Store into PC registers: func = %d\n", (int) my_info->func);
        printf("Store into NextPC registers: func = %d\n", (int) my_info->func + 4);
    //write to the stack register , the starting postion of the stack for this thread.
    machine->WriteRegister(StackReg, (start_point*PageSize) - 16);
    machine->Run();

}


int Fork_Syscall(void (*func), unsigned int vaddr, int length){
    printf("Fork_Syscall:\n");
    //Check if the address is within boundary
    if(!currentThread->space->checkAddr((int) func)){
        printf("Error: Trying to fork to an invalid address: %d\n", (int) func);
        return -1;
    }
    //Check if the maximun number of threads is reached
    int P_ID = currentThread->space->ProcessID;
    if(ProcessTable[P_ID].threadCount >= Ptable_MaxThread){
        printf("Error: Maximum number of threads reached, Quitting the program");
        interrupt->Halt();
    }
    //Store the name of the function
    char *name = read_into_new_buffer(vaddr, length);
    if(func == NULL){
        return -1;
    }
    func_info_class *info_ptr = new func_info_class();
    info_ptr->func = func;
    info_ptr->name = name;

    Thread *t = new Thread(name);
    //Forked thread have the same addrSpace as currentThread
        printf("Current Thread: %s\n", currentThread->getName());
    t->space = currentThread->space;
    t->Fork((VoidFunctionPtr) kernel_thread, (int)info_ptr);
    //machine->Run();
    currentThread->Yield();
    return 0;

}

//----------------------------
//
//       Exec SysCall
//
//----------------------------

void exec_kernel_function(int i){
    printf("Inside exec_kernel_function:\n");
    printf("Current Thread: %s\n", currentThread->getName());
    currentThread->space->InitRegisters();     // set the initial register values
    currentThread->space->RestoreState();      // load page table register   

    //Update the process table and related data structure
    //Find an available process spot
    bool found_P = false, found_T = false;
    int ptr, P_ID;
    for(int p = 0; p < Ptable_MaxProcess; p++){
        if(ProcessTable[p].as == NULL){
            Process_counter++;
            P_ID = p;
            found_P = true;
            break;
        }
    }
    //Find an available thread spot
    for(int t = 0; t < Ptable_MaxThread; t++){
        if(ProcessTable[P_ID].threads[t].myThread == NULL){
            ProcessTable[P_ID].threadCount++;
            ptr = t;
            found_T = true;
            break;
        }
    }
    if(found_P = false){
        //All process spot are taken
        printf("Error: No spot avaiable in process table\n");
        interrupt->Halt();
    }
    if(found_T = false){
        //All Thread spot are taken
        printf("Error: No Thread spot avaiable in process table[%d]\n", P_ID);
        interrupt->Halt();
    } 
    //Assign values to the Process Table
    printf("Found ProcessID: %d, threadID: %d FirstStackPage: %d\n",P_ID, ptr, machine->pageTableSize * PageSize);
    currentThread->space->ProcessID = P_ID;
    currentThread->thread_ID = ptr;
    ProcessTable[P_ID].as = currentThread->space;
    ProcessTable[P_ID].threads[ptr].myThread = currentThread;
    ProcessTable[P_ID].threads[ptr].firstStackPage = machine->pageTableSize * PageSize;
    machine->Run();
}


int exec_syscall(unsigned int vaddr, int size){

    char *buf = new char[size+1];    // Kernel buffer to put the name in
    if (!buf) {
    printf("%s","Can't allocate kernel buffer in Open\n");
    return -1;
    }

    if( copyin(vaddr,size,buf) == -1 ) {
    printf("%s","Bad pointer passed to Open\n");
    delete[] buf;
    return -1;
    }

    buf[size]='\0';
    printf("Characters read from vaddr: %s\n", buf);

    //Open the file name stored in buf
    OpenFile *user_executable = fileSystem->Open(buf);
    if (user_executable == NULL) {
       printf("Unable to open file %s\n", buf);
       return -1;
    }
    //Create a new thread with the address space of the file
    AddrSpace *file_space = new AddrSpace(user_executable);

    Thread *exec_thread = new Thread("exec_thread");
    exec_thread->space = file_space;
    exec_thread->Fork(exec_kernel_function, 0);

    
    //delete user_executable;          // close file
    printf("Current Thread: %s\n", currentThread->getName());
    while(!scheduler->getreadyList()->IsEmpty()){
        currentThread->Yield();
    }
    return 0;
}

//===============================
//
//          Exit Syscall
//Pass in 0 to return normally
//
//===============================

int exit_syscall(unsigned int status){
    Lock exitLock("exitLock");
    printf("Exit(%d)\n", status);
    exitLock.Acquire();
        printf("In Exit_Syscall:\n");
    //Get the current process ID
    int P_ID = currentThread->space->ProcessID;
    int ptr = currentThread->thread_ID;
    //Print out the debugging message
        printf("\tProcess Count: %d, Thread Count: %d\n", Process_counter, ProcessTable[P_ID].threadCount);
        printf("\tProcessTable[%d]\t Thread_ID: %d\n", P_ID, ptr);
        printf("CurrentThread: %s, ProcessTable[%d]\n", currentThread->getName(), P_ID);
        //Set the process table to exit the current thread
        ProcessTable[P_ID].threads[ptr].myThread = NULL;
        ProcessTable[P_ID].threads[ptr].firstStackPage = 0;
        ProcessTable[P_ID].threadCount--;
    if(ProcessTable[P_ID].threadCount > 1){
            printf("Going to Remove Stack\n");
        //Child threads exist, reclaim 8 stack pages and go to sleep
        int stack_start = ProcessTable[P_ID].threads[ptr].firstStackPage;
        if(stack_start == 0){
            printf("Error: Trying to remove stack page 0\n");
            return -1;
        }
        currentThread->space->removeStack(stack_start);
        exitLock.Release();
        currentThread->Finish();
    }
    else if(ProcessTable[P_ID].threadCount == 1 && Process_counter > 1){
        //If last thread but not last process
        printf("Last thread but not last process\n");
        //Reclaim all memory
        currentThread->space->returnMemory();
        //Recliam all locks
        delete ProcessTable[P_ID].as->file_ptr;
        ProcessTable[P_ID].as = NULL;
        exitLock.Release();
        currentThread->Finish();

    }
    else if(ProcessTable[P_ID].threadCount == 1 && Process_counter == 1){
        //Last Thread in the last process
        printf("Last Thread in the last process\n");
        delete ProcessTable[P_ID].as->file_ptr;
        exitLock.Release();
        interrupt->Halt();
    }
    return 0;
}
//----------Sprintf-----------------
//mychar is the output char*
//text will be use to determine the formatting Ex: "Today is the %d\n"
//i is the number that will replace %d in text
//------------------------------------

int sprintf_syscall(unsigned int mychar, unsigned int text, int i){
    unsigned int vpn, offset;
    TranslationEntry *entry;
    unsigned int physAddr;
    unsigned int pageFrame;

    vpn = (unsigned) text / PageSize;
    offset = (unsigned) text % PageSize;
    if (vpn >= machine->pageTableSize) {
        DEBUG('a', "virtual page # %d too large for page table size %d!\n", 
            text, machine->pageTableSize);
        printf("virtual page # %d too large for page table size %d!\n", 
            text, machine->pageTableSize);
        return 1;
    }
    if(machine->tlb == NULL){
        entry = &machine->pageTable[vpn];
    }
    else{
        entry = &machine->tlb[vpn];
    }
    pageFrame = entry->physicalPage;
    if (pageFrame >= NumPhysPages) { 
        DEBUG('a', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
        printf( "*** frame %d > %d!\n", pageFrame, NumPhysPages);
        return 1;
    } 
    entry->use = TRUE;
    physAddr = pageFrame * PageSize + offset;
    unsigned int adder = physAddr;
    char buf = 'a';
    int text_count = 0;
    while(buf != '\0'){
        buf = machine->mainMemory[adder];
        adder++;
        text_count++;
    }
    char* Input_text = read_into_new_buffer(physAddr, text_count);
    char* output_char = new char[40];
    sprintf(output_char, Input_text, i);
    char nextchar = output_char[0];
    for(int j = 0; nextchar != '\0'; j++){
        nextchar = output_char[j];
        copyout(mychar++, 1, &nextchar);
    }
    return 0;
}

//-----------------------------------------------
//             Handle Memory Full
//
// When the memory is filled, evict one page of
// memory using FIFO or random select method.
//-----------------------------------------------

int handleMemoryFull(){
    int ppn,vpn;
    memFullLock->Acquire();
    if(isFIFO){
        //Use FIFO
        ppn = (int) FIFO_list->Remove();
    }
    else {
        //Use Random
        ppn = rand() % NumPhysPages;
    }
    //Need to check if this ppn is in the TLB, if it is, we need to invalidate it
    for (int i = 0; i < TLBSize; i++){
        if(machine->tlb[i].physicalPage == ppn){
            machine->tlb[i].valid = FALSE;
        }
    }
    //The virtual page that will be removed from memory
    vpn = IPT[ppn].virtualPage;
    //Set the use bit
    IPT[ppn].use = TRUE;
    currentThread->space->ExPageTable[vpn].use = TRUE;
    memFullLock->Release();

    //If the IPT is dirty, write to swap file
    if(IPT[ppn].dirty == TRUE){
        //Write to swap file
        int swapPage = swap_map->Find();
        if(swapPage == -1){
            //printf("SwapFile full, Exiting program\n");
            interrupt->Halt();
        }
        swap_file->WriteAt(&machine->mainMemory[ppn*PageSize],PageSize, swapPage*PageSize);
        currentThread->space->ExPageTable[vpn].byteoffset =  swapPage*PageSize;
        currentThread->space->ExPageTable[vpn].diskLocation = 1;
        currentThread->space->ExPageTable[vpn].dirty = TRUE;
        currentThread->space->ExPageTable[vpn].valid = FALSE;
    }

    currentThread->space->ExPageTable[vpn].use = FALSE;
    IPT[ppn].use = FALSE;
    //Add this page back to FIFO queue at the end
    FIFO_list->Append((void*)ppn);
    return ppn;
}

//-----------------------------------------------
//             Handle IPT Misses
//
// If IPT missed during the PageFault Exception
// Perfrom this function and it will return the proper
// Physical Page number
//-----------------------------------------------

int handleIPTMiss(int vpn){
    //define the range of vpn that is within the executable, file_size includes stack pages.
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    int vpn_range = currentThread->space->file_size;
    int ppn, P_ID;
    P_ID = currentThread->space->ProcessID;
    //The data is not in IPT, so we are loading the data into memory
    //Virtual Page is in the executable or swap file
    ppn = memory_map->Find();
    if(ppn == -1){
        //All memory occupied,  evict a page
        //printf("Memory Full, evict a page\n");
        ppn = handleMemoryFull();
        //printf("MemFull: obtain ppn:%d\n", ppn);
    }
    else{
        //Insert the page to the back of the FIFO list
        FIFO_list->Append((void*)ppn);
    }
    //Acquire the IPT lock
    IPTLock->Acquire();
    //printf("Found an available physical page: %d\n", ppn);
    //Clear the space one page at a time and write to memory
    bzero(&(machine->mainMemory[ppn*PageSize]), PageSize);
    if(currentThread->space->ExPageTable[vpn].diskLocation == 2){
        //Do nothing as the page is not in executable or swap
    }
    else if(currentThread->space->ExPageTable[vpn].diskLocation == 1){
        //Read from the swap file
        //printf("Writing into mainMemoryPage#%d from swap file\n", ppn);
        swap_file->ReadAt(&(machine->mainMemory[ppn*PageSize]),
                PageSize, currentThread->space->ExPageTable[vpn].byteoffset);
    }
    else {
        //Read from the executable
        //printf("Writing into physicalPage[%d]\n", ppn);
        currentThread->space->file_ptr->ReadAt(&(machine->mainMemory[ppn*PageSize]),
                PageSize, 40 + vpn*PageSize);
    }
    // Update Translation Tables
    // ExPageTable is the extended version of page table decalred in addrspace.h
    currentThread->space->ExPageTable[vpn].physicalPage = ppn;
    currentThread->space->ExPageTable[vpn].valid = TRUE;

    //IPT is the inverted page table declared in system.h with size of MaxPhysPages
    IPT[ppn].virtualPage = vpn;
    IPT[ppn].physicalPage = ppn;
    IPT[ppn].valid = TRUE;
    IPT[ppn].ProcessID = currentThread->space->ProcessID;
    IPT[ppn].use = FALSE;
    IPT[ppn].dirty = currentThread->space->ExPageTable[vpn].dirty;
    
    (void) interrupt->SetLevel(oldLevel);
    return ppn;
}   


void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);    // Which syscall?
    int rv = 0;                             // the return value from a syscall

    if ( which == SyscallException ) {
        switch (type) {
            default:
                DEBUG('a', "Unknown syscall - shutting down.\n");
                break;
            case SC_Halt:
                DEBUG('a', "Shutdown, initiated by user program.\n");
                interrupt->Halt();
                break;
            case SC_Create:
                DEBUG('a', "Create syscall.\n");
                Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
                break;
            case SC_Open:
                DEBUG('a', "Open syscall.\n");
                rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
                break;
            case SC_Write:
                DEBUG('a', "Write syscall.\n");
                Write_Syscall(machine->ReadRegister(4),
                          machine->ReadRegister(5),
                          machine->ReadRegister(6));
                break;
            case SC_Read:
                DEBUG('a', "Read syscall.\n");
                rv = Read_Syscall(machine->ReadRegister(4),
                          machine->ReadRegister(5),
                          machine->ReadRegister(6));
                break;
            case SC_Close:
                DEBUG('a', "Close syscall.\n");
                Close_Syscall(machine->ReadRegister(4));
                break;
            case SC_Yield:
                DEBUG('a', "Yield Syscall.\n");
                Yield_Syscall();
                break;
            case SC_Fork:
                DEBUG('a', "Fork.\n");
                rv = Fork_Syscall( (void*) (machine->ReadRegister(4)), 
                                                machine->ReadRegister(5),
                                                machine->ReadRegister(6));
                break;
            case SC_Exec:
                DEBUG('a',"Exec.\n");
                rv = exec_syscall(machine->ReadRegister(4),machine->ReadRegister(5));
                break; 
            case SC_Exit:
                DEBUG('a',"Exit.\n");
                rv = exit_syscall(machine->ReadRegister(4));
                break;  
            // LOCK SYSTEM CALLS
            case SC_LOCK_CREATE:
                DEBUG('a', "Lock create syscall.\n");
                rv = lock_create_syscall(machine->ReadRegister(4),
                                            machine->ReadRegister(5));
                break;
            case SC_LOCK_ACQUIRE:
                DEBUG('a', "Lock acquire syscall.\n");
                rv = lock_acquire_syscall(machine->ReadRegister(4));
                break;      
            case SC_LOCK_RELEASE:
                DEBUG('a', "Lock release syscall.\n");
                rv = lock_release_syscall(machine->ReadRegister(4));
                break;
            case SC_LOCK_DELETE:
                DEBUG('a', "Lock delete syscall.\n");
                rv = lock_delete_syscall(machine->ReadRegister(4));
                break;
            // CONDITION SYSTEM CALLS
            case SC_CONDITION_CREATE:
                DEBUG('a', "Condition create syscall.\n");
                rv = condition_create_syscall(machine->ReadRegister(4),
                                                machine->ReadRegister(5));
                break;
            case SC_CONDITION_WAIT:
                DEBUG('a', "Condition wait syscall.\n");
                rv = condition_wait_syscall(machine->ReadRegister(4),
                                                machine->ReadRegister(5));
                break;
            case SC_CONDITION_SIGNAL:
                DEBUG('a', "Condition signal syscall.\n");
                rv = condition_signal_syscall(machine->ReadRegister(4),
                                                machine->ReadRegister(5));
                break;
            case SC_CONDITION_BROADCAST:
                DEBUG('a', "Condition broadcast syscall.\n");
                rv = condition_broadcast_syscall(machine->ReadRegister(4),
                                                    machine->ReadRegister(5));
                break;
            case SC_CONDITION_DELETE:
                DEBUG('a', "Condition delete syscall.\n");
                rv = condition_delete_syscall(machine->ReadRegister(4));
                break;
            case SC_Print_F:
                DEBUG('a', "Print F sys call.\n");
                rv = print_f_syscall(machine->ReadRegister(4),
                                    machine->ReadRegister(5));
                break;
            case SC_Sprintf:
                DEBUG('a', "Sprintf sys call.\n");
                rv = sprintf_syscall(machine->ReadRegister(4),
                                    machine->ReadRegister(5),
                                        machine->ReadRegister(6));
                break;
        }
        // Put in the return value and increment the PC
        machine->WriteRegister(2,rv);
        machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
        return;
    }else if(which == PageFaultException) {
            //printf("\nUserMode Exception: [PageFaultException]\n");
        //Set the interrupts off when Exception occurs
        IntStatus oldLevel = interrupt->SetLevel(IntOff);

        //Set the dirty bits
        for (int i = 0; i < TLBSize; i++){
            if(machine->tlb[i].valid == TRUE){
                //If the tlb is dirty, write to the corresponding IPT
                IPT[machine->tlb[i].physicalPage].dirty = machine->tlb[i].dirty;
                currentThread->space->ExPageTable[machine->tlb[i].virtualPage].dirty = machine->tlb[i].dirty;
            }
        }

        int ppn = -1;
        int P_ID = currentThread->space->ProcessID;
        //Record the virtual address that throw this error in vpn
        int vpn = (machine->ReadRegister(BadVAddrReg)) / PageSize;
        //Validate the vpn
        //currentThread->space->file_size returns file size in number of pages exclude stacks
        int vpn_range = currentThread->space->file_size;
            //printf("BadVAddrReg: %d, P_ID: %d, vpn_range: %d\n", vpn, P_ID, vpn_range);
        if(vpn > (vpn_range + divRoundUp(UserStackSize,PageSize) - 1) ){
            //Invalid vpn
            printf("Invalid vpn address\n");
            interrupt->Halt();
        }
        //###### Need an IPT lock
        IPTLock->Acquire();

        //Check all the pages stored in IPT for the 3 parameters
        for (int i = 0; i < NumPhysPages; i++){
            if(IPT[i].valid == TRUE && IPT[i].virtualPage == vpn && IPT[i].ProcessID == P_ID ){
                ppn = i;
                break;
            }
        }
        if(ppn == -1){
            //IPT missed
                //printf("IPT missed, going to handleIPTMiss\n");
            //RElease IPT lock here
            IPTLock->Release();
            ppn = handleIPTMiss(vpn);   ///### Need to acquire a ipt lock in this function
        }
        //printf("Updating to TLB[%d] with ppn: %d\n", currentTLB, ppn);
        //Update the tlb page table
        machine->tlb[currentTLB].virtualPage = vpn;
        machine->tlb[currentTLB].physicalPage = ppn;
        machine->tlb[currentTLB].valid = TRUE;
        machine->tlb[currentTLB].dirty = IPT[ppn].dirty;
        machine->tlb[currentTLB].use = FALSE;
        machine->tlb[currentTLB].readOnly = FALSE;
        //Update the TLB index
        currentTLB = (currentTLB + 1) % TLBSize;

        (void) interrupt->SetLevel(oldLevel);
        IPTLock->Release();
        return;

    }else if(which == ReadOnlyException) {
        printf("UserMode Exception: [ReadOnlyException]\n");
        interrupt->Halt();
    }else if(which == BusErrorException) {
        printf("UserMode Exception: [BusErrorException]\n");
        interrupt->Halt();
    }else if(which == AddressErrorException) {
        printf("UserMode Exception: [AddressErrorException]\n");
        interrupt->Halt();
    }else {
        printf("Unexpected user mode exception - which:[%d] type:[%d]\n", which, type);
        interrupt->Halt();
    }
}
