/* syscalls.h 
 *  Nachos system call interface.  These are Nachos kernel operations
 *  that can be invoked from user programs, by trapping to the kernel
 *  via the "syscall" instruction.
 *
 *  This file is included by user programs and by the Nachos kernel. 
 *
 * Copyright (c) 1992-1993 The Regents of the University of California.
 * All rights reserved.  See copyright.h for copyright notice and limitation 
 * of liability and disclaimer of warranty provisions.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "copyright.h"

/* system call codes -- used by the stubs to tell the kernel which system call
 * is being asked for
 */
#define SC_Halt                 0
#define SC_Exit                 1
#define SC_Exec                 2
#define SC_Join                 3
#define SC_Create               4
#define SC_Open                 5
#define SC_Read                 6
#define SC_Write                7
#define SC_Close                8
#define SC_Fork                 9
#define SC_Yield                10

/* Lock system calls */
#define SC_LOCK_CREATE          11
#define SC_LOCK_ACQUIRE         12
#define SC_LOCK_RELEASE         13
#define SC_LOCK_DELETE          14

/* Condition system calls */
#define SC_CONDITION_CREATE     15
#define SC_CONDITION_WAIT       16
#define SC_CONDITION_SIGNAL     17
#define SC_CONDITION_BROADCAST  18
#define SC_CONDITION_DELETE     19

#define SC_Print_F              20
#define SC_Sprintf				21

#define MAXFILENAME 256

#ifndef IN_ASM

/* The system call interface.  These are the operations the Nachos
 * kernel needs to support, to be able to run user programs.
 *
 * Each of these is invoked by a user program by simply calling the 
 * procedure; an assembly language stub stuffs the system call code
 * into a register, and traps to the kernel.  The kernel procedures
 * are then invoked in the Nachos kernel, after appropriate error checking, 
 * from the system call entry point in exception.cc.
 */

/* Stop Nachos, and print out performance stats */
void Halt();        
 

/* Address space control operations: Exit, Exec, and Join */

/* This user program is done (status = 0 means exited normally). */
void Exit(int status);  

/* A unique identifier for an executing user program (address space) */
typedef int SpaceId;    
 
/* Run the executable, stored in the Nachos file "name", and return the 
 * address space identifier
 */
SpaceId Exec(char *name ,int size);
 
/* Only return once the the user program "id" has finished.  
 * Return the exit status.
 */
int Join(SpaceId id);   
 

/* File system operations: Create, Open, Read, Write, Close
 * These functions are patterned after UNIX -- files represent
 * both files *and* hardware I/O devices.
 *
 * If this assignment is done before doing the file system assignment,
 * note that the Nachos file system has a stub implementation, which
 * will work for the purposes of testing out these routines.
 */
 
/* A unique identifier for an open Nachos file. */
typedef int OpenFileId; 

/* when an address space starts up, it has two open files, representing 
 * keyboard input and display output (in UNIX terms, stdin and stdout).
 * Read and Write can be used directly on these, without first opening
 * the console device.
 */

#define ConsoleInput    0  
#define ConsoleOutput   1  
 
/* Create a Nachos file, with "name" */
void Create(char *name, int size);

/* Open the Nachos file "name", and return an "OpenFileId" that can 
 * be used to read and write to the file.
 */
OpenFileId Open(char *name, int size);

/* Write "size" bytes from "buffer" to the open file. */
void Write(char *buffer, int size, OpenFileId id);

/* Read "size" bytes from the open file into "buffer".  
 * Return the number of bytes actually read -- if the open file isn't
 * long enough, or if it is an I/O device, and there aren't enough 
 * characters to read, return whatever is available (for I/O devices, 
 * you should always wait until you can return at least one character).
 */
int Read(char *buffer, int size, OpenFileId id);

/* Close the file, we're done reading and writing to it. */
void Close(OpenFileId id);


/*******************************************************************************************\
| THREAD OPERATIONS:                                                                        |
|   user-level thread operations: Fork, Yield, Finish.                                      |
|   Allows multiple threads to operate within a user program.                               |
\*******************************************************************************************/

/**
 * Fork a thread to run a procedure (function) in the same address space 
 * as the calling thread. If the system is unable to fork a new thread,
 * returns a value < 0, Otherwise returns 0.
 *
 * @param *func                 function to execute
 * @param name                  name of the thread
 * @param name_buffer_size      size of name buffer (not null terminated). 
 */
int Fork(void (*func), char* name, int name_buffer_size);

/* Yield the CPU to another runnable thread, whether in this address space 
 * or not. 
 */
void Yield();



/*******************************************************************************************\
| LOCK OPERATIONS:                                                                          |
|   user-level lock operations: Create, Acquire, Release, Delete.                           |
\*******************************************************************************************/

/**
 * Create a new lock. If the system is unable to allocate a new lock, this
 * function will return -1. On success it will return an index to the newly created lock.
 *
 * @param name_buffer           name of the lock.
 * @param name_buffer_size      size of name buffer (not null terminated).
 *
 * @return int - index of lock or -1 on failure
 */
int Lock_Create(char* name_buffer, int name_buffer_size);

/**
 * Acquire a lock
 *
 * @param lock_index    index of lock
 *
 * @return int - 0 on success -1 otherwise
 */
int Lock_Acquire(int lock_index);

/**
 * Acquire a lock
 *
 * @param lock_index    index of lock
 *
 * @return int - 0 on success -1 otherwise
 */
int Lock_Release(int lock_index);

/**
 * Delete a lock. If the lock is in use, marks the lock to be deleted at a
 * later time. Returns 0 if the lock has been deleted or scheduled for delete.
 *
 * @param lock_index    index of lock
 *
 * @return int - 0 on success -1 otherwise
 */
int Lock_Delete(int lock_index);


/*******************************************************************************************\
| CONDITION OPERATIONS:                                                                     |
|   user-level condition operations: Create, Wait, Signal, Broadcast, Delete.               |
\*******************************************************************************************/

/**
 * Allocate a new condition. If the system is unable to allocate a new condition, this
 * function will return -1. On success it will return an index to the newly created condition.
 *
 * @param name_buffer           name of the condition.
 * @param size_of_name_buffer   size of name buffer (not null terminated).
 *
 * @return int - index of condition or -1 on failure
 */
int Condition_Create(char* name_buffer, int size_of_name_buffer);

/**
 * Wait on a condition.
 *
 * @param condition_index   index of condition
 * @param lock_index        index of lock
 *
 * @return int - 0 on success -1 otherwise
 */
int Condition_Wait(int condition_index, int lock_index);

/**
 * Signal on a condition.
 *
 * @param condition_index   index of condition
 * @param lock_index        index of lock
 *
 * @return int - 0 on success -1 otherwise
 */
int Condition_Signal(int condition_index, int lock_index);

/**
 * Broadcast on a condition.
 *
 * @param condition_index   index of condition
 * @param lock_index        index of lock
 *
 * @return int - 0 on success -1 otherwise
 */
int Condition_Broadcast(int condition_index, int lock_index);

/**
 * Delete a condition. If the lock is in use, marks the condition to be deleted at a
 * later time. Returns 0 if the condition has been deleted or scheduled for delete.
 *
 * @param condition_index   index of condition
 *
 * @return int - 0 on success -1 otherwise
 */
int Condition_Delete(int condition_index);

void Print_F(char* buf, int size);

/*----------Sprintf-----------------
mychar is the output char*
text will be use to determine the formatting Ex: "Today is the %d\n"
i is the number that will replace %d in text
------------------------------------*/

void Sprint_f(char* mychar, char* text, int i);

#endif /* IN_ASM */

#endif /* SYSCALL_H */
