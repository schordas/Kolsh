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
#define CLERKS_COUNT 3


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

// Patient doctor cashier data
int patient_consultancy_fee[NUMBER_OF_PATIENTS];    // so why does this shared data structure need not be protected
                                                    // with a lock? Since Each patient has a unique index into the
                                                    // array, and we have guarantees on the sequencing on read/write
                                                    // operations, we know only one thread will ever be accessing
                                                    // an entry at any given time.

// P

Lock *doctor_state_lock = new Lock("Doctor state lock");
Lock *doctor_lock[DOCTORS_COUNT];

// Disease data
const char *diseases[5] = {"is not sick", "is sick with measles", "is sick with chickenpox", "is sick with typhoid", "is sick with gangrene"};
const char *medicines[5] = {"", "ibuprofen", "vaccination shots", "antibiotics", "penicillin"};
const int consultation_cost[5] = {25, 35, 45, 55, 65};

//Cashier patient data
int cashier_state[CASHIERS_COUNT];
int number_of_patients_in_cashier_line = 0;
int number_of_available_cashiers = 0;
int patient_cashier_bucket[CASHIERS_COUNT];
int cashier_line_bucket;
int total_cashier_fee = 0;

Lock *cashier_patient_lock = new Lock("Cashier paitient lock");
Lock *cashier_line_lock = new Lock("Cashier line lock");
Lock *cashier_lock[CASHIERS_COUNT];
Lock *cashier_payment_lock = new Lock("Cashier payment lock");

Condition *patient_read_next_available_cashier_cv = new Condition("Patient read next available cashier CV");
Condition *cashier_line_empty = new Condition("Cashier line empty CV");
Condition *cashier_line_populated_cv = new Condition("Cashier line is not empty CV");
Condition *cashier_available_cv = new Condition("Cashier available CV");
Condition *cashier_cv[CASHIERS_COUNT];



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

    //initialize chared cashier patient data
    fill_array(cashier_state, CASHIERS_COUNT, 1);
    for(int i = 0; i < CASHIERS_COUNT; i++){
        char cashier_lock_name[32];
        char cashier_cv_name[32];

        sprintf(cashier_lock_name, "Cashier lock %d", i);
        sprintf(cashier_cv_name, "Cashier CV %d", i);

        cashier_lock[i] = new Lock(cashier_lock_name);
        cashier_cv[i] = new Condition(cashier_cv_name);
    }

    // initialize shared patient doctor data
    fill_array(patient_doctor_bucket, DOCTORS_COUNT, -2);
    fill_array(patient_consultancy_fee, NUMBER_OF_PATIENTS, -2);
    fill_array(patient_cashier_bucket, CASHIERS_COUNT, -2);
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
        printf("Receptionist [%d] told someone to come up.\n", receptionist_index);
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
    }
}

void patient(const int patient_index) {
    int my_patient_diagnosis = -2;
    int my_doctor_index = -2;
    int my_cashier_index = -2;

    

    
    int my_patient_token = -2;

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
    printf("Patient [%d] in line for receptionist [%d].\n", patient_index, my_receptionist_index);
    
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

    printf("Patient [%d] is going to receptionist [%d]\n", patient_index, my_receptionist_index);

    receptionist_lock[my_receptionist_index]->Acquire();
    // let's check if the receptionist has produced a token. If he hasn't we will wait till he does.
    // we will wait while the token we are looking for is -1. As soon as it's not, we know the receptionist
    // delivered a token to us.
    while(receptionist_token_bucket[my_receptionist_index] == -1) {
        printf("Patient [%d] waiting for receptionist [%d] token\n", patient_index, my_receptionist_index);
        receptionist_cv[my_receptionist_index]->Wait(receptionist_lock[my_receptionist_index]);
    }

    my_patient_token = receptionist_token_bucket[my_receptionist_index];
    receptionist_token_bucket[my_receptionist_index] = -1;                  
    receptionist_cv[my_receptionist_index]->Signal(receptionist_lock[my_receptionist_index]);   // to inform the receptionist we took the token
                                                                                                // again doesn't matter if the signal gets lost
                                                                                                // since we have the check if token = -1 in the
                                                                                                // receptionist

    receptionist_lock[my_receptionist_index]->Release();

    printf("Patient [%d] got token [%d] from receptionist [%d]\n", 
        patient_index, my_patient_token, my_receptionist_index);
    
    
    // now we are going to get in line with the door-boy
    // always waiting in doctor's offices aren't we?
    doorboy_line_lock->Acquire();
    number_of_patients_in_doorboy_line++;
    doorboy_line_full_cv->Signal(doorboy_line_lock);
    printf("Patient [%d] waiting to see doctor.\n", patient_index);
    doorboy_line_empty_cv->Wait(doorboy_line_lock);

    // now we need to read which doctor the door boy has assigned us
    my_doctor_index = next_available_doctor;
    next_available_doctor = -2;
    
    // inform the door boy we've read the value and it's OK to move on.
    patient_read_next_available_doctor_cv->Signal(doorboy_line_lock);
    doorboy_line_lock->Release();

    // we should never get to this point and my_doctor_index be == -2
    assert(my_doctor_index != -2);

    // let's tell our doctor we're coming to him.
    printf("Patient [%d] acquiring doctor_lock[%d].\n", patient_index, my_doctor_index);
    doctor_lock[my_doctor_index]->Acquire();
    printf("Patient [%d] going to see doctor [%d].\n", patient_index, my_doctor_index);
    doctor_cv[my_doctor_index]->Signal(doctor_lock[my_doctor_index]);

    // we are now with the doctor. let's give him
    // our token and wait till the consultation is over.
    // since we hold the doctor_lock right now, we know the
    // doctor will always see our token in the bucket.
    patient_doctor_bucket[my_doctor_index] = my_patient_token;
    doctor_cv[my_doctor_index]->Wait(doctor_lock[my_doctor_index]);

    printf("Patient [%d] is done with a consultation.\n", patient_index);

    // the doctor has finished his consultation with me now.
    // let's pick our diagnosis and then go pay the cashier
    // for the consultation
    my_patient_diagnosis = patient_doctor_bucket[my_doctor_index];
    patient_doctor_bucket[my_doctor_index] = -2;
    
    // inform the doctor we are leaving
    doctor_cv[my_doctor_index]->Signal(doctor_lock[my_doctor_index]);
    doctor_lock[my_doctor_index]->Release();

    cashier_line_lock->Acquire();
    number_of_patients_in_cashier_line++;
    cashier_line_populated_cv->Signal(cashier_line_lock);
    printf("****Waking up Cashier****\n");
    cashier_line_empty->Wait(cashier_line_lock);
    printf("*******SUMMONED to Counter\n");
    number_of_patients_in_cashier_line--;
    printf("***Patient [%d] will visit cashier [%d]\n", my_patient_token, cashier_line_bucket);
    my_cashier_index = cashier_line_bucket;
    cashier_line_bucket = -2;

    assert(my_cashier_index != -2);
    printf("#####################\n");
    cashier_line_lock->Release();
    

    cashier_lock[my_cashier_index]->Acquire();
    cashier_cv[my_cashier_index]->Signal(cashier_lock[my_cashier_index]);

    patient_cashier_bucket[my_cashier_index] = my_patient_token;
    cashier_cv[my_cashier_index]->Wait(cashier_lock[my_cashier_index]);
    const int my_consultation_fee = patient_cashier_bucket[my_cashier_index];
    patient_cashier_bucket[my_cashier_index] = -2;

    cashier_cv[my_cashier_index]->Signal(cashier_lock[my_cashier_index]);
    cashier_lock[my_cashier_index]->Release();

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

        next_available_doctor = available_doctor;

        // now we need a guarantee that no other door-boy will alter the
        // value of next_available_doctor while we wait for the next patient
        // to read that value. As such we will hold onto the doctor_doorboy_lock
        // so no one else can enter this critical section until after we have
        // confirmation of a read from the patient.
        doorboy_line_lock->Acquire();
        doorboy_line_empty_cv->Signal(doorboy_line_lock);
        patient_read_next_available_doctor_cv->Wait(doorboy_line_lock);

        assert(next_available_doctor == -2);

        // at this point we have confirmation that the patient has read
        // the next_available doctor and we can release our locks and wait
        // for the next patient
        doorboy_line_lock->Release();
        doctor_doorboy_lock->Release();
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
        
        // doctor break stuff


        doctor_lock[doctor_index]->Release();
    }
}

void cashier(const int cashier_index){
    printf("Cashier [%d] is ready to work.\n", cashier_index);
    while(true){
        cashier_line_lock->Acquire();
        cashier_state[cashier_index] = 0;
        while(number_of_patients_in_cashier_line == 0){
            printf("Cashier [%d] no one here, going to sleep\n", cashier_index);
           // cashier_state[cashier_index] = 0;//make available
            cashier_line_populated_cv->Wait(cashier_line_lock);
        }
        //someone in line. tell patient cashier is ready
        cashier_state[cashier_index] = 1;
        //cashier has a patient but not sure which
        //put our own index into bucket
        cashier_line_bucket = cashier_index;
        printf("Cashier [%d] told someone to come to counter.\n", cashier_index);
        cashier_line_empty->Signal(cashier_line_lock);
        printf("////////////////////////\n");
        //need to wait for responce from patient but we don't want to miss
        cashier_lock[cashier_index]->Acquire();
        printf("#####cashier lock acquired#####\n");
        cashier_line_lock->Release();
        printf("#####line lock released#####\n");

        cashier_cv[cashier_index]->Wait(cashier_lock[cashier_index]);
        printf("???????????????????????????\n");

        int patient_token = patient_cashier_bucket[cashier_index];
        int total_consultation_fee = patient_consultancy_fee[patient_token];
        printf("Cashier [%d] received token from patient [%d].\n", cashier_index, patient_token);
        patient_cashier_bucket[cashier_index] = total_consultation_fee;
        cashier_cv[cashier_index]->Signal(cashier_lock[cashier_index]);
        
        while(patient_cashier_bucket[cashier_index] != -2){
            cashier_cv[cashier_index]->Wait(cashier_lock[cashier_index]);
        }

        cashier_payment_lock->Acquire();
        total_cashier_fee += total_consultation_fee;
        cashier_payment_lock->Release();

        
        cashier_lock[cashier_index]->Release();

    }
}


void TestSuite() {
    initialize();

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

    for(int i = 0; i < NUMBER_OF_PATIENTS; i++) {
        char thread_name[32];
        sprintf(thread_name, "Patient function %d", i);
        (new Thread(thread_name))->Fork((VoidFunctionPtr)patient, i);
    }
}

#endif
