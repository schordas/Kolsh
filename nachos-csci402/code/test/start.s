/* Start.s 
 *  Assembly language assist for user programs running on top of Nachos.
 *
 *  Since we don't want to pull in the entire C library, we define
 *  what we need for a user program here, namely Start and the system
 *  calls.
 */

#define IN_ASM
#include "syscall.h"

        .text   
        .align  2

/* -------------------------------------------------------------
 * __start
 *  Initialize running a C program, by calling "main". 
 *
 *  NOTE: This has to be first, so that it gets loaded at location 0.
 *  The Nachos kernel always starts a program by jumping to location 0.
 * -------------------------------------------------------------
 */

    .globl __start
    .ent    __start
__start:
    jal main
    move    $4,$0       
    jal Exit     /* if we return from main, exit(0) */
    .end __start

/* -------------------------------------------------------------
 * System call stubs:
 *  Assembly language assist to make system calls to the Nachos kernel.
 *  There is one stub per system call, that places the code for the
 *  system call into register r2, and leaves the arguments to the
 *  system call alone (in other words, arg1 is in r4, arg2 is 
 *  in r5, arg3 is in r6, arg4 is in r7)
 *
 *  The return value is in r2. This follows the standard C calling
 *  convention on the MIPS.
 * -------------------------------------------------------------
 */

    .globl  Halt
    .ent    Halt
Halt:
    addiu $2,$0,SC_Halt
    syscall
    j   $31
    .end Halt

    .globl  Exit
    .ent    Exit
Exit:
    addiu $2,$0,SC_Exit
    syscall
    j   $31
    .end Exit

    .globl  Exec
    .ent    Exec
Exec:
    addiu $2,$0,SC_Exec
    syscall
    j   $31
    .end Exec

    .globl  Join
    .ent    Join
Join:
    addiu $2,$0,SC_Join
    syscall
    j   $31
    .end Join

    .globl  Create
    .ent    Create
Create:
    addiu $2,$0,SC_Create
    syscall
    j   $31
    .end Create

    .globl  Open
    .ent    Open
Open:
    addiu $2,$0,SC_Open
    syscall
    j   $31
    .end Open

    .globl  Read
    .ent    Read
Read:
    addiu $2,$0,SC_Read
    syscall
    j   $31
    .end Read

    .globl  Write
    .ent    Write
Write:
    addiu $2,$0,SC_Write
    syscall
    j   $31
    .end Write

    .globl  Close
    .ent    Close
Close:
    addiu $2,$0,SC_Close
    syscall
    j   $31
    .end Close

    .globl  Fork
    .ent    Fork
Fork:
    addiu $2,$0,SC_Fork
    syscall
    j   $31
    .end Fork

    .globl  Yield
    .ent    Yield
Yield:
	addiu $2,$0,SC_Yield
	syscall
	j	$31
	.end Yield

    .globl  Lock_Create
    .ent    Lock_Create
Lock_Create:
    addiu $2,$0,SC_LOCK_CREATE
    syscall
    j   $31
    .end Lock_Create
    
    .globl  Lock_Acquire
    .ent    Lock_Acquire
Lock_Acquire:
    addiu $2,$0,SC_LOCK_ACQUIRE
    syscall
    j   $31
    .end Lock_Acquire

    .globl  Lock_Release
    .ent    Lock_Release
Lock_Release:
    addiu $2,$0,SC_LOCK_RELEASE
    syscall
    j   $31
    .end Lock_Release

    .globl  Lock_Delete
    .ent    Lock_Delete
Lock_Delete:
    addiu $2,$0,SC_LOCK_DELETE
    syscall
    j   $31
    .end Lock_Delete

    .globl  Condition_Create
    .ent    Condition_Create
Condition_Create:
    addiu $2,$0,SC_CONDITION_CREATE
    syscall
    j   $31
    .end Condition_Create

    .globl  Condition_Wait
    .ent    Condition_Wait
Condition_Wait:
    addiu $2,$0,SC_CONDITION_WAIT
    syscall
    j   $31
    .end Condition_Wait

    .globl  Condition_Signal
    .ent    Condition_Signal
Condition_Signal:
    addiu $2,$0,SC_CONDITION_SIGNAL
    syscall
    j   $31
    .end Condition_Signal

    .globl  Condition_Broadcast
    .ent    Condition_Broadcast
Condition_Broadcast:
    addiu $2,$0,SC_CONDITION_BROADCAST
    syscall
    j   $31
    .end Condition_Broadcast

    .globl  Condition_Delete
    .ent    Condition_Delete
Condition_Delete:
    addiu $2,$0,SC_CONDITION_DELETE
    syscall
    j   $31
    .end Condition_Delete

    .globl  Sprint_f 
    .ent    Sprint_f
Sprint_f:
    addiu $2,$0,SC_Sprintf
    syscall
    j   $31
    .end Sprint_f 
    
    .globl  Print_F 
    .ent    Print_F
Print_F:
    addiu $2,$0,SC_Print_F
    syscall
    j   $31
    .end Print_F
/* dummy function to keep gcc happy */
        .globl  __main
        .ent    __main
__main:
        j       $31
        .end    __main

