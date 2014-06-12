#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "copyright.h"
#include "system.h"

#ifdef CHANGED
#include "synch.h"

#define NUMBER_OF_PATIENTS 10
#define NUMBER_OF_RECEPTIONISTS 5
#define DOCTORS_COUNT 3
#define DOORBOYS_COUNT 5
#define CASHIERS_COUNT 3
#define NUMBER_OF_PHARMACISTS 3




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




/**
 * For any status variables, the following codes will be used
 *  0: available
 *  1: busy
 *  2: on-break
 */


// to make sense of our condition variable name, every time you see a wait
// read it as such. I am waiting for the condition <condition variable name>
// might be true. Every time you see a signal or broadcast read it as such.
// I am notifying you that <condition variable name> might be true now.


// bookkeeping data
Lock *random_number_lock = new Lock("Random number lock");
Lock *patient_count_mutex = new Lock("patient count mutex");
int patients_in_system = NUMBER_OF_PATIENTS;

// receptionist data
Condition *receptionist_break_cv = new Condition("Receptionist break CV");

// Patient Receptionist Data
int next_receptionist_token = 0;
int receptionist_token_bucket[NUMBER_OF_RECEPTIONISTS];
int receptionist_line_length[NUMBER_OF_RECEPTIONISTS] = { 0 };
int receptionist_state[NUMBER_OF_RECEPTIONISTS];

Condition *receptionist_line_empty_cv[NUMBER_OF_RECEPTIONISTS];
Condition *receptionist_line_full_cv[NUMBER_OF_RECEPTIONISTS];
Condition *receptionist_cv[NUMBER_OF_RECEPTIONISTS];

Lock *receptionist_line_lock = new Lock("receptionist line lock");
Lock *receptionist_lock[NUMBER_OF_RECEPTIONISTS];
Lock *receptionist_token_access_lock = new Lock("receptionist token access lock");

// door-boy data
Condition *doorboy_break_cv = new Condition("Door-boy break CV");

// Patient Door-boy Doctor Data
int doctor_state[DOCTORS_COUNT];
int number_of_patients_in_doorboy_line = 0;
int number_of_available_doctors = 0;
int next_available_doctor = -2;

Lock *doctor_doorboy_lock = new Lock("Doctor door-boy lock");
Lock *doorboy_line_lock = new Lock("Door-boy line lock");

Condition *patient_read_next_available_doctor_cv = new Condition("Patient read next available doctor CV");
Condition *doorboy_line_empty_cv = new Condition("Door-boy line empty CV");
Condition *doorboy_line_full_cv = new Condition("Door-boy line full CV");
Condition *doctor_available_cv = new Condition("Doctor available CV");
Condition *doctor_cv[DOCTORS_COUNT];


// Patient doctor data
int patient_doctor_bucket[DOCTORS_COUNT];
Lock *doctor_lock[DOCTORS_COUNT];


// Patient doctor cashier data
int patient_consultancy_fee[NUMBER_OF_PATIENTS];    // so why does this shared data structure need not be protected
                                                    // with a lock? Since Each patient has a unique index into the
                                                    // array, and we have guarantees on the sequencing on read/write
                                                    // operations, we know only one thread will ever be accessing
                                                    // an entry at any given time.

// cashier data
int total_cashier_fee = 0;

Condition *cashier_break_cv = new Condition("Cashier break CV");

Lock *cashier_payment_lock = new Lock("Cashier payment lock");


// patient cashier data
int cashier_available_count = 0;
int number_of_patients_in_cashier_line = 0;
int cashier_state[CASHIERS_COUNT];
int patient_cashier_bucket[CASHIERS_COUNT];

Lock *cashier_line_lock = new Lock("Cashier line lock");
Lock *cashier_lock[CASHIERS_COUNT];

Condition *cashier_line_empty_cv = new Condition("Cashier line empty CV");
Condition *cashier_cv[CASHIERS_COUNT];


// pharmacist data
int total_pharmacsit_money_collected = 0;

Condition *pharmacist_break_cv = new Condition("Pharmacist break CV");

Lock *pharmacist_payment_lock = new Lock("Pharmacist payment lock");


// Patient pharmacist data
int pharmacist_available_count = 0;
int pharmacist_line_length = 0;
int pharmacist_status[NUMBER_OF_PHARMACISTS];
int to_pharmacist_patient_token_bucket[NUMBER_OF_PHARMACISTS];
int to_pharmacist_prescription_bucket[NUMBER_OF_PHARMACISTS];
int to_patient_medicine_bucket[NUMBER_OF_PHARMACISTS];
int to_patient_medicine_bill_bucket[NUMBER_OF_PHARMACISTS];
int to_pharmacist_medicine_payment_bucket[NUMBER_OF_PHARMACISTS];

Condition *pharmacist_line_empty_cv = new Condition("Pharmacist line empty CV");
Condition *pharmacist_cv[NUMBER_OF_PHARMACISTS];

Lock *pharmacist_line_lock = new Lock("Pharmacist line lock");
Lock *pharmacist_lock[NUMBER_OF_PHARMACISTS];


// Disease data
const char *diseases[5] = {"is not sick", "is sick with measles", "is sick with chickenpox", "is sick with typhoid", "is sick with gangrene"};
const char *medicines[5] = {"", "ibuprofen", "vaccination shots", "antibiotics", "penicillin"};
const int consultation_cost[5] = {25, 35, 45, 55, 65};
const int medicine_costs[5] = {0, 15, 20, 25, 30};



void fill_array(int *array, int number_of_elements, int fill_value) {
    for(int i = 0; i < number_of_elements; i++) {
        array[i] = fill_value;
    }
}


void initialize() {
    // DO NOT perform a srand() call. If nachos is using this value
    // to handle its context switching we don't want to mess with it.

    // initialize all of our arrays with the appropriate objects

    // initialize shared patient receptionist arrays
    fill_array(receptionist_state, NUMBER_OF_RECEPTIONISTS, 1);
    fill_array(receptionist_token_bucket, NUMBER_OF_RECEPTIONISTS, -1);
    for(int i = 0; i < NUMBER_OF_RECEPTIONISTS; i++) {
        char receptionist_cv_name[32];
        char receptionist_line_empty_cv_name[32];
        char receptionist_line_full_cv_name[32];
        char receptionist_lock_name[32];

        sprintf(receptionist_cv_name, "Receptionist CV %d", i);
        sprintf(receptionist_line_empty_cv_name, "Receptionist line empty CV %d", i);
        sprintf(receptionist_line_full_cv_name, "Receptionist line full CV %d", i);
        sprintf(receptionist_lock_name, "Receptionist lock %d",i);
        
        receptionist_cv[i] = new Condition(receptionist_cv_name);
        receptionist_line_empty_cv[i] = new Condition(receptionist_line_empty_cv_name);
        receptionist_line_full_cv[i] = new Condition(receptionist_line_full_cv_name);
        receptionist_lock[i] = new Lock(receptionist_lock_name);
    }

    // initialize shared patient door-boy doctor data
    fill_array(doctor_state, DOCTORS_COUNT, 1);
    for(int i = 0; i < DOCTORS_COUNT; i++) {
        char doctor_lock_name[32];
        char doctor_cv_name[32];

        sprintf(doctor_lock_name, "Doctor lock %d", i);
        sprintf(doctor_cv_name, "Doctor CV %d", i);
        
        doctor_lock[i] = new Lock(doctor_lock_name);
        doctor_cv[i] = new Condition(doctor_cv_name);
    }

    // initialize shared patient doctor data
    fill_array(patient_doctor_bucket, DOCTORS_COUNT, -2);
    fill_array(patient_consultancy_fee, NUMBER_OF_PATIENTS, -2);

    //initialize shared cashier patient data
    fill_array(cashier_state, CASHIERS_COUNT, 1);
    fill_array(patient_cashier_bucket, CASHIERS_COUNT, -2);
    for(int i = 0; i < CASHIERS_COUNT; i++){
        char cashier_lock_name[32];
        char cashier_cv_name[32];

        sprintf(cashier_lock_name, "Cashier lock %d", i);
        sprintf(cashier_cv_name, "Cashier CV %d", i);

        cashier_lock[i] = new Lock(cashier_lock_name);
        cashier_cv[i] = new Condition(cashier_cv_name);
    }

    // initialize patient pharmacist data
    fill_array(pharmacist_status, NUMBER_OF_PHARMACISTS, 1);
    fill_array(to_pharmacist_patient_token_bucket, NUMBER_OF_PHARMACISTS, -2);
    fill_array(to_pharmacist_prescription_bucket, NUMBER_OF_PHARMACISTS, -2);
    fill_array(to_patient_medicine_bucket, NUMBER_OF_PHARMACISTS, -2);
    fill_array(to_patient_medicine_bill_bucket, NUMBER_OF_PHARMACISTS, -2);
    fill_array(to_pharmacist_medicine_payment_bucket, NUMBER_OF_PHARMACISTS, -2);

    for(int i = 0; i < NUMBER_OF_PHARMACISTS; i++) {
        char pharmacist_lock_name[32];
        char pharmacist_cv_name[32];

        sprintf(pharmacist_lock_name, "Pharmacist lock %d", i);
        sprintf(pharmacist_cv_name, "Pharmacist CV %d", i);
        
        pharmacist_lock[i] = new Lock(pharmacist_lock_name);
        pharmacist_cv[i] = new Condition(pharmacist_cv_name);
    }
}

void log(char *output_message) {
    return;
}

// Across the different functions to avoid clunky array[index]
// operations, we have local references to indexed values.
// Notice the trailing _ to denote they are local variables.


// our service policy is as follows
// Resource requesting threads (consumers), must always
// wait until a resource producing thread (producer)
// notifies them it is safe to continue.

void receptionist(const int receptionist_index) {
    printf("Receptionist [%d] is ready to work.\n", receptionist_index);
    while(true) {
        // let's check if there is someone in line
        // if there is we will tell them we are available
        // if not, we will sleep till someone tells us they are in line
        receptionist_line_lock->Acquire();
        while(receptionist_line_length[receptionist_index] == 0) {
            // no one is available, let's go to sleep
            printf("Receptionist [%d] no one here, going to bed.\n", receptionist_index);
            receptionist_state[receptionist_index] = 0;
            receptionist_line_empty_cv[receptionist_index]->Wait(receptionist_line_lock);
        }
        
        // someone is waiting in our line. Let's tell them we're ready
        receptionist_state[receptionist_index] = 1;
        printf("Receptionist [%d] has signaled a Patient.\n", receptionist_index);
        receptionist_line_full_cv[receptionist_index]->Signal(receptionist_line_lock);
        receptionist_line_lock->Release();
        
        receptionist_lock[receptionist_index]->Acquire();
        // now we need to go get a token to give to the patient. However we need
        // to be sure that bucket used to store the token is empty.
        while(receptionist_token_bucket[receptionist_index] != -1) {
            printf("We really shouldn't be getting here....\n");
            receptionist_cv[receptionist_index]->Wait(receptionist_lock[receptionist_index]);
        }

        receptionist_token_access_lock->Acquire();
        receptionist_token_bucket[receptionist_index] = next_receptionist_token;
        next_receptionist_token++;
        receptionist_token_access_lock->Release();
        receptionist_cv[receptionist_index]->Signal(receptionist_lock[receptionist_index]);

        // we need to ensure our customer has gotten their token before we can continue
        while(receptionist_token_bucket[receptionist_index] != -1) {
            printf("Receptionist [%d] waiting for customer to take token.\n", receptionist_index);
            receptionist_cv[receptionist_index]->Wait(receptionist_lock[receptionist_index]);
        }

        receptionist_lock[receptionist_index]->Release();

        receptionist_line_lock->Acquire();
        // let's check if we can go on break
        if(receptionist_line_length == 0) {
            receptionist_state[receptionist_index] = 2;
            printf("Receptionist [%d] is going on break.\n", receptionist_index);
            receptionist_break_cv->Wait(receptionist_line_lock);
            receptionist_state[receptionist_index] = 1;
            printf("Receptionist [%d] is coming off break.\n", receptionist_index);
        }
        receptionist_line_lock->Release();
    }
}

void patient(const int patient_index) {
    int my_patient_diagnosis = -2;
    int my_doctor_index = -2;
    int my_patient_token = -2;
    int my_pharmacist_index = -2;
    int my_cashier_index = -2;

    printf("Patient [%d] has arrived at the hospital.\n", patient_index);

    // let's find the shortest line available to us
    receptionist_line_lock->Acquire();
    int shortest_line_length = receptionist_line_length[0];
    int my_receptionist_index = 0;
    for(int i = 0; i < NUMBER_OF_RECEPTIONISTS; i++) {
        if (receptionist_line_length[i] == 0) {
            // why bother checking anymore?
            // we aren't going to find a shorter line
            my_receptionist_index = i;
            break;
        }
        else if(receptionist_line_length[i] < shortest_line_length) {
            shortest_line_length = receptionist_line_length[i];
            my_receptionist_index = i;
        }
    }

    // irrespective of how we got here, we are always going to wait in line
    // this enforces our service policy that a service requesting thread must
    // always wait on a service producing thread.
    receptionist_line_length[my_receptionist_index] ++;
    printf("Patient [%d] is waiting for Receptionist [%d].\n", patient_index, my_receptionist_index);
    
    // According to our logic, the receptionist could be busy
    // with another patient or could be waiting for a patient.
    // If the receptionist is waiting for a patient, the signal
    // will wake them up. If the receptionist is with another patient
    // the receptionist loop will ensure that we still see this patient.
    // The structure of how we designed this interaction gives us the liberty
    // of not caring if the signal is lost.
    receptionist_line_empty_cv[my_receptionist_index]->Signal(receptionist_line_lock);
    receptionist_line_full_cv[my_receptionist_index]->Wait(receptionist_line_lock);                                                                                              

    // at this point the receptionist has told us that they are ready for us
    // let's remove ourselves from the line and be on our way
    receptionist_line_length[my_receptionist_index]--;
    receptionist_line_lock->Release();

    receptionist_lock[my_receptionist_index]->Acquire();
    // let's check if the receptionist has produced a token. If he hasn't we will wait till he does.
    // we will wait while the token we are looking for is -1. As soon as it's not, we know the receptionist
    // delivered a token to us.
    while(receptionist_token_bucket[my_receptionist_index] == -1) {
        printf("Patient [%d] is waiting on receptionist [%d]\n", patient_index, my_receptionist_index);
        receptionist_cv[my_receptionist_index]->Wait(receptionist_lock[my_receptionist_index]);
    }

    my_patient_token = receptionist_token_bucket[my_receptionist_index];
    receptionist_token_bucket[my_receptionist_index] = -1;                  
    receptionist_cv[my_receptionist_index]->Signal(receptionist_lock[my_receptionist_index]);   // to inform the receptionist we took the token
                                                                                                // again doesn't matter if the signal gets lost
                                                                                                // since we have the check if token = -1 in the
                                                                                                // receptionist

    receptionist_lock[my_receptionist_index]->Release();

    printf("Patient [%d] has received Token [%d] from receptionist [%d].\n", 
        patient_index, my_patient_token, my_receptionist_index);
    
    // now we are going to get in line with the door-boy
    // always waiting in doctor's offices aren't we?
    doorboy_line_lock->Acquire();
    number_of_patients_in_doorboy_line++;
    printf("Patient [%d] is waiting on a DoorBoy\n", my_patient_token);
    doorboy_line_full_cv->Signal(doorboy_line_lock);
    
    doorboy_line_empty_cv->Wait(doorboy_line_lock);

    // now we need to read which doctor the door boy has assigned us
    my_doctor_index = next_available_doctor;

    next_available_doctor = -2;
    
    // inform the door boy we've read the value and it's OK to move on.
    patient_read_next_available_doctor_cv->Signal(doorboy_line_lock);
    doorboy_line_lock->Release();

    // we should never get to this point and my_doctor_index be == -2
    assert(my_doctor_index != -2);
    printf("Patient [%d] has been told by Door Boy to go to examining room [%d].\n", 
        my_patient_token, my_doctor_index);
    

    // let's tell our doctor we're coming to him.
    doctor_lock[my_doctor_index]->Acquire();
    printf("Patient [%d] is going to examining room [%d].\n", 
        my_patient_token, my_doctor_index);
    doctor_cv[my_doctor_index]->Signal(doctor_lock[my_doctor_index]);

    // we are now with the doctor. let's give him
    // our token and wait till the consultation is over.
    // since we hold the doctor_lock right now, we know the
    // doctor will always see our token in the bucket.
    patient_doctor_bucket[my_doctor_index] = my_patient_token;
    printf("Patient [%d] is waiting to be examined by the doctor in Examining Room [%d].\n", 
        patient_index, my_doctor_index);
    doctor_cv[my_doctor_index]->Wait(doctor_lock[my_doctor_index]);

    printf("Patient [%d] is done with a consultation.\n", patient_index);

    // the doctor has finished his consultation with me now.
    // let's pick our diagnosis and then go pay the cashier
    // for the consultation
    my_patient_diagnosis = patient_doctor_bucket[my_doctor_index];
    printf("Patient [%d] [%s] in examining room [%d]",
        patient_index, diseases[my_patient_diagnosis], my_doctor_index);
    patient_doctor_bucket[my_doctor_index] = -2;
    
    // inform the doctor we are leaving
    doctor_cv[my_doctor_index]->Signal(doctor_lock[my_doctor_index]);
    doctor_lock[my_doctor_index]->Release();

    printf("Patient [%d] is leaving examining room [%d]", patient_index, my_doctor_index);

    // we're off to see the wizard!
    // the wonderful Wizard of OZZ :)

    // LOL, no, off to see the cashier

    cashier_line_lock->Acquire();

    number_of_patients_in_cashier_line++;

    // let's see if there is an available cashier
    // if there is, we'll find him, else we'll have to wait.
    if(cashier_available_count == 0) {
        while(cashier_available_count == 0) {
            cashier_line_empty_cv->Wait(cashier_line_lock);
        }
    }

    number_of_patients_in_cashier_line--;

    // let's go find our free cashier
    for(int i = 0; i < CASHIERS_COUNT; i++) {
        if(cashier_state[i] == 0) {
            my_cashier_index = i;
            cashier_state[i] = 1;
            cashier_available_count--;
            break;
        }
    }

    assert(my_cashier_index != -2);

    printf("Patient [%d] is waiting to see cashier. [%d]\n", patient_index, my_cashier_index);

    cashier_line_lock->Release();
    
    // let's go talk to our cashier
    cashier_lock[my_cashier_index]->Acquire();
    cashier_cv[my_cashier_index]->Signal(cashier_lock[my_cashier_index]);

    // let's give the cashier our data and wait
    patient_cashier_bucket[my_cashier_index] = my_patient_token;
    cashier_cv[my_cashier_index]->Wait(cashier_lock[my_cashier_index]);
    
    const int my_consultation_fee = patient_cashier_bucket[my_cashier_index];
    patient_cashier_bucket[my_cashier_index] = -2;

    printf("Patient [%d] is paying their consultancy fees of [%d]",
        patient_index, my_consultation_fee);

    cashier_cv[my_cashier_index]->Signal(cashier_lock[my_cashier_index]);
    cashier_lock[my_cashier_index]->Release();


    printf("Patient [%d] is leaving Cashier [%d]",
        patient_index, my_cashier_index);


    // let's go off to the pharmacy
    pharmacist_line_lock->Acquire();
    
    pharmacist_line_length++;
    printf("Patient [%d] is in line at the pharmacy.\n", patient_index);

    // let's see if there is an available pharmacist
    // if there is, we'll find him, else we'll have to wait
    if(pharmacist_available_count == 0) {
        while(pharmacist_available_count == 0) {
            pharmacist_line_empty_cv->Wait(pharmacist_line_lock);
        }
    }

    pharmacist_line_length--;

    // let's go find our free pharmacist
    for(int i = 0; i < NUMBER_OF_PHARMACISTS; i++) {
        if(pharmacist_status[i] == 0) {
            my_pharmacist_index = i;
            pharmacist_status[i] = 1;
            pharmacist_available_count--;
            break;
        }
    }

    assert(my_pharmacist_index != -2);

    printf("Patient [%d] is waiting to see pharmacy clerk. [%d]\n", 
        patient_index, my_pharmacist_index);

    pharmacist_line_lock->Release();

    // let's go talk directly to our pharmacist
    pharmacist_lock[my_pharmacist_index]->Acquire();
    pharmacist_cv[my_pharmacist_index]->Signal(pharmacist_lock[my_pharmacist_index]);

    // let's give the pharmacist our data and then wait
    to_pharmacist_patient_token_bucket[my_pharmacist_index] = my_patient_token;
    to_pharmacist_prescription_bucket[my_pharmacist_index] = my_patient_diagnosis;
    pharmacist_cv[my_pharmacist_index]->Wait(pharmacist_lock[my_pharmacist_index]);

    const int my_medicine = to_patient_medicine_bucket[my_pharmacist_index];
    const int my_medicine_cost = to_patient_medicine_bill_bucket[my_pharmacist_index];

    printf("Patient [%d] is paying their prescription fees of [%d].\n", 
        patient_index, my_medicine_cost);
    
    // clear the buckets
    to_patient_medicine_bucket[my_pharmacist_index] = NULL;
    to_patient_medicine_bill_bucket[my_pharmacist_index] = -2;

    // let's pay the pharmacist and be on our way
    to_pharmacist_medicine_payment_bucket[my_pharmacist_index] = my_medicine_cost;

    pharmacist_cv[my_pharmacist_index]->Signal(pharmacist_lock[my_pharmacist_index]);

    printf("Patient [%d] is leaving pharmacy clerk [%d].\n", 
        patient_index, my_pharmacist_index);
    
    pharmacist_lock[my_pharmacist_index]->Release();

    // that's it
    // we are now free to leave the hospital!!

    printf("Patient [%d] is leaving the Hospital.\n", patient_index);

    patient_count_mutex->Acquire();
    patients_in_system--;
    if(patients_in_system == 0) {
        printf("\nALL PATIENTS EXITED THE SIMULATION\n\n");
    }
    patient_count_mutex->Release();

}

// The patient door-boy doctor interaction is a bit interesting.
// Since the door-boy works as an intermediary to allow patients
// to see the doctor, we can't model this exactly like we did for
// the patient - receptionist interaction.

// Here is a quick overview of what is going on.
// A patient will first arrive and will always wait in the door-boy
// line. If we were to think of a producer consumer model, the patient
// is a consumable being stored in an unbounded queue (which we model
// with the doorboy_line_empty/full_cv). Now when a patient gets in line
// they signal the doorboy_line_empty_cv to notify any waiting door-boy
// that there is now at least one patient in line. The door-boy wakes up
// and then checks if there is an available doctor. If there isn't the door-boy
// must wait for a doctor to become available before signaling the patient
// that they can go in to see the doctor.

// After the door-boy has a guarantee that there is a one to one matching
// between a patient and a doctor, they wake up the patient to inform them
// a doctor is available (it is still up to the patient to figure out which 
// doctor is free). The patient will then identify the available doctor and
// directly signal the doctor the doctor to wake up.

void doorboy(const int doorboy_index) {
    // if the number of patients in the door-boy line is 0
    // the door-boy will wait on the doorboy_line_empty_cv 
    // for a patient to show up. Once a patient is available,
    // the door-boy will check if there are any available
    // doctors. If there are, he will send the patient through.
    // Otherwise, the door-boy will wait on the doctor_available_cv
    // until a doctor becomes available.
    printf("Door-boy [%d] is ready to work.\n", doorboy_index);
    while(true) {
        doorboy_line_lock->Acquire();

        while(number_of_patients_in_doorboy_line == 0) {
            printf("Door-boy [%d] waiting for a patient to arrive.\n", doorboy_index);
            doorboy_line_full_cv->Wait(doorboy_line_lock);
        }
        number_of_patients_in_doorboy_line--;       // we are reserving a patient so another
                                                    // door-boy won't try and find the same
                                                    // patient a doctor as well.
        doorboy_line_lock->Release();

        printf("Door-boy [%d] acquired a patient.\n", doorboy_index);
        // we now have a patient in our grasp muahahaha >;)
        doctor_doorboy_lock->Acquire();
        while(number_of_available_doctors == 0) {
            doctor_available_cv->Wait(doctor_doorboy_lock);
        }

        // now we need to find a free doctor
        int available_doctor = -2;
        for(int i = 0; i < DOCTORS_COUNT; i++) {
            if(doctor_state[i] == 0) {
                available_doctor = i;
                doctor_state[i] = 1;
                number_of_available_doctors--;
                break;
            }
        }
        assert(available_doctor != -2);
        printf("Door-boy [%d] acquired doctor [%d].\n", doorboy_index, available_doctor);

        // now we need a guarantee that no other door-boy will alter the
        // value of next_available_doctor while we wait for the next patient
        // to read that value. As such we will hold onto the doctor_doorboy_lock
        // so no one else can enter this critical section until after we have
        // confirmation of a read from the patient.
        doorboy_line_lock->Acquire();
        next_available_doctor = available_doctor;
        doorboy_line_empty_cv->Signal(doorboy_line_lock);
        patient_read_next_available_doctor_cv->Wait(doorboy_line_lock);

        assert(next_available_doctor == -2);

        // at this point we have confirmation that the patient has read
        // the next_available doctor and we can release our locks and wait
        // for the next patient
        doctor_doorboy_lock->Release();

        // we will hold off for just a second
        // to release our line lock to see if,
        // we can go on break.
        if(number_of_patients_in_doorboy_line == 0) {
            // let's go on break
            doorboy_break_cv->Wait(doorboy_line_lock);
        }
        doorboy_line_lock->Release();
    }
}


void doctor(const int doctor_index) {
    printf("Doctor [%d] is ready to work.\n", doctor_index);
    while(true) {
        // first we need to set the doctor state without 
        // other threads getting in the way
        doctor_doorboy_lock->Acquire();
        doctor_state[doctor_index] = 0;

        number_of_available_doctors++;
        doctor_available_cv->Signal(doctor_doorboy_lock);
        
        printf("Doctor [%d] acquiring doctor_lock.\n", doctor_index);
        doctor_lock[doctor_index]->Acquire();       // we need to ensure that this doctor
        doctor_doorboy_lock->Release();        // will actually be waiting for their patient
        
        printf("Doctor [%d] waiting for a patient.\n", doctor_index);
        doctor_cv[doctor_index]->Wait(doctor_lock[doctor_index]);

        int patient_token = patient_doctor_bucket[doctor_index];    // we will not clear out this value since we going to 
                                                                    // write back to the bucket before we give up the lock.
        
        printf("Doctor [%d] is examining a Patient with Token [%d].\n", doctor_index, patient_token);

        random_number_lock->Acquire();
        int consultation_time = (rand() % 20) + 10;
        int patient_sickness = (rand() % 5);
        int take_break = (rand() % 2);
        random_number_lock->Release();

        // now that we have a patient, let's examine them
        for(int i = 0; i < consultation_time; i++) {
            currentThread->Yield();
        }

        printf("Doctor [%d] has determined that the Patient with Token [%d] [%s].\n",
            doctor_index, patient_token, diseases[patient_sickness]);
        if(patient_sickness != 0) {
            printf("Doctor [%d] is prescribing [%s] to the Patient with Token [%d].\n",
                doctor_index, medicines[patient_sickness], patient_token);
        }

        // put the patient sickness in the patient_illness_bucket
        // and the patient_doctor_bucket. This is to inform the 
        // cashier of the consultation fee and to tell the patient
        // what is wrong with them so they can get the right medicine
        // at the pharmacy.
        patient_doctor_bucket[doctor_index] = patient_sickness;
        patient_consultancy_fee[patient_token] = patient_sickness;

        // inform the patient we're done and wait
        // for a guarantee that the patient has
        // seen their diagnosis and left.
        doctor_cv[doctor_index]->Signal(doctor_lock[doctor_index]);
        while(patient_doctor_bucket[doctor_index] != -2) {
            doctor_cv[doctor_index]->Wait(doctor_lock[doctor_index]);
        }
        
        doctor_lock[doctor_index]->Release();
    }
}


void cashier(const int cashier_index){
    printf("Cashier [%d] is ready to work.\n", cashier_index);
    while(true){

        int my_patient_token = -2;

        // the cashier line mechanism is the same as the
        // pharmacists. Consult the comments for the 
        // pharmacist for more details.
        cashier_line_lock->Acquire();

        if(number_of_patients_in_cashier_line == 0) {
            // let's go on break
            cashier_state[cashier_index] = 2;
            cashier_break_cv->Wait(cashier_line_lock);
        }

        cashier_state[cashier_index] = 0;
        cashier_available_count++;
        printf("Cashier [%d] has signaled a Patient", cashier_index);
        cashier_line_empty_cv->Signal(cashier_line_lock);

        // transition to the next critical
        // section before moving out of this one.
        cashier_lock[cashier_index]->Acquire();
        cashier_line_lock->Release();

        // now we wait for a customer
        cashier_cv[cashier_index]->Wait(cashier_lock[cashier_index]);

        // let's gather the patient data
        my_patient_token = patient_cashier_bucket[cashier_index];
        assert(my_patient_token != -2);
        printf("Cashier [%d] gets Token [%d] from a Patient.\n", cashier_index, my_patient_token);
        
        // let's send this data back to the patient
        int total_consultation_fee = patient_consultancy_fee[my_patient_token];
        patient_cashier_bucket[cashier_index] = total_consultation_fee;
        printf("Cashier [%d] tells Patient with Token [%d] they owe $%d\n", cashier_index, my_patient_token, total_consultation_fee);
        cashier_cv[cashier_index]->Signal(cashier_lock[cashier_index]);

        // to ensure the patient saw the amount, we are going
        // to wait until they clear the patient_cashier_bucket.
        // Their viewing of the payment amount is consent for us
        // to charge their card that we have on file. Therefore
        // we don't need another interaction with the payment.
        while(patient_cashier_bucket[cashier_index] != -2){
            cashier_cv[cashier_index]->Wait(cashier_lock[cashier_index]);
        }

        cashier_payment_lock->Acquire();
        total_cashier_fee += total_consultation_fee;
        printf("Cashier [%d] receives fees from Patient with Token [%d] from a Patient.\n", cashier_index, my_patient_token);
        cashier_payment_lock->Release();

        // we're now done with this customer        
        cashier_lock[cashier_index]->Release();

    }
}


void pharmacist(const int pharmacist_index) {
    printf("Pharmacist [%d] is ready to work.\n", pharmacist_index);
    while(true) {

        int my_patient_token = -2;

        // we are going to wait for the patient to come up to us
        // it is easier to manage initial transfer of data to the
        // pharmacist because it can be done in the pharmacist / patient
        // critical section, we don't need to add synchronization bloat
        // to the line area.
        pharmacist_line_lock->Acquire();

        if(pharmacist_line_length == 0) {
            // we're going on break
            pharmacist_status[pharmacist_index] = 2;
            pharmacist_break_cv->Wait(pharmacist_line_lock);
        }

        pharmacist_status[pharmacist_index] = 0;
        pharmacist_available_count++;
        pharmacist_line_empty_cv->Signal(pharmacist_line_lock); // in case there was a patient already waiting in line
        
        // we need to make sure that we are ready for the
        // patient's signal before it is issued. Therefore
        // we need to enter the second critical section
        // before we release the line_lock.
        pharmacist_lock[pharmacist_index]->Acquire();
        pharmacist_line_lock->Release();

        // now let's wait for a customer to come up to us
        pharmacist_cv[pharmacist_index]->Wait(pharmacist_lock[pharmacist_index]);
        
        // now we know the patient has arrived and we can proceed
        // we also have a guarantee that the patient diagnosis is
        // waiting for us in our bucket.
        my_patient_token = to_pharmacist_patient_token_bucket[pharmacist_index];
        const int my_patient_prescription = to_pharmacist_prescription_bucket[pharmacist_index];
        const char *patient_medicine = medicines[my_patient_prescription];
        const int patient_medicine_cost = medicine_costs[my_patient_prescription];

        assert(my_patient_token != -2);

        // clear the buckets
        to_pharmacist_prescription_bucket[pharmacist_index] = -2;

        printf("Pharmacist [%d] got prescription [%s] from patient with token [%d].\n", 
            pharmacist_index, patient_medicine, my_patient_token);
        printf("Pharmacist [%d] gives prescription [%s] to patient with token [%d].\n", 
            pharmacist_index, patient_medicine, my_patient_token);
        printf("Pharmacist [%d] tells patient with token [%d] they owe [%d].\n", 
            pharmacist_index, my_patient_token, patient_medicine_cost);

        // let's send this data back to the patient
        to_patient_medicine_bucket[pharmacist_index] = my_patient_prescription;
        to_patient_medicine_bill_bucket[pharmacist_index] = patient_medicine_cost;

        // now we let the patient know we're done
        // and we wait for the payment response.
        pharmacist_cv[pharmacist_index]->Signal(pharmacist_lock[pharmacist_index]);
        pharmacist_cv[pharmacist_index]->Wait(pharmacist_lock[pharmacist_index]);

        printf("Pharmacist [%d] received a response from patient [%d].\n",
            pharmacist_index, my_patient_token);
        
        // we now have a response from the patient with their payment
        pharmacist_payment_lock->Acquire();
        const int payment_for_medicine = to_pharmacist_medicine_payment_bucket[pharmacist_index];
        assert(payment_for_medicine != -2);

        printf("Pharmacist [%d] gets money from patient with token [%d].\n", 
            pharmacist_index, my_patient_token);

        total_pharmacsit_money_collected += payment_for_medicine;
        to_pharmacist_medicine_payment_bucket[pharmacist_index] = -2;
        pharmacist_payment_lock->Release();

        pharmacist_lock[pharmacist_index]->Release();
    }
}

void hospital_manager(const int hospital_manager_index) {
    bool continue_running = true;
    while(true) {

        // pick a random amount of time to wait
        random_number_lock->Acquire();
        const int wait_time = (rand() % 75) + 25;
        const bool request_total_cashier_sales = ((rand() % 2) == 0);
        const bool request_total_pharmacy_sales = ((rand() % 2) == 0);
        random_number_lock->Release();
        
        for(int i = 0; i < wait_time; i++) {
            currentThread->Yield();
        }

        // first let's check if our simulation is done
        patient_count_mutex->Acquire();
        if(patients_in_system == 0) {
            // we can't return here to terminate the thread.
            // Why? Because if we do, we will leak the lock.
            // The thread the currently owns it (this) will terminate
            // without ever calling release. So everyone else waiting
            // to acquire this lock is screwed.
            continue_running = false;
        }
        patient_count_mutex->Release();

        if(!continue_running) {
            return;
        }

        // now we hop in randomly to cause some ruckus

        if(request_total_pharmacy_sales) {
            pharmacist_payment_lock->Acquire();
            printf("HospitalManager reports total sales in pharmacy are [%d].\n", total_pharmacsit_money_collected);
            pharmacist_payment_lock->Release();
        }

        if(request_total_cashier_sales) {
            cashier_payment_lock->Acquire();
            printf("Hospital manager reports total consultancy fees are [%d].\n.", total_cashier_fee);
            cashier_payment_lock->Release();
        }

        // let's check if there is someone waiting in line for the receptionist
        receptionist_line_lock->Acquire();
        for(int i = 0; i < NUMBER_OF_RECEPTIONISTS; i++) {
            if(receptionist_line_length[i] > 2) {
                // signal all the receptionists to come off break
                receptionist_break_cv->Signal(receptionist_line_lock);
                printf("HospitalManager signaled a Receptionist to come off break.\n");
            }
        }
        receptionist_line_lock->Release();

        // let's check if there is someone waiting for the door-boy
        doorboy_line_lock->Acquire();
        if(number_of_patients_in_doorboy_line != 0) {
            // signal all the door-boys to off break
            doorboy_break_cv->Signal(doorboy_line_lock);
            printf("HospitalManager signaled a DoorBoy to come off break.\n");
        }
        doorboy_line_lock->Release();

        // let's check if there is someone waiting in line for the cashier
        cashier_line_lock->Acquire();
        if(number_of_patients_in_cashier_line != 0) {
            // signal all the cashiers to come off break
            cashier_break_cv->Signal(cashier_line_lock);
            printf("HospitalManager signaled a Cashier to come off break.\n");
        }
        cashier_line_lock->Release();

        // let's check if there is someone waiting for the pharmacist
        pharmacist_line_lock->Acquire();
        if(pharmacist_line_length != 0) {
            // signal all the pharmacists to come off break
            pharmacist_break_cv->Signal(pharmacist_line_lock);
            printf("HospitalManager signaled a PharmacyClerk to come off break.\n");
        }
        pharmacist_line_lock->Release();
    }
}


void TestSuite() {
    Thread *t;
    char *name;
    int i;

    // Test 1
    printf("Starting Test 1\n");

    t = new Thread("t1_t1");
    t->Fork((VoidFunctionPtr)t1_t1,0);

    printf("Thread t1_t1 allocated\n");

    t = new Thread("t1_t2");
    t->Fork((VoidFunctionPtr)t1_t2,0);

    t = new Thread("t1_t3");
    t->Fork((VoidFunctionPtr)t1_t3,0);


    // Wait for Test 1 to complete
    for (  i = 0; i < 2; i++ ) {
        t1_done.P();
    }

    // Test 2

    printf("Starting Test 2.  Note that it is an error if thread t2_t2\n");
    printf("completes\n");

    t = new Thread("t2_t1");
    t->Fork((VoidFunctionPtr)t2_t1,0);

    t = new Thread("t2_t2");
    t->Fork((VoidFunctionPtr)t2_t2,0);

    // Wait for Test 2 to complete
    t2_done.P();

    // Test 3

    printf("Starting Test 3\n");

    for (  i = 0 ; i < 5 ; i++ ) {
        name = new char [20];
        sprintf(name,"t3_waiter%d",i);
        t = new Thread(name);
        t->Fork((VoidFunctionPtr)t3_waiter,0);
    }
    t = new Thread("t3_signaller");
    t->Fork((VoidFunctionPtr)t3_signaller,0);

    // Wait for Test 3 to complete
    for (  i = 0; i < 2; i++ ) {
        t3_done.P();
    }

    // Test 4

    printf("Starting Test 4\n");

    for (  i = 0 ; i < 5 ; i++ ) {
        name = new char [20];
        sprintf(name,"t4_waiter%d",i);
        t = new Thread(name);
        t->Fork((VoidFunctionPtr)t4_waiter,0);
    }
    t = new Thread("t4_signaller");
    t->Fork((VoidFunctionPtr)t4_signaller,0);

    // Wait for Test 4 to complete
    for (  i = 0; i < 6; i++ ) {
        t4_done.P();
    }

    // Test 5

    printf("Starting Test 5.  Note that it is an error if thread t5_t1\n");
    printf("completes\n");

    t = new Thread("t5_t1");
    t->Fork((VoidFunctionPtr)t5_t1,0);

    t = new Thread("t5_t2");
    t->Fork((VoidFunctionPtr)t5_t2,0);
}


void HmSimulation() {
    initialize();
    printf("Number of Receptionists = [%d]\n", NUMBER_OF_RECEPTIONISTS);
    printf("Number of Doctors = [%d]\n", DOCTORS_COUNT);
    printf("Number of DoorBoys = [%d]\n", DOORBOYS_COUNT);
    printf("Number of Cashiers = [%d]\n", CASHIERS_COUNT);
    printf("Number of PharmacyClerks = [%d]\n", NUMBER_OF_PHARMACISTS);
    printf("Number of Patients = [%d]\n", NUMBER_OF_PATIENTS);

    // all shared data has been initialized at this point
    // the initialize function solves the issue of threads
    // accessing shared data before it has all been initialized

    
    for(int i = 0; i < NUMBER_OF_RECEPTIONISTS; i++) {
        char thread_name[32];
        sprintf(thread_name, "Receptionist function %d", i);
        (new Thread(thread_name))->Fork((VoidFunctionPtr)receptionist, i);
    }

    for(int i = 0; i < DOORBOYS_COUNT; i++) {
        char thread_name[32];
        sprintf(thread_name, "Door-boy function %d", i);
        (new Thread(thread_name))->Fork((VoidFunctionPtr)doorboy, i);
    }

    for(int i = 0; i < DOCTORS_COUNT; i++) {
        char thread_name[32];
        sprintf(thread_name, "Doctor function %d", i);
        (new Thread(thread_name))->Fork((VoidFunctionPtr)doctor, i);
    }

    for(int i = 0; i < CASHIERS_COUNT; i++){
        char thread_name[32];
        sprintf(thread_name, "Cashier function %d", i);
        (new Thread(thread_name))->Fork((VoidFunctionPtr)cashier, i);
    }

    for(int i = 0; i < NUMBER_OF_PHARMACISTS; i++) {
        char thread_name[32];
        sprintf(thread_name, "Pharmacist function %d", i);
        (new Thread(thread_name))->Fork((VoidFunctionPtr)pharmacist, i);
    }

    char hm_name[32];
    sprintf(hm_name, "HospitalManager function");
    (new Thread(hm_name))->Fork((VoidFunctionPtr)hospital_manager, 0);

    for(int i = 0; i < NUMBER_OF_PATIENTS; i++) {
        char thread_name[32];
        sprintf(thread_name, "Patient function %d", i);
        (new Thread(thread_name))->Fork((VoidFunctionPtr)patient, i);
    }
}

#endif
