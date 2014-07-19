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
#include "addrspace.h"
#include "syscall.h"
#include <stdio.h>
#include <iostream>

using namespace std;

int thread_count = 0;

int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;      // The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) // FALL 09 CHANGES
    {
        result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
    } 
      
      buf[n++] = *paddr;
     
      if ( !result ) {
  //translation failed
  return -1;
      }

      vaddr++;
    }

    delete paddr;
    return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;      // The number of bytes copied in

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

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];  // Kernel buffer to put the name in

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
    char *buf = new char[len+1];  // Kernel buffer to put the name in
    OpenFile *f;      // The new open file
    int id;       // The openfile id

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

    if ( f ) {
  if ((id = currentThread->space->fileTable.Put(f)) == -1 )
      delete f;
  return id;
    }
    else
  return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.
    
    char *buf;    // Kernel buffer for output
    OpenFile *f;  // Open file for output

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
    char *buf;    // Kernel buffer for input
    OpenFile *f;  // Open file for output

    if ( id == ConsoleOutput) return -1;
    
    if ( !(buf = new char[len]) ) {
  printf("%s","Error allocating kernel buffer in Read\n");
  return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
      scanf("%s", buf);

      if ( copyout(vaddr, len, buf) == -1 ) {
  printf("%s","Bad pointer passed to Read: data not copied\n");
      }
    } else {
  if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
      len = f->Read(buf, len);
      if ( len > 0 ) {
          //Read something from the file. Put into user's address space
            if ( copyout(vaddr, len, buf) == -1 ) {
        printf("%s","Bad pointer passed to Read: data not copied\n");
    }
      }
  } else {
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

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}

/**
 * thread_data[0] = vaddr of fork function
 * thread_data[1] = stack start address of thread
 */
void kernel_thread_function(const unsigned int* thread_data) {
    unsigned int vaddr = thread_data[0];
    unsigned int stack_start = thread_data[1];
    
    machine->WriteRegister(PCReg, vaddr);
    machine->WriteRegister(NextPCReg, vaddr+4);

    currentThread->space->RestoreState();

    machine->WriteRegister(StackReg, stack_start);

    machine->Run();
}

int sc_fork(const unsigned int func_to_fork_vaddr,
                const unsigned int char_buff,
                const unsigned int buff_length) {

    AddrSpace *currentThread_addrspace = currentThread->space;
    int* thread_data = new int[2];
    char *thread_name;

    // validate the function pointer
    // since we can accept NULL as an input for char_buff
    // we'll validate that later on.
    if(!currentThread_addrspace->is_valid_code_vaddr(func_to_fork_vaddr)) {
        return -1;
    }

    if(char_buff == NULL) {
        thread_name = new char[sizeof("user thread")];
        sprintf(thread_name, "user thread");
    }else {
        if(!currentThread_addrspace->is_valid_data_vaddr(char_buff)) {
            return -1;
        }
        thread_name = read_into_new_buffer(char_buff, buff_length); // this is freed when delete 
                                                                    // is called on the thread   
    }
    
    Thread *kernel_thread = new Thread(thread_name);

    kernel_thread->space = currentThread_addrspace;
    int stack_start = kernel_thread->space->allocate_new_thread_stack(kernel_thread);
    if(stack_start == -1) {
        // process has reached its thread limit.
        // the fork request cannot be completed
        return -2;
    }

    thread_data[0] = func_to_fork_vaddr;
    thread_data[1] = (unsigned int)stack_start;

    DEBUG('t', "Preparing to fork thread [%s]\n", thread_name);
    kernel_thread->Fork((VoidFunctionPtr)kernel_thread_function, (int)thread_data);

    return 0;
}

void sc_printf(const unsigned int vaddr,
                const unsigned int buff_length) {

    char* output_buffer = read_into_new_buffer(vaddr, buff_length);
    printf("%s", output_buffer);
    delete output_buffer;
}

void sc_printl(const int literal) {
    printf("%d", literal);
}

void sc_exit(const int exit_status) {

    /**
     *
     */


    currentThread->space->release_thread_resources(currentThread);
    currentThread->Finish();
};

void sc_yield() {
    currentThread->Yield();
}

/**
 * exec_data[0] = OpenFile* executable
 * exec_data[1] = int process_id
 */
void kernel_exec_function(int exec_data) {
    currentThread->space->InitRegisters();     // set the initial register values
    currentThread->space->RestoreState();      // load page table register

    printf("running a new proceess?\n");

    machine->Run();             // jump to the user progam
    ASSERT(FALSE);              // machine->Run never returns;
                                // the address space exits
                                // by doing the syscall "exit"
}

int sc_exec(const unsigned int filename_addr, 
                const unsigned int buff_length) {

    char *filename = read_into_new_buffer(filename_addr, buff_length);
    OpenFile *executable = fileSystem->Open(filename);

    if(executable == NULL) {
        printf("Unable to open file %s\n", filename);
        delete filename;
        delete executable;
        return -1;
    }

    int process_id = process_table->acquire_new_process_id();
    if(process_id < 0) {
        // we've reached the limit on the number of processes we can have
        delete filename;
        delete executable;
        return -2;
    }

    char *process_name = new char[buff_length + 5];
    sprintf(process_name,"%s_main", filename);
    Thread *kernel_exec_thread = new Thread(process_name);

    AddrSpace *space = new AddrSpace(executable, process_id, kernel_exec_thread);
    if(!process_table->bind_address_space(process_id, space)) {
        // what the hell went wrong? Recovery isn't really possible.
        ASSERT(FALSE);
    }

    kernel_exec_thread->space = space;

    kernel_exec_thread->Fork((VoidFunctionPtr)kernel_exec_function, process_id);
    
    delete executable;
    delete filename;

    return process_id;
}

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);    // Which syscall?
    int rv=0;                               // the return value from a syscall

    if(which == SyscallException ) {
        switch (type) {
            default:
                DEBUG('e', "Unknown syscall type [%d] - shutting down.\n", type);
                break;
            case SC_Yield:
                DEBUG('e', "Yield syscall\n");
                sc_yield();
                break;
            case SC_Exec:
                DEBUG('e', "Exec syscall\n");
                rv = sc_exec(machine->ReadRegister(4),
                                machine->ReadRegister(5));
                break;
            case SC_Fork:
                DEBUG('e', "Fork syscall.\n");
                rv = sc_fork(machine->ReadRegister(4),
                                machine->ReadRegister(5),
                                machine->ReadRegister(6));
                break;
            case SC_Halt:
                DEBUG('e', "Shutdown, initiated by user program.\n");
                interrupt->Halt();
                break;
            case SC_Exit:
                DEBUG('e', "Exit Syscall.\n");
                sc_exit(machine->ReadRegister(4));
                break;
            case SC_Create:
                DEBUG('e', "Create syscall.\n");
                Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
                break;
            case SC_Open:
                DEBUG('e', "Open syscall.\n");
                rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
                break;
            case SC_Write:
                DEBUG('e', "Write syscall.\n");
                Write_Syscall(machine->ReadRegister(4),
                        machine->ReadRegister(5),
                        machine->ReadRegister(6));
                break;
            case SC_Read:
                DEBUG('e', "Read syscall.\n");
                rv = Read_Syscall(machine->ReadRegister(4),
                        machine->ReadRegister(5),
                        machine->ReadRegister(6));
                break;
            case SC_Close:
                DEBUG('e', "Close syscall.\n");
                Close_Syscall(machine->ReadRegister(4));
                break;
            case SC_PrintF:
                DEBUG('e', "Print_F system call.\n");
                sc_printf(machine->ReadRegister(4),
                            machine->ReadRegister(5));
                break;
            case SC_PrintL:
                sc_printl(machine->ReadRegister(4));
                break;
        }

        // Put in the return value and increment the PC
        machine->WriteRegister(2,rv);
        machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
        machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
        machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
        return;
    }else {
        cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
        printf("[%d]\n", machine->ReadRegister(BadVAddrReg));
        interrupt->Halt();
    }
}
