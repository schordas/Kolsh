// threadtest.cc 
//  Simple test case for the threads assignment.
//
//  Create two threads, and have them context switch
//  back and forth between themselves by calling Thread::Yield, 
//  to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.


//test
#include "copyright.h"
#include "system.h"
//----------------------------------------------------------------------
// SimpleThread
//  Loop 5 times, yielding the CPU to another ready thread 
//  each iteration.
//
//  "which" is simply a number identifying the thread, for debugging
//  purposes.
//----------------------------------------------------------------------

void SimpleThread(int which) {
    int num;
    
    for (num = 0; num < 5; num++) {
    printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
//  Set up a ping-pong between two threads, by forking a thread 
//  to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void ThreadTest() {
        return;
    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}


//  Simple test cases for the threads assignment.
//


#include "copyright.h"
#include "system.h"
#ifdef CHANGED
#include "synch.h"
#endif

#ifdef CHANGED
// --------------------------------------------------
// Test Suite
// --------------------------------------------------


// --------------------------------------------------
// Test 1 - see TestSuite() for details
// --------------------------------------------------
Semaphore t1_s1("t1_s1",0);       // To make sure t1_t1 acquires the lock before t1_t2
Semaphore t1_s2("t1_s2",0);       // To make sure t1_t2 Is waiting on the lock before t1_t3 releases it
Semaphore t1_s3("t1_s3",0);       // To make sure t1_t1 does not release the lock before t1_t3 tries to acquire it
Semaphore t1_done("t1_done",0);   // So that TestSuite knows when Test 1 is done
Lock t1_l1("t1_l1");              // the lock tested in Test 1

// --------------------------------------------------
// t1_t1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------
void t1_t1() {
    t1_l1.Acquire();
    t1_s1.V();  // Allow t1_t2 to try to Acquire Lock
 
    printf ("%s: Acquired Lock %s, waiting for t3\n",currentThread->getName(),
        t1_l1.getName());
    t1_s3.P();
    printf ("%s: working in CS\n",currentThread->getName());
    for (int i = 0; i < 1000000; i++) ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
        t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------
void t1_t2() {

    t1_s1.P();  // Wait until t1 has the lock
    t1_s2.V();  // Let t3 try to acquire the lock

    printf("%s: trying to acquire lock %s\n",currentThread->getName(),
        t1_l1.getName());
    t1_l1.Acquire();

    printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),
        t1_l1.getName());
    for (int i = 0; i < 10; i++)
    ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
        t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------
void t1_t3() {

    t1_s2.P();  // Wait until t2 is ready to try to acquire the lock

    t1_s3.V();  // Let t1 do it's stuff
    for ( int i = 0; i < 3; i++ ) {
    printf("%s: Trying to release Lock %s\n",currentThread->getName(),
           t1_l1.getName());
    t1_l1.Release();
    }
}

// --------------------------------------------------
// Test 2 - see TestSuite() for details
// --------------------------------------------------
Lock t2_l1("t2_l1");        // For mutual exclusion
Condition t2_c1("t2_c1");   // The condition variable to test
Semaphore t2_s1("t2_s1",0); // To ensure the Signal comes before the wait
Semaphore t2_done("t2_done",0);     // So that TestSuite knows when Test 2 is
                                  // done

// --------------------------------------------------
// t2_t1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------
void t2_t1() {
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
       t2_l1.getName(), t2_c1.getName());
    t2_c1.Signal(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
       t2_l1.getName());
    t2_l1.Release();
    t2_s1.V();  // release t2_t2
    t2_done.V();
}

// --------------------------------------------------
// t2_t2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------
void t2_t2() {
    t2_s1.P();  // Wait for t2_t1 to be done with the lock
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
       t2_l1.getName(), t2_c1.getName());
    t2_c1.Wait(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
       t2_l1.getName());
    t2_l1.Release();
}
// --------------------------------------------------
// Test 3 - see TestSuite() for details
// --------------------------------------------------
Lock t3_l1("t3_l1");        // For mutual exclusion
Condition t3_c1("t3_c1");   // The condition variable to test
Semaphore t3_s1("t3_s1",0); // To ensure the Signal comes before the wait
Semaphore t3_done("t3_done",0); // So that TestSuite knows when Test 3 is
                                // done

// --------------------------------------------------
// t3_waiter()
//     These threads will wait on the t3_c1 condition variable.  Only
//     one t3_waiter will be released
// --------------------------------------------------
void t3_waiter() {
    t3_l1.Acquire();
    t3_s1.V();      // Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
       t3_l1.getName(), t3_c1.getName());
    t3_c1.Wait(&t3_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t3_c1.getName());
    t3_l1.Release();
    t3_done.V();
}


// --------------------------------------------------
// t3_signaller()
//     This threads will signal the t3_c1 condition variable.  Only
//     one t3_signaller will be released
// --------------------------------------------------
void t3_signaller() {

    // Don't signal until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
    t3_s1.P();
    t3_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
       t3_l1.getName(), t3_c1.getName());
    t3_c1.Signal(&t3_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t3_l1.getName());
    t3_l1.Release();
    t3_done.V();
}
 
// --------------------------------------------------
// Test 4 - see TestSuite() for details
// --------------------------------------------------
Lock t4_l1("t4_l1");        // For mutual exclusion
Condition t4_c1("t4_c1");   // The condition variable to test
Semaphore t4_s1("t4_s1",0); // To ensure the Signal comes before the wait
Semaphore t4_done("t4_done",0); // So that TestSuite knows when Test 4 is
                                // done

// --------------------------------------------------
// t4_waiter()
//     These threads will wait on the t4_c1 condition variable.  All
//     t4_waiters will be released
// --------------------------------------------------
void t4_waiter() {
    t4_l1.Acquire();
    t4_s1.V();      // Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
       t4_l1.getName(), t4_c1.getName());
    t4_c1.Wait(&t4_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t4_c1.getName());
    t4_l1.Release();
    t4_done.V();
}


// --------------------------------------------------
// t2_signaller()
//     This thread will broadcast to the t4_c1 condition variable.
//     All t4_waiters will be released
// --------------------------------------------------
void t4_signaller() {

    // Don't broadcast until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
    t4_s1.P();
    t4_l1.Acquire();
    printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),
       t4_l1.getName(), t4_c1.getName());
    t4_c1.Broadcast(&t4_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t4_l1.getName());
    t4_l1.Release();
    t4_done.V();
}
// --------------------------------------------------
// Test 5 - see TestSuite() for details
// --------------------------------------------------
Lock t5_l1("t5_l1");        // For mutual exclusion
Lock t5_l2("t5_l2");        // Second lock for the bad behavior
Condition t5_c1("t5_c1");   // The condition variable to test
Semaphore t5_s1("t5_s1",0); // To make sure t5_t2 acquires the lock after
                                // t5_t1

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a condition under t5_l1
// --------------------------------------------------
void t5_t1() {
    t5_l1.Acquire();
    t5_s1.V();  // release t5_t2
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
       t5_l1.getName(), t5_c1.getName());
    t5_c1.Wait(&t5_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
       t5_l1.getName());
    t5_l1.Release();
}

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a t5_c1 condition under t5_l2, which is
//     a Fatal error
// --------------------------------------------------
void t5_t2() {
    t5_s1.P();  // Wait for t5_t1 to get into the monitor
    t5_l1.Acquire();
    t5_l2.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
       t5_l2.getName(), t5_c1.getName());
    t5_c1.Signal(&t5_l2);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
       t5_l2.getName());
    t5_l2.Release();
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
       t5_l1.getName());
    t5_l1.Release();
}

#include "synch.h"

using namespace std;

#define PATIENTS_COUNT 10
#define RECEPTIONISTS_COUNT 5
#define DOCTORS_COUNT 3
#define DOORBOYS_COUNT 5
#define CASHIERS_COUNT 3
#define CLERKS_COUNT 3

Lock recLineLock("recLineLock");
Lock* recLock[RECEPTIONISTS_COUNT];
Lock tokenLock("tokenLock");
Condition* recLineCV[RECEPTIONISTS_COUNT];
Condition* recCV[RECEPTIONISTS_COUNT];
int recToken[RECEPTIONISTS_COUNT];
int recLineCount[RECEPTIONISTS_COUNT];
int recState[RECEPTIONISTS_COUNT]; // 0 = available, 1 = busy, 2 = on break
//Patient
int myToken[PATIENTS_COUNT];
int nextToken;
int doctor_to_visit[PATIENTS_COUNT];
int patient_illness[PATIENTS_COUNT];
char* patient_medicine[PATIENTS_COUNT];
int patient_phar_bill[PATIENTS_COUNT];
int patient_money_spent_at_cashier[PATIENTS_COUNT];
//DoorBoy
Lock* doorboyLock[DOORBOYS_COUNT];
Lock doctorWaitLock("doctorWaitLock");
Condition* doorboyCV[DOORBOYS_COUNT];
Condition doctorWaitLine("doctorWaitLine");
Lock* door_boy_break_lock[DOORBOYS_COUNT];
Condition* door_boy_break_CV[DOORBOYS_COUNT];
int doorboyLineCount;
int doorboyState[DOORBOYS_COUNT]; // 0 = available, 1 = busy, 2 = on break
int random_doctor_index;
//Doctor
Lock* doctorLock[DOCTORS_COUNT];
Condition* doctorCV[DOCTORS_COUNT];
Lock* doctorToDoorboyLock[DOCTORS_COUNT];
Condition* doctorToDoorboyCV[DOCTORS_COUNT];
int patientFee[PATIENTS_COUNT];
int current_patient_index[DOCTORS_COUNT];
int doctor_prescription[DOCTORS_COUNT];

//Cashier
Lock* cashierLock[CASHIERS_COUNT];
Condition* cashierCV[CASHIERS_COUNT];
Lock cashier_Line_Lock("cashier_Line_Lock");
Condition* cashier_Line_CV[CASHIERS_COUNT];
Lock* cashier_break_lock[CASHIERS_COUNT];
Condition* cashier_break_CV[CASHIERS_COUNT];
int cashier_state[CASHIERS_COUNT];
int cashier_line[CASHIERS_COUNT];
int patient_index_for_cashier[CASHIERS_COUNT];
int cashier_consulting_fee[CASHIERS_COUNT];
int totalIncome; //Hospital's income from patients

//Pharmacy_Clerk
Lock* phar_clerk_Lock[CLERKS_COUNT];
Condition* phar_clerk_CV[CLERKS_COUNT];
Lock phar_clerk_line_Lock("phar_clerk_line_Lock");
Condition* phar_clerk_line_CV[CLERKS_COUNT];
Lock* phar_clerk_break_Lock[CLERKS_COUNT];
Condition* phar_clerk_break_CV[CLERKS_COUNT];
int phar_clerk_line[CLERKS_COUNT];
int phar_clerk_state[CLERKS_COUNT];
int Patient_bill[PATIENTS_COUNT];
int patient_index_to_clerk[CLERKS_COUNT];
char* medicine[CLERKS_COUNT];
int medicine_cost[CLERKS_COUNT];
int phar_income;

void patient(int index){
	char debug_name[20];
	//printf("Patient [%u]:\n" ,index );
	sprintf(debug_name, "Patient line lock # %d", index);
	//printf("%s \n",debug_name);
	//Acquire Lock for waiting in line
	recLineLock.Acquire();
		printf("Patient [%u]: Acquired %s\n" ,index, recLineLock.getName() );
	//Find the shortest line, if no line, go to receptionist and if there are lines, go to the shortest line
	int shortest = recLineCount[0], line_index;
	for(int i = 0; i < RECEPTIONISTS_COUNT; i++){
		//index the shortest line and update shortest count
		if(recLineCount[i] < shortest){
			line_index = i;
			shortest = recLineCount[i];
		}
		//If Receptionist is available
		if(recState[i] == 0){
			printf("\t Found an available receptionist [%d]\n", i);
			line_index = i;
			recState[i] = 1; //make him busy
			shortest = -1;
			break;	
		}
	}
	//If all receptionist are busy, wait in line
	if(shortest > -1){
		printf("\t shortest : %d, larger than -1, increment and wait\n", shortest);
		recLineCount[line_index]++;
		recLineCV[line_index]->Wait(&recLineLock);
		recLineCount[line_index]--;
	}
	recLineLock.Release();
		printf("Patient [%u]: Released %s\n" ,index, recLineLock.getName() );
	recLock[line_index]->Acquire();
		printf("Patient [%u]: Acquired %s\n" ,index, recLock[line_index]->getName() );
	recCV[line_index]->Signal(recLock[line_index]);
	recCV[line_index]->Wait(recLock[line_index]);
	myToken[index] = recToken[line_index];
		printf("Patient [%u]: Acquired token: %d\n" ,index, myToken[index] );
	recCV[line_index]->Signal(recLock[line_index]);
	recLock[line_index]->Release();
	//Now patient acquired a unique token number [myToken], going to stand in line to meet doctor
	//Doorboy
	doctorWaitLock.Acquire();
	//Pick a random doctor
	random_doctor_index = rand()%DOCTORS_COUNT;
		printf("Random Doctor index: %u\n", random_doctor_index);
	doctor_to_visit[index] = random_doctor_index;
	//Signal the door boy
			printf("Patient [%u]: signal %s\n" ,index, doctorWaitLine.getName() );
	doctorWaitLine.Signal(&doctorWaitLock);
	doorboyLineCount++;
	doctorWaitLine.Wait(&doctorWaitLock);
		printf("Patient [%u]: Got signalled by %s\n" ,index, doctorWaitLine.getName() );
	doorboyLineCount--;
	doctorWaitLock.Release();

	//Wait for doorboy to signal, then go straight to doctor
	doctorLock[doctor_to_visit[index]]->Acquire();
			printf("Patient [%u]: signal %s\n" ,index, doctorCV[doctor_to_visit[index]]->getName() );
	doctorCV[doctor_to_visit[index]]->Signal(doctorLock[doctor_to_visit[index]]);
	//patient's index for the doctor
	current_patient_index[doctor_to_visit[index]] = index;
	doctorCV[doctor_to_visit[index]]->Wait(doctorLock[doctor_to_visit[index]]);
		printf("Patient [%u]: Got signalled by %s\n" ,index, doctorCV[doctor_to_visit[index]]->getName() );
	//Doctor finished examining, get a prescription
	patient_illness[index] = doctor_prescription[doctor_to_visit[index]];
	
	//Signal the doctor so he know I am leaving
			printf("Patient [%u]: signal %s\n" ,index, doctorCV[doctor_to_visit[index]]->getName() );
	doctorCV[doctor_to_visit[index]]->Signal(doctorLock[doctor_to_visit[index]]);
	doctorLock[doctor_to_visit[index]]->Release();
	
	//go to the cashier, find the shortest line
	cashier_Line_Lock.Acquire();
	shortest = cashier_line[0];
	for(int i = 0; i < CASHIERS_COUNT; i++){
		//Find the shortest line and index to it
		if(cashier_line[i] < shortest){
			shortest = cashier_line[i];
			line_index = i;
		}
		//If cashier is available
		if(cashier_state[i] == 0){
			line_index = i;		
			cashier_state[line_index] = 1; //Make him busy
			shortest = -1;
			break;
		}
	}
	if(shortest > -1){
		cashier_line[line_index]++;
		cashier_Line_CV[line_index]->Wait(&cashier_Line_Lock);
		cashier_line[line_index]--;
	}
	cashier_Line_Lock.Release();
	//Passed the line, Going to see cashier
	cashierLock[line_index]->Acquire();
	//Store the patient index for cashier use
	patient_index_for_cashier[line_index] = index;
	cashierCV[line_index]->Signal(cashierLock[line_index]);
	cashierCV[line_index]->Wait(cashierLock[line_index]);
	//paid the cashier
	patient_money_spent_at_cashier[index] = cashier_consulting_fee[line_index];
	cashierCV[line_index]->Signal(cashierLock[line_index]);
	cashierCV[line_index]->Wait(cashierLock[line_index]);
	//Cashier collected the money, leave now
	cashierLock[line_index]->Release();
	
	
	//now go to pharmacy clerk, find the shortest line
	phar_clerk_line_Lock.Acquire();
	shortest = phar_clerk_line[0];
	for(int i = 0; i < CLERKS_COUNT; i++){
		//find the shortest line
		if(phar_clerk_line[i] < shortest){
			shortest = phar_clerk_line[i];
			line_index = i;
		}
		//If clerk is available
		if(phar_clerk_state[i] == 0){
			line_index = i;
			phar_clerk_state[i] = 1;
			shortest = -1;
			break;
		}
	}
	if(shortest > -1){
		phar_clerk_line[line_index]++;
		phar_clerk_line_CV[line_index]->Wait(&phar_clerk_line_Lock);
		phar_clerk_line[line_index]--;
	}
	phar_clerk_line_Lock.Release();
	//Line passed
	phar_clerk_Lock[line_index]->Acquire();
	patient_index_to_clerk[line_index] = index;
	phar_clerk_CV[line_index]->Signal(phar_clerk_Lock[line_index]);
	phar_clerk_CV[line_index]->Wait(phar_clerk_Lock[line_index]);
	//Get the medicine, and pay the cost; signal the clerk
	patient_medicine[index] = medicine[line_index];
	patient_phar_bill[index] = medicine_cost[line_index];
	//Signal the clerk to pay
	phar_clerk_CV[line_index]->Signal(phar_clerk_Lock[line_index]);
	phar_clerk_CV[line_index]->Wait(phar_clerk_Lock[line_index]);

	//Tell the clerk I am leaving	
	phar_clerk_CV[line_index]->Signal(phar_clerk_Lock[line_index]);
	phar_clerk_Lock[line_index]->Release();
	
	//Leave hospital
	
}


void receptionist(int index){
	char* debug_name;
	//Assign pointers to new variables
	debug_name = new char[20];
	sprintf(debug_name, "Receptionist_LineCV #%d", index);
	recLineCV[index] = new Condition(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "recLock #%d", index);
	recLock[index] = new Lock(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "recCV #%d", index);
	recCV[index] = new Condition(debug_name);
	
	recLineCount[index] = 0;
	while(true){
		printf("\nReceptionist [%u]\n", index);
		recLineLock.Acquire();
		printf("receptionist [%u]: Acquired %s\n" ,index, recLineLock.getName() );
		recState[index] = 0;
		printf("recLineCount : %d\n", recLineCount[index]);
		if(recLineCount[index] > 0){
			//People waiting in this receptionist's line
			printf("receptionist [%u]: Signals %s\n", index, recLineCV[index]->getName());
			recLineCV[index]->Signal(&recLineLock); //Signal those patient waiting in line
			recState[index] = 1;	//Make me busy
		}
		recLock[index]->Acquire(); //Receptionist enter the lock first
			printf("receptionist [%u]: Acquired %s and going to release %s\n", index, recLock[index]->getName()
			, recLineLock.getName() );
		recLineLock.Release();
		//Wait for patient to ask for token
		recCV[index]->Wait(recLock[index]);
		tokenLock.Acquire();
			printf("receptionist [%u]: Acquired %s\n" ,index, tokenLock.getName() );
		recToken[index] = nextToken;
		nextToken++;
		tokenLock.Release();
		//Signal patient to take the token
		recCV[index]->Signal(recLock[index]);
		printf("receptionist [%u]: Signalled %s and Going to wait\n" ,index, recCV[index]->getName() );
		recCV[index]->Wait(recLock[index]);
		printf("receptionist [%u]: Released %s\n" ,index, recLock[index]->getName());
		recLock[index]->Release();
		//If there are no one in the line, waiting until hospital manager signals if more than 2 in line
		if(recLineCount[index] == 0){
			printf("receptionist [%u]: No one waiting in line, go to break with %s\n" ,index, recCV[index]->getName() );
			recLock[index]->Acquire();
			recState[index] = 2;
			recCV[index]->Wait(recLock[index]);
			
		}
	}


}

void DoorBoy(int index){
	char* debug_name;
	//Assign pointers to new variables
	debug_name = new char[20];
	sprintf(debug_name, "DoorboyCV #%d", index);
	doorboyCV[index] = new Condition(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "DoorboyLock #%d", index);
	doorboyLock[index] = new Lock(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "door_boy_break_lock #%d", index);
	door_boy_break_lock[index] = new Lock(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "door_boy_break_CV #%d", index);
	door_boy_break_CV[index] = new Condition(debug_name);	
	
	while(true){
		printf("\nDoor_Boy [%u]\n", index);
		doctorWaitLock.Acquire();
		doctorWaitLine.Wait(&doctorWaitLock);
			printf("DoorBoy [%u]: Got signalled by %s\n" ,index, doctorWaitLine.getName() );
		doctorToDoorboyLock[random_doctor_index]->Acquire();
		doctorToDoorboyCV[random_doctor_index]->Signal(doctorToDoorboyLock[random_doctor_index]);
		doctorWaitLock.Release();
		doctorToDoorboyCV[random_doctor_index]->Wait(doctorToDoorboyLock[random_doctor_index]);
			printf("DoorBoy [%u]: Got signalled by %s\n" ,index, doctorToDoorboyCV[random_doctor_index]->getName() );
		//Wait for the doctor to signal, so we know he is ready
		doctorWaitLine.Signal(&doctorWaitLock);
		doctorToDoorboyLock[random_doctor_index]->Release();
		if(doorboyLineCount == 0){
			door_boy_break_lock[index]->Acquire();
			door_boy_break_CV[index]->Wait(door_boy_break_lock[index]);
			//Wait for manager to call
		}
	}
}



void Doctor(int index){
	char* debug_name;
	//Assign pointers to new variables
	debug_name = new char[20];
	sprintf(debug_name, "DoctorCV #%d", index);
	doctorCV[index] = new Condition(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "DoctorLock #%d", index);
	doctorLock[index] = new Lock(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "doctorToDoorboyCV #%d", index);
	doctorToDoorboyCV[index] = new Condition(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "doctorToDoorboyLock #%d", index);
	doctorToDoorboyLock[index] = new Lock(debug_name);	
	while(true){
		printf("\nDoctor [%u]\n", index);
		doctorToDoorboyLock[index]->Acquire();
		doctorToDoorboyCV[index]->Wait(doctorToDoorboyLock[index]);
			printf("Doctor [%d]: Got signalled with %s\n", index, doctorToDoorboyCV[index]->getName());
		//A door boy has signalled, signal the door boy to bring in the patient
		doctorToDoorboyCV[index]->Signal(doctorToDoorboyLock[index]);
		doctorToDoorboyLock[index]->Release();
		//Waiting for the patient
		doctorLock[index]->Acquire();
		doctorCV[index]->Wait(doctorLock[index]);
			printf("Doctor [%d]: Got signalled with %s\n", index, doctorCV[index]->getName());

		//Patient now in the room
		int sickChances = rand()%100;
		for(int i = 0; i < 10; i++){
			currentThread->Yield();
		}
		//Set the fee indexed by patient's token
		if(sickChances < 30){
			//Patient is not sick
			//current_patient_index[index] will give current patient's number
			patientFee[myToken[current_patient_index[index]]]  = 25;
			doctor_prescription[index] = 0;
		}
		else if(sickChances >= 30 && sickChances < 50){
			//Common sickness #1  20%
			patientFee[myToken[current_patient_index[index]]] = 250;
			doctor_prescription[index] = 1;
		}	
		else if(sickChances >= 50 && sickChances <80){
			//Common sickness #2  30%
			patientFee[myToken[current_patient_index[index]]]  = 100;
			doctor_prescription[index] = 2;
		}
		else if(sickChances >= 80 && sickChances <90){
			//Common sickness #3  10%
			patientFee[myToken[current_patient_index[index]]]  = 500;
			doctor_prescription[index] = 3;
		}
		else{
			//Common sickness #4  10%
			patientFee[myToken[current_patient_index[index]]]  = 700;
			doctor_prescription[index] = 4;
		}
		//Signal patient for the prescription, and leave
		doctorCV[index]->Signal(doctorLock[index]);
		doctorCV[index]->Wait(doctorLock[index]);
		doctorLock[index]->Release();
		//Doctor decide if he want to break
		int break_chance = rand()% 100;
		if (break_chance > 80){
			//Doctor go to break
			for(int i = 0; i < 5; i++){
				currentThread->Yield();
			}
		}
		
	}
}

void Cashier(int index){
	char* debug_name;
	//Assign pointers to new variables
	debug_name = new char[20];
	sprintf(debug_name, "CashierCV #%d", index);
	cashierCV[index] = new Condition(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "CashierLock #%d", index);
	cashierLock[index] = new Lock(debug_name);

	debug_name = new char[20];
	sprintf(debug_name, "cashier_Line_CV #%d", index);
	cashier_Line_CV[index] = new Condition(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "cashier_break_CV #%d", index);
	cashier_break_CV[index] = new Condition(debug_name);
	
	debug_name = new char[20];
	sprintf(debug_name, "cashier_break_lock #%d", index);
	cashier_break_lock[index] = new Lock(debug_name);
	
	while(true){
		printf("\nCashier [%u]\n", index);
		cashier_Line_Lock.Acquire();
		cashier_state[index] = 0;
		if(cashier_line[index] > 0){
			//People waiting in line
			cashier_Line_CV[index]->Signal(&cashier_Line_Lock);
			cashier_state[index] = 1;
		}
		cashierLock[index]->Acquire();
		cashier_Line_Lock.Release();
		cashierCV[index]->Wait(cashierLock[index]);
		//Patient signalled, send the payment to the patient
		int patient_token = myToken[patient_index_for_cashier[index]];
		cashier_consulting_fee[index] = patientFee[patient_token];
		//Wait for the patient to pay
		cashierCV[index]->Signal(cashierLock[index]);
		cashierCV[index]->Wait(cashierLock[index]);
		//Get money
		totalIncome += patient_money_spent_at_cashier[patient_index_for_cashier[index]];
		//Make them leave
		cashierCV[index]->Signal(cashierLock[index]);
		//If line is empty, go to break
		if(cashier_line[index] == 0){
			cashier_state[index] = 2;
			cashier_break_lock[index]->Acquire();
			cashier_break_CV[index]->Wait(cashier_break_lock[index]);
			//hospital manager called
		}
		
	}
}

#include <stdlib.h>
void clerk(int index){
	char *debug_name2;
	//Assign pointers to new variables
	debug_name2 = new char[20];	
	sprintf(debug_name2, "clerk_CV #%d", index);
	phar_clerk_CV[index] = new Condition(debug_name2);


	debug_name2 = new char[40];
	sprintf(debug_name2, "phar_clerk_Lock #%d", index);
	phar_clerk_Lock[index] = new Lock(debug_name2);
	
	debug_name2 = new char[40];
	sprintf(debug_name2, "phar_clerk_line_CV #%d", index);
	phar_clerk_line_CV[index] = new Condition(debug_name2);
	
	debug_name2 = new char[40];
	sprintf(debug_name2, "phar_clerk_break_CV #%d", index);
	phar_clerk_break_CV[index] = new Condition(debug_name2);
	
	debug_name2 = new char[40];
	sprintf(debug_name2, "phar_clerk_break_Lock #%d", index);
	phar_clerk_break_Lock[index] = new Lock(debug_name2);
	

	
	while(true){
		printf("\nClerk [%u]\n", index);
		phar_clerk_line_Lock.Acquire();
			printf("Clerk [%d]: Acquired Lock %s\n", index, phar_clerk_line_Lock.getName());
		phar_clerk_state[index] = 0;
		if(phar_clerk_line[index] > 0){
			phar_clerk_line_CV[index]->Signal(&phar_clerk_line_Lock);
			phar_clerk_state[index] = 1;
		}
		phar_clerk_Lock[index]->Acquire();
		phar_clerk_line_Lock.Release();
		phar_clerk_CV[index]->Wait(phar_clerk_Lock[index]);
		//get the prescription
		int patient_condition;
		patient_condition = patient_illness[patient_index_to_clerk[index]];
		//Analyse the prescription and give medicine
		//Prescription is number from 1 ~ 4
		switch (patient_condition){
			case 1:
				//Illness #1
				medicine[index] = "Medicine #1";
				medicine_cost[index] = 20;
				break;
			case 2:
				//Illness #2
				medicine[index] = "Medicine #2";
				medicine_cost[index] = 15;
				break;
			case 3:
				//Illness #3
				medicine[index] = "Medicine #3";
				medicine_cost[index] = 40;
				break;
			case 4:
				//Illness #4
				medicine[index] = "Medicine #4";
				medicine_cost[index] = 50;
				break;
		}
		//Signal the patient to let them know medicine and bill
		phar_clerk_CV[index]->Signal(phar_clerk_Lock[index]);
		phar_clerk_CV[index]->Wait(phar_clerk_Lock[index]);
		//The patient return with the bill, collect the bill and make him leave
		phar_income += patient_phar_bill[patient_index_to_clerk[index]];
		//Signal him so he can leave
		phar_clerk_CV[index]->Signal(phar_clerk_Lock[index]);
		phar_clerk_CV[index]->Wait(phar_clerk_Lock[index]);
		//Wait for the patient to leave
		phar_clerk_Lock[index]->Release();
		//Take a break if no one in line
		if(phar_clerk_line[index] == 0){
			phar_clerk_state[index] = 2;
			phar_clerk_break_Lock[index]->Acquire();
			phar_clerk_break_CV[index]->Wait(phar_clerk_break_Lock[index]);
			//Wait for manager's signal
		}
	}

}

// --------------------------------------------------
// TestSuite()
//     This is the main thread of the test suite.  It runs the
//     following tests:
//
//       1.  Show that a thread trying to release a lock it does not
//       hold does not work
//
//       2.  Show that Signals are not stored -- a Signal with no
//       thread waiting is ignored
//
//       3.  Show that Signal only wakes 1 thread
//
//   4.  Show that Broadcast wakes all waiting threads
//
//       5.  Show that Signalling a thread waiting under one lock
//       while holding another is a Fatal error
//
//     Fatal errors terminate the thread in question.
// --------------------------------------------------
void TestSuite() {
    Thread *t;
    char *name;
    int i;
	for(i = 0; i < 20; i++){
		printf("#################################################################\n");
	}
		printf("#####################Start Testing###############################\n");
	for(i = 0; i < RECEPTIONISTS_COUNT; i++){
		name = new char[20];
		sprintf(name, "Receptionist Function #%d", i);
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)receptionist,i);
	}
	
	for(i = 0; i < DOORBOYS_COUNT; i++){
		name = new char[20];
		sprintf(name, "Door_Boy Function #%d", i);
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)DoorBoy,i);
	}	
	
	for(i = 0; i < DOCTORS_COUNT; i++){
		name = new char[20];
		sprintf(name, "Doctor Function #%d", i);
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)Doctor,i);
	}
	
	for(i = 0; i < CASHIERS_COUNT; i++){
		name = new char[20];
		sprintf(name, "Cashier Function #%d", i);
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)Cashier,i);
	}
	
	for(i = 0; i < CLERKS_COUNT; i++){
		name = new char[20];
		sprintf(name, "Clerk Function #%d", i);
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)clerk,i);
	}	

	for(i = 0; i < PATIENTS_COUNT; i++){
		name = new char[20];
		sprintf(name, "Patient Function #%d", i);
		t = new Thread(name);
		t->Fork((VoidFunctionPtr)patient,i);
	}	
	
	
}
#endif

