// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
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
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {
	name = debugName;
	ownerThread = NULL;
	isBusy = false;
}
Lock::~Lock() {}
bool isHeldByCurrentThread(){
	return ownerThread == currentThread;
}
void Lock::Acquire() {
	int oldLevel = interrupt->SetLevel(IntOff);
	//if the current thread holds the lock we do not need ot continue
	if (isHeldByCurrentThread()){
		interrupt->SetLevel(oldLevel);
		return;
	}
	//if the lock is not busy, we make it busy and make the current thread the owner
	if (isBusy){
		isBusy = true;
		ownerThread = currentThread;
	}
	else{ //the lock is busy in which case we put the thread into the wait queue and put it to sleep 
		//TODO: add to wait queue
		currentThread->Sleep();	
	}
	interrupt->SetLevel(oldLevel);}
void Lock::Release() {
	int oldLevel = interrupt->SetLevel(IntOff);
	//if we do not own the lock we cannot release it
	if(!isHeldByCurrentThread()){
		//Error message
		printf("Error: currentThread is not Lock owner. Cannot release lock");
		//restore interrupts
		interrupt->SetLevel(oldLevel);
		return;
	}

	if(/*the wait queue is not empty*/){
		//Remove thread from the wait queue
		//place thread in ready queue
		//make that thread the lock owner
	}
	else{//the wait queue is empty
		//free the lock
		isBusy = false;
		//clear lock ownership
		ownerThread = NULL;
	}
	//restore interrupts
	interrupt->SetLevel(oldLevel);
	return;
}

Condition::Condition(char* debugName) { }
Condition::~Condition() { }
void Condition::Wait(Lock* conditionLock) { 
	//disable interrupts
	int oldLevel = interrupt->SetLevel(IntOff);
	if(conditionLock == NULL){
		//conditionLock is NULL, cannot later reference a NULL lock
		printf("%s\n", "Error: Lock is NULL, cannot reference an NULL pointer");
		//restore interrupts
		interrupt->SetLevel(oldLevel);
		return;
	}

	if(/*waitingLock == NULL*/){
	//waitingLock = conditionLock;
	}

	if(/*waitingLock != NULL*/){
		printf("%s\n", "Error: The Waiting Lock is NULL");
		interrupt->SetLevel(oldLevel);
		return;
	}
	//put self into wait queue
	conditionLock->Release();
	currentThread->sleep();
	conditionLock->Acquire();
	interrupt->SetLevel(oldLevel);
	ASSERT(FALSE); 
	//Not Sure Where to restore
}
void Condition::Signal(Lock* conditionLock) { 
	int oldLevel = interrupt->SetLevel(IntOff);
	if(/*Wait queue is empty*/){
		interrupt->SetLevel(oldLevel);
		return;
	}

	if()
	interrupt->SetLevel(oldLevel);
}
void Condition::Broadcast(Lock* conditionLock) { 
int oldLevel = interrupt->SetLevel(IntOff);}
