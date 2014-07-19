// synch.cc 
//  Routines for synchronizing threads.  Three kinds of
//  synchronization routines are defined here: semaphores, locks 
//      and condition variables (the implementation of the last two
//  are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
//  Initialize a semaphore, so that it can be used for synchronization.
//
//  "debugName" is an arbitrary name, useful for debugging.
//  "initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue) {
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
//  De-allocate semaphore, when no longer needed.  Assume no one
//  is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore() {
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
//  Wait until semaphore value > 0, then decrement.  Checking the
//  value and decrementing must be done atomically, so we
//  need to disable interrupts before checking the value.
//
//  Note that Thread::Sleep assumes that interrupts are disabled
//  when it is called.
//----------------------------------------------------------------------

void Semaphore::P() {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    
    while (value == 0) {            // semaphore not available
    queue->Append((void *)currentThread);   // so go to sleep
    currentThread->Sleep();
    } 
    value--;                    // semaphore available, 
                        // consume its value
    
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
//  Increment semaphore value, waking up a waiter if necessary.
//  As with P(), this operation must be atomic, so we need to disable
//  interrupts.  Scheduler::ReadyToRun() assumes that threads
//  are disabled when it is called.
//----------------------------------------------------------------------

void Semaphore::V() {
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)    // make thread ready, consuming the V immediately
    scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {
    this->name = debugName;
    this->ownerThread = NULL;
    this->isBusy = false;
    this->waitQueue = new List;
    this->entrant_count = 0;
}


Lock::~Lock() {
    delete waitQueue;
}

/**
 * Checks if the current thread owns the lock.
 */
bool Lock::isHeldByCurrentThread() {
    bool return_value;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    return_value = this->ownerThread == currentThread;
    (void) interrupt->SetLevel(oldLevel);
    return return_value;
}


void Lock::Acquire() {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    //printLockDetails();
    if(isHeldByCurrentThread()) {
        // the owning thread has reacquired the lock.
        //printf("[%s] is held by current thread [%s]\n",name, currentThread->getName());
        entrant_count++;
        (void) interrupt->SetLevel(oldLevel);
        return;
    }
    
    //if the lock is not busy, we make it busy and make the current thread the owner
    if(isBusy == false) {
        isBusy = true;
        ownerThread = currentThread;
        entrant_count++;
        //printf("[%s] is acquired by new thread [%s]\n",name, currentThread->getName());
    }
    else { 
        // the lock is busy. Put the calling thread into the wait queue and sleep 
        waitQueue->Append((void *)currentThread);
        //printf("[%s] is busy, thread [%s] is waiting\n",name, currentThread->getName());
        currentThread->Sleep();
    }

    (void) interrupt->SetLevel(oldLevel);
}


bool Lock::Release() {
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    // the currentThread must own the lock to release is
    if(!isHeldByCurrentThread()) {
        printf("Error: currentThread %s is not Lock owner [NULL]. Cannot release lock [%s]\n", 
            currentThread->getName(),
            name);

        // This condition should NEVER happen. Go back and look at
        // the code that generated this. There's a race condition
        // somewhere and it needs to be found.
        ASSERT(FALSE);
        
        (void) interrupt->SetLevel(oldLevel);
        return false;
    }

    entrant_count--;

    // this is a reentrant lock. The entrant count must
    // be 0 before we can specify the lock as available again.
    if(entrant_count == 0) {
        //printf("[%s] is being released by [%s]\n", name, ownerThread->getName());
        if(waitQueue->IsEmpty()) {
            // there are no threads waiting on the lock.
            // set the the lock to be available and return.
            isBusy = false;
            ownerThread = NULL;
        }else {
            // wake up a thread waiting for the lock. Switch
            // the lock ownership to the new thread, increment
            // the entrant_count to reflect 1 acquire, and return.
            thread = (Thread *)waitQueue->Remove();
            ownerThread = thread;
            entrant_count = 1;
            scheduler->ReadyToRun(thread);
        }
    }

    //restore interrupts
    (void) interrupt->SetLevel(oldLevel);
    return true;
}

void Lock::printLockDetails() {
    if(ownerThread == NULL) {
        printf("[%s] Lock unowned\n", name);
    }else {
        printf("[%s] Lock Owner:\t [%s]\n",name, ownerThread->getName());
    }
    waitQueue->Mapcar((VoidFunctionPtr) ThreadPrint);
    printf("\n");
}


Condition::Condition(char* debugName) {
    name = debugName;
    waitQueue = new List;
    waitLock = NULL;
}


Condition::~Condition() { 
    delete waitQueue;
}


void Condition::Wait(Lock* conditionLock) { 
    //disable interrupts
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    if(conditionLock == NULL){
        //conditionLock is NULL, cannot later reference a NULL lock
        printf("%s\n", "Error: Lock is NULL, cannot reference an NULL pointer");
        //restore interrupts
        (void) interrupt->SetLevel(oldLevel);
        return;
    }

    if(waitLock == NULL) {
        waitLock = conditionLock;
    }

    if(waitLock != conditionLock) {
        printf("%s\n", "Error: The Waiting Lock and the Condition Lock do not match.");
        (void) interrupt->SetLevel(oldLevel);
        return;
    }
    
    waitQueue->Append((void *)currentThread);
    conditionLock->Release();
    currentThread->Sleep();
    
    // acquire lock so that another thread doesn't enter a critical section where wait is called
    conditionLock->Acquire();
    (void) interrupt->SetLevel(oldLevel);
}


void Condition::Signal(Lock* conditionLock) { 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    if(waitQueue->IsEmpty()) {
        (void) interrupt->SetLevel(oldLevel);
        return;
    }

    if(waitLock != conditionLock) {
        printf("%s\n", "Error: The Waiting Lock and the Condition Lock do not match.");
        (void) interrupt->SetLevel(oldLevel);
        return;
    }
    
    Thread *thread = (Thread*)waitQueue->Remove();
    scheduler->ReadyToRun(thread);
    
    if(waitQueue->IsEmpty()) {
        waitLock = NULL;
    }
    (void) interrupt->SetLevel(oldLevel);
}

void Condition::Broadcast(Lock* conditionLock) { 
    while(!waitQueue->IsEmpty()){
        Condition::Signal(conditionLock);
    }
}
