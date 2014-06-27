#include "syscall.h"
#define NUMBER_OF_PATIENTS 10
#define NUMBER_OF_RECEPTIONISTS 5
#define DOCTORS_COUNT 3
#define DOORBOYS_COUNT 5
#define CASHIERS_COUNT 3
#define NUMBER_OF_PHARMACISTS 3
#define true 1
#define false 0




/**
 * For any status variables, the following codes will be used
 *  0: available
 *  1: busy
 *  2: on-break
 */


/*to make sense of our condition variable name, every time you see a wait
read it as such. I am waiting for the condition <condition variable name>
might be true. Every time you see a signal or broadcast read it as such.
I am notifying you that <condition variable name> might be true now.*/


/*bookkeeping data*/
int random_number_lock;
int patient_count_mutex;
int patients_in_system = NUMBER_OF_PATIENTS;

/*receptionist data*/
int receptionist_break_cv;

/*Patient Receptionist Data*/
int next_receptionist_token = 0;
int receptionist_token_bucket[NUMBER_OF_RECEPTIONISTS];
int receptionist_line_length[NUMBER_OF_RECEPTIONISTS] = { 0 };
int receptionist_state[NUMBER_OF_RECEPTIONISTS];

int receptionist_line_empty_cv[NUMBER_OF_RECEPTIONISTS];
int receptionist_line_full_cv[NUMBER_OF_RECEPTIONISTS];
int receptionist_cv[NUMBER_OF_RECEPTIONISTS];

int receptionist_line_lock;
int receptionist_lock[NUMBER_OF_RECEPTIONISTS];
int receptionist_token_access_lock;

/*door-boy data*/
int doorboy_break_cv;

/*Patient Door-boy Doctor Data*/
int doctor_state[DOCTORS_COUNT];
int number_of_patients_in_doorboy_line = 0;
int number_of_available_doctors = 0;
int next_available_doctor = -2;

int doctor_doorboy_lock;
int doorboy_line_lock;

int patient_read_next_available_doctor_cv;
int doorboy_line_empty_cv;
int doorboy_line_full_cv;
int doctor_available_cv;
int doctor_cv[DOCTORS_COUNT];


/*Patient doctor data*/
int patient_doctor_bucket[DOCTORS_COUNT];
int doctor_lock[DOCTORS_COUNT];


/*Patient doctor cashier data*/
int patient_consultancy_fee[NUMBER_OF_PATIENTS];    /*// so why does this shared data structure need not be protected
                                                    // with a lock? Since Each patient has a unique index into the
                                                    // array, and we have guarantees on the sequencing on read/write
                                                    // operations, we know only one thread will ever be accessing
                                                    // an entry at any given time.*/

/*cashier data*/
int total_cashier_fee = 0;

int cashier_break_cv;

int cashier_payment_lock;


/*patient cashier data*/
int cashier_available_count = 0;
int number_of_patients_in_cashier_line = 0;
int cashier_state[CASHIERS_COUNT];
int patient_cashier_bucket[CASHIERS_COUNT];

int cashier_line_lock;
int cashier_lock[CASHIERS_COUNT];

int cashier_line_empty_cv;
int cashier_cv[CASHIERS_COUNT];


/*pharmacist data*/
int total_pharmacsit_money_collected = 0;

int pharmacist_break_cv;

int pharmacist_payment_lock;


/*Patient pharmacist data*/
int pharmacist_available_count = 0;
int pharmacist_line_length = 0;
int pharmacist_status[NUMBER_OF_PHARMACISTS];
int to_pharmacist_patient_token_bucket[NUMBER_OF_PHARMACISTS];
int to_pharmacist_prescription_bucket[NUMBER_OF_PHARMACISTS];
int to_patient_medicine_bucket[NUMBER_OF_PHARMACISTS];
int to_patient_medicine_bill_bucket[NUMBER_OF_PHARMACISTS];
int to_pharmacist_medicine_payment_bucket[NUMBER_OF_PHARMACISTS];

int pharmacist_line_empty_cv;
int pharmacist_cv[NUMBER_OF_PHARMACISTS];

int pharmacist_line_lock;
int pharmacist_lock[NUMBER_OF_PHARMACISTS];


/*Disease data*/
const char *diseases[5] = {"is not sick", "is sick with measles", "is sick with chickenpox", "is sick with typhoid", "is sick with gangrene"};
const char *medicines[5] = {"", "ibuprofen", "vaccination shots", "antibiotics", "penicillin"};
const int consultation_cost[5] = {25, 35, 45, 55, 65};
const int medicine_costs[5] = {0, 15, 20, 25, 30};



void fill_array(int *array, int number_of_elements, int fill_value) {
    int i;
    for(i = 0; i < number_of_elements; i++) {
        array[i] = fill_value;
    }
}


void initialize() {
/*    DO NOT perform a srand() call. If nachos is using this value
    to handle its context switching we don't want to mess with it.

    initialize all of our arrays with the appropriate objects

    initialize shared patient receptionist arrays*/
    int i;
    receptionist_break_cv = Condition_Create("Receptionist break CV", sizeof("Receptionist break CV"));
    fill_array(receptionist_state, NUMBER_OF_RECEPTIONISTS, 1);
    fill_array(receptionist_token_bucket, NUMBER_OF_RECEPTIONISTS, -1);
    for(i = 0; i < NUMBER_OF_RECEPTIONISTS; i++) {
        char receptionist_cv_name[32];
        char receptionist_line_empty_cv_name[32];
        char receptionist_line_full_cv_name[32];
        char receptionist_lock_name[32];

        sprintf(receptionist_cv_name, "Receptionist CV %d", i);
        sprintf(receptionist_line_empty_cv_name, "Receptionist line empty CV %d", i);
        sprintf(receptionist_line_full_cv_name, "Receptionist line full CV %d", i);
        sprintf(receptionist_lock_name, "Receptionist lock %d",i);
        
        receptionist_cv[i] = Condition_Create(receptionist_cv_name, sizeof(receptionist_cv_name));
        receptionist_line_empty_cv[i] = Condition_Create(receptionist_line_empty_cv_name, sizeof(receptionist_line_empty_cv_name));
        receptionist_line_full_cv[i] = Condition_Create(receptionist_line_full_cv_name, sizeof(receptionist_line_full_cv_name));
        receptionist_lock[i] = Lock_Create(receptionist_lock_name, sizeof(receptionist_lock_name));
    }

/*    initialize shared patient door-boy doctor data
*/    fill_array(doctor_state, DOCTORS_COUNT, 1);
    for(i = 0; i < DOCTORS_COUNT; i++) {
        char doctor_lock_name[32];
        char doctor_cv_name[32];

        sprintf(doctor_lock_name, "Doctor lock %d", i);
        sprintf(doctor_cv_name, "Doctor CV %d", i);
        
        doctor_lock[i] = Lock_Create(doctor_lock_name, sizeof(doctor_lock_name));
        doctor_cv[i] = Condition_Create(doctor_cv_name, sizeof(doctor_cv_name));
    }

    /*initialize shared patient doctor data*/
    fill_array(patient_doctor_bucket, DOCTORS_COUNT, -2);
    fill_array(patient_consultancy_fee, NUMBER_OF_PATIENTS, -2);

    /*initialize shared cashier patient data*/
    fill_array(cashier_state, CASHIERS_COUNT, 1);
    fill_array(patient_cashier_bucket, CASHIERS_COUNT, -2);
    for(i = 0; i < CASHIERS_COUNT; i++){
        char cashier_lock_name[32];
        char cashier_cv_name[32];

        sprintf(cashier_lock_name, "Cashier lock %d", i);
        sprintf(cashier_cv_name, "Cashier CV %d", i);

        cashier_lock[i] = Lock_Create(cashier_lock_name, sizeof(cashier_lock_name));
        cashier_cv[i] = Condition_Create(cashier_cv_name, sizeof(cashier_cv_name));
    }

    /*initialize patient pharmacist data*/
    fill_array(pharmacist_status, NUMBER_OF_PHARMACISTS, 1);
    fill_array(to_pharmacist_patient_token_bucket, NUMBER_OF_PHARMACISTS, -2);
    fill_array(to_pharmacist_prescription_bucket, NUMBER_OF_PHARMACISTS, -2);
    fill_array(to_patient_medicine_bucket, NUMBER_OF_PHARMACISTS, -2);
    fill_array(to_patient_medicine_bill_bucket, NUMBER_OF_PHARMACISTS, -2);
    fill_array(to_pharmacist_medicine_payment_bucket, NUMBER_OF_PHARMACISTS, -2);

    for(i = 0; i < NUMBER_OF_PHARMACISTS; i++) {
        char pharmacist_lock_name[32];
        char pharmacist_cv_name[32];

        sprintf(pharmacist_lock_name, "Pharmacist lock %d", i);
        sprintf(pharmacist_cv_name, "Pharmacist CV %d", i);
        
        pharmacist_lock[i] = Lock_Create(pharmacist_lock_name, sizeof(pharmacist_lock_name));
        pharmacist_cv[i] = Condition_Create(pharmacist_cv_name, sizeof(pharmacist_cv_name));
    }

    random_number_lock = Lock_Create("Random number lock", sizeof("Random number lock"));
    patient_count_mutex = Lock_Create("patient count mutex", sizeof("patient count mutex"));
    receptionist_break_cv = Condition_Create("Receptionist break CV", sizeof("Receptionist break CV"));
    receptionist_line_lock = Lock_Create("receptionist line lock", sizeof("receptionist line lock"));
    receptionist_token_access_lock = Lock_Create("receptionist token access lock", sizeof("receptionist token access lock"));
    doorboy_break_cv = Condition_Create("Door-boy break CV", sizeof("Door-boy break CV"));
    doctor_doorboy_lock = Lock_Create("Doctor door-boy lock", sizeof("Doctor door-boy lock"));
    doorboy_line_lock = Lock_Create("Door-boy line lock", sizeof("Door-boy line lock"));
    patient_read_next_available_doctor_cv = Condition_Create("Patient read next available doctor CV", sizeof("Patient read next available doctor CV"));
	doorboy_line_empty_cv = Condition_Create("Door-boy line empty CV", sizeof("Door-boy line empty CV"));
	doorboy_line_full_cv = Condition_Create("Door-boy line full CV", sizeof("Door-boy line full CV"));
	doctor_available_cv = Condition_Create("Doctor available CV", sizeof("Doctor available CV"));
	cashier_break_cv = Condition_Create("Cashier break CV", sizeof("Cashier break CV"));
	cashier_payment_lock = Lock_Create("Cashier payment lock", sizeof("Cashier payment lock"));
	cashier_line_lock = Lock_Create("Cashier line lock", sizeof("Cashier line lock"));
	cashier_line_empty_cv = Condition_Create("Cashier line empty CV", sizeof("Cashier line empty CV"));
	pharmacist_break_cv = Condition_Create("Pharmacist break CV", sizeof("Pharmacist break CV"));
	pharmacist_payment_lock = Lock_Create("Pharmacist payment lock", sizeof("Pharmacist payment lock"));
	pharmacist_line_empty_cv = Condition_Create("Pharmacist line empty CV", sizeof("Pharmacist line empty CV"));
	pharmacist_line_lock = Lock_Create("Pharmacist line lock", sizeof("Pharmacist line lock"));
}

void log(char *output_message) {
    return;
}

/*Across the different functions to avoid clunky array[index]
operations, we have local references to indexed values.
Notice the trailing _ to denote they are local variables.


our service policy is as follows
Resource requesting threads (consumers), must always
wait until a resource producing thread (producer)
notifies them it is safe to continue.*/

void receptionist(const int receptionist_index) {
    char name_text[40]; 
    sprintf(name_text, "Receptionist [%d] is ready to work.\n", receptionist_index);
    Print_F(name_text, sizeof(name_text));
    while(true) {
        /*let's check if there is someone in line
        if there is we will tell them we are available
        if not, we will sleep till someone tells us they are in line*/
        Lock_Acquire(receptionist_line_lock);
        while(receptionist_line_length[receptionist_index] == 0) {
            /*no one is available, let's go to sleep*/
            sprintf(name_text, "Receptionist [%d] no one here, going to bed.\n", receptionist_index);
            Print_F(name_text, sizeof(name_text));
            receptionist_state[receptionist_index] = 0;
            condition_Wait(receptionist_line_empty_cv[receptionist_index], receptionist_line_lock);
        }
        
        /*someone is waiting in our line. Let's tell them we're ready*/
        receptionist_state[receptionist_index] = 1;
        sprintf(name_text, "Receptionist [%d] has signaled a Patient.\n", receptionist_index);
        Print_F(name_text, sizeof(name_text));
        Condition_Signal(receptionist_line_full_cv[receptionist_index], receptionist_line_lock);
        Lock_Release(receptionist_line_lock);
        
        Lock_Acquire(receptionist_lock[receptionist_index]);
        /*now we need to go get a token to give to the patient. However we need
        to be sure that bucket used to store the token is empty.*/
        while(receptionist_token_bucket[receptionist_index] != -1) {
            /*printf("We really shouldn't be getting here....\n");*/
            Condition_Wait(receptionist_cv[receptionist_index], receptionist_lock[receptionist_index]);
        }

       
        Lock_Acquire(receptionist_token_access_lock);
        receptionist_token_bucket[receptionist_index] = next_receptionist_token;
        next_receptionist_token++;
        Lock_Release(receptionist_token_access_lock);
        Condition_Signal(receptionist_cv[receptionist_index], receptionist_lock[receptionist_index]);

        /*we need to ensure our customer has gotten their token before we can continue*/
        while(receptionist_token_bucket[receptionist_index] != -1) {
            sprintf(name_text, "Receptionist [%d] waiting for customer to take token.\n", receptionist_index);
            Print_F(name_text, sizeof(name_text));
            Condition_Wait(receptionist_cv[receptionist_index], receptionist_lock[receptionist_index]);
        }

        Lock_Release(receptionist_lock[receptionist_index]);

        Lock_Acquire(receptionist_line_lock);
        /*let's check if we can go on break*/
        if(receptionist_line_length == 0) {
            receptionist_state[receptionist_index] = 2;
            sprintf(name_text, "Receptionist [%d] is going on break.\n", receptionist_index);
            Print_F(name_text, sizeof(name_text));
            Condition_Wait(receptionist_break_cv, receptionist_line_lock);
            receptionist_state[receptionist_index] = 1;
            sprintf(name_text, "Receptionist [%d] is coming off break.\n", receptionist_index);
            Print_F(name_text, sizeof(name_text));
        }
        
        Lock_Release(receptionist_line_lock);
    }
}

void patient(const int patient_index) {
    int my_medicine;
    int my_medicine_cost;
    int my_patient_diagnosis = -2;
    int my_doctor_index = -2;
    int my_patient_token = -2;
    int my_pharmacist_index = -2;
    int my_cashier_index = -2;
    int shortest_line_length = receptionist_line_length[0];
    int my_receptionist_index = 0;
    int my_consultation_fee;
    int i;
    char name_text[40]; 

    /*let's find the shortest line available to us*/
    Lock_Acquire(receptionist_line_lock);

    sprintf(name_text, "Patient [%d] has arrived at the hospital.\n", patient_index);
    Print_F(name_text, sizeof(name_text));
    for(i = 0; i < NUMBER_OF_RECEPTIONISTS; i++) {
        if (receptionist_line_length[i] == 0) {
            /*why bother checking anymore?
            we aren't going to find a shorter line*/
            my_receptionist_index = i;
            break;
        }
        else if(receptionist_line_length[i] < shortest_line_length) {
            shortest_line_length = receptionist_line_length[i];
            my_receptionist_index = i;
        }
    }

    /*irrespective of how we got here, we are always going to wait in line
    this enforces our service policy that a service requesting thread must
    always wait on a service producing thread.*/
    receptionist_line_length[my_receptionist_index] ++;
    sprintf(name_text,"Patient [%d] is waiting for Receptionist [%d].\n", patient_index, my_receptionist_index);
    Print_F(name_text, sizeof(name_text));
    /*According to our logic, the receptionist could be busy
    with another patient or could be waiting for a patient.
    If the receptionist is waiting for a patient, the signal
    will wake them up. If the receptionist is with another patient
    the receptionist loop will ensure that we still see this patient.
    The structure of how we designed this interaction gives us the liberty
    of not caring if the signal is lost.*/
    Condition_Signal(receptionist_line_empty_cv[my_receptionist_index], receptionist_line_lock);
    Condition_Wait(receptionist_line_full_cv[my_receptionist_index], receptionist_line_lock);                                                                                              

    /*at this point the receptionist has told us that they are ready for us
    let's remove ourselves from the line and be on our way*/
    receptionist_line_length[my_receptionist_index]--;
    Lock_Release(receptionist_line_lock);

    Lock_Acquire(receptionist_lock[my_receptionist_index]);
    /*let's check if the receptionist has produced a token. If he hasn't we will wait till he does.
    we will wait while the token we are looking for is -1. As soon as it's not, we know the receptionist
    delivered a token to us.*/
    while(receptionist_token_bucket[my_receptionist_index] == -1) {
        sprintf(name_text,"Patient [%d] is waiting on receptionist [%d]\n", patient_index, my_receptionist_index);
        Print_F(name_text, sizeof(name_text));
        Condition_Wait(receptionist_cv[my_receptionist_index], receptionist_lock[my_receptionist_index]);
    }

    my_patient_token = receptionist_token_bucket[my_receptionist_index];
    receptionist_token_bucket[my_receptionist_index] = -1;                  
    Condition_Signal(receptionist_cv[my_receptionist_index], receptionist_lock[my_receptionist_index]);  
    /*to inform the receptionist we took the token
    again doesn't matter if the signal gets lost
    since we have the check if token = -1 in the
    receptionist*/

    Lock_Release(receptionist_lock[my_receptionist_index]);

    sprintf(name_text,"Patient [%d] has received Token [%d] from receptionist [%d].\n", 
        patient_index, my_patient_token, my_receptionist_index);
    Print_F(name_text, sizeof(name_text));    
    /*now we are going to get in line with the door-boy
    always waiting in doctor's offices aren't we?*/
    
    Lock_Acquire(doorboy_line_lock);
    number_of_patients_in_doorboy_line++;
    sprintf(name_text,"Patient [%d] is waiting on a DoorBoy\n", my_patient_token);
    Print_F(name_text, sizeof(name_text));
    Condition_Signal(doorboy_line_full_cv, doorboy_line_lock);
    
    Condition_Wait(doorboy_line_empty_cv, doorboy_line_lock);

    /*now we need to read which doctor the door boy has assigned us*/
    my_doctor_index = next_available_doctor;

    next_available_doctor = -2;
    
    /*inform the door boy we've read the value and it's OK to move on.*/
    Condition_Signal(patient_read_next_available_doctor_cv, doorboy_line_lock);
    Lock_Release(doorboy_line_lock);

    /*we should never get to this point and my_doctor_index be == -2*/
    assert(my_doctor_index != -2);
    sprintf(name_text,"Patient [%d] has been told by Door Boy to go to examining room [%d].\n", 
        my_patient_token, my_doctor_index);
    Print_F(name_text, sizeof(name_text));
    

    /*let's tell our doctor we're coming to him.*/
    Lock_Acquire(doctor_lock[my_doctor_index]);
    sprintf(name_text,"Patient [%d] is going to examining room [%d].\n", 
        my_patient_token, my_doctor_index);
    Print_F(name_text, sizeof(name_text));
    Condition_Signal(doctor_cv[my_doctor_index], doctor_lock[my_doctor_index]);

    /*we are now with the doctor. let's give him
    our token and wait till the consultation is over.
    since we hold the doctor_lock right now, we know the
    doctor will always see our token in the bucket.*/
    patient_doctor_bucket[my_doctor_index] = my_patient_token;
    sprintf(name_text,"Patient [%d] is waiting to be examined by the doctor in Examining Room [%d].\n", 
        patient_index, my_doctor_index);
    Print_F(name_text, sizeof(name_text));
    Condition_Wait(doctor_cv[my_doctor_index], doctor_lock[my_doctor_index]);

    sprintf(name_text,"Patient [%d] is done with a consultation.\n", patient_index);
    Print_F(name_text, sizeof(name_text));

    /*the doctor has finished his consultation with me now.
    let's pick our diagnosis and then go pay the cashier
    for the consultation*/
    my_patient_diagnosis = patient_doctor_bucket[my_doctor_index];
    sprintf(name_text,"Patient [%d] [%s] in examining room [%d]",
        patient_index, diseases[my_patient_diagnosis], my_doctor_index);
    Print_F(name_text, sizeof(name_text));
    patient_doctor_bucket[my_doctor_index] = -2;
    
    /*inform the doctor we are leaving*/
    Condition_Signal(doctor_cv[my_doctor_index], doctor_lock[my_doctor_index]);
    Lock_Release(doctor_lock[my_doctor_index]);

    sprintf(name_text,"Patient [%d] is leaving examining room [%d]", patient_index, my_doctor_index);
    Print_F(name_text, sizeof(name_text));

   /* we're off to see the wizard!
    the wonderful Wizard of OZZ :)

    LOL, no, off to see the cashier*/

    Lock_Acquire(cashier_line_lock);

    number_of_patients_in_cashier_line++;

    /*let's see if there is an available cashier
    if there is, we'll find him, else we'll have to wait.*/
    if(cashier_available_count == 0) {
        while(cashier_available_count == 0) {
            Condition_Wait(cashier_line_empty_cv, cashier_line_lock);
        }
    }

    number_of_patients_in_cashier_line--;

    /*let's go find our free cashier*/
    for(i = 0; i < CASHIERS_COUNT; i++) {
        if(cashier_state[i] == 0) {
            my_cashier_index = i;
            cashier_state[i] = 1;
            cashier_available_count--;
            break;
        }
    }

    assert(my_cashier_index != -2);

    sprintf(name_text,"Patient [%d] is waiting to see cashier. [%d]\n", patient_index, my_cashier_index);
    Print_F(name_text, sizeof(name_text));
    Lock_Release(cashier_line_lock);
    
    /*let's go talk to our cashier*/
    Lock_Acquire(cashier_lock[my_cashier_index]);
    Condition_Signal(cashier_cv[my_cashier_index], cashier_lock[my_cashier_index]);

    /*let's give the cashier our data and wait*/
    patient_cashier_bucket[my_cashier_index] = my_patient_token;
    Condition_Wait(cashier_cv[my_cashier_index], cashier_lock[my_cashier_index]);
    my_consultation_fee = patient_cashier_bucket[my_cashier_index];
    patient_cashier_bucket[my_cashier_index] = -2;

    sprintf(name_text,"Patient [%d] is paying their consultancy fees of [%d]",
        patient_index, my_consultation_fee);
    Print_F(name_text, sizeof(name_text));

    Condition_Signal(cashier_cv[my_cashier_index], cashier_lock[my_cashier_index]);
    Lock_Release(cashier_lock[my_cashier_index]);


    sprintf(name_text,"Patient [%d] is leaving Cashier [%d]",
        patient_index, my_cashier_index);
    Print_F(name_text, sizeof(name_text));


    /*let's go off to the pharmacy*/
    Lock_Acquire(pharmacist_line_lock);
    
    pharmacist_line_length++;
    sprintf(name_text,"Patient [%d] is in line at the pharmacy.\n", patient_index);
    Print_F(name_text, sizeof(name_text));

    /*let's see if there is an available pharmacist
    if there is, we'll find him, else we'll have to wait*/
    if(pharmacist_available_count == 0) {
        while(pharmacist_available_count == 0) {
            Condition_Wait(pharmacist_line_empty_cv, pharmacist_line_lock);
        }
    }

    pharmacist_line_length--;

    /*let's go find our free pharmacist*/
    for(i = 0; i < NUMBER_OF_PHARMACISTS; i++) {
        if(pharmacist_status[i] == 0) {
            my_pharmacist_index = i;
            pharmacist_status[i] = 1;
            pharmacist_available_count--;
            break;
        }
    }

    assert(my_pharmacist_index != -2);

    sprintf(name_text,"Patient [%d] is waiting to see pharmacy clerk. [%d]\n", 
        patient_index, my_pharmacist_index);
    Print_F(name_text, sizeof(name_text));

    Lock_Release(pharmacist_line_lock);

    /*let's go talk directly to our pharmacist*/
    Lock_Acquire(pharmacist_lock[my_pharmacist_index]);
    Condition_Signal(pharmacist_cv[my_pharmacist_index], pharmacist_lock[my_pharmacist_index]);

    /*let's give the pharmacist our data and then wait*/
    to_pharmacist_patient_token_bucket[my_pharmacist_index] = my_patient_token;
    to_pharmacist_prescription_bucket[my_pharmacist_index] = my_patient_diagnosis;
    Condition_Wait(pharmacist_cv[my_pharmacist_index], pharmacist_lock[my_pharmacist_index]);

    my_medicine = to_patient_medicine_bucket[my_pharmacist_index];
    my_medicine_cost = to_patient_medicine_bill_bucket[my_pharmacist_index];

    sprintf(name_text,"Patient [%d] is paying their prescription fees of [%d].\n", 
        patient_index, my_medicine_cost);
    Print_F(name_text, sizeof(name_text));
    
    /*clear the buckets*/
    to_patient_medicine_bucket[my_pharmacist_index] = 0;
    to_patient_medicine_bill_bucket[my_pharmacist_index] = -2;

    /*let's pay the pharmacist and be on our way*/
    to_pharmacist_medicine_payment_bucket[my_pharmacist_index] = my_medicine_cost;

    Condition_Signal(pharmacist_cv[my_pharmacist_index], pharmacist_lock[my_pharmacist_index]);

    sprintf(name_text,"Patient [%d] is leaving pharmacy clerk [%d].\n", 
        patient_index, my_pharmacist_index);
    Print_F(name_text, sizeof(name_text));
    
    Lock_Release(pharmacist_lock[my_pharmacist_index]);

    /*that's it
    we are now free to leave the hospital!!*/

    sprintf(name_text,"Patient [%d] is leaving the Hospital.\n", patient_index);
    Print_F(name_text, sizeof(name_text));

    Lock_Acquire(patient_count_mutex);
    patients_in_system--;
    if(patients_in_system == 0) {
        sprintf(name_text,"\nALL PATIENTS EXITED THE SIMULATION\n\n");
        Print_F(name_text, sizeof(name_text));
    }
    Lock_Release(patient_count_mutex);

}

/*The patient door-boy doctor interaction is a bit interesting.
Since the door-boy works as an intermediary to allow patients
to see the doctor, we can't model this exactly like we did for
the patient - receptionist interaction.

Here is a quick overview of what is going on.
A patient will first arrive and will always wait in the door-boy
line. If we were to think of a producer consumer model, the patient
is a consumable being stored in an unbounded queue (which we model
with the doorboy_line_empty/full_cv). Now when a patient gets in line
they signal the doorboy_line_empty_cv to notify any waiting door-boy
that there is now at least one patient in line. The door-boy wakes up
and then checks if there is an available doctor. If there isn't the door-boy
must wait for a doctor to become available before signaling the patient
that they can go in to see the doctor.

After the door-boy has a guarantee that there is a one to one matching
between a patient and a doctor, they wake up the patient to inform them
a doctor is available (it is still up to the patient to figure out which 
doctor is free). The patient will then identify the available doctor and
directly signal the doctor the doctor to wake up.*/

void doorboy(const int doorboy_index) {
    char name_text[40]; 
    int available_doctor;
    /*if the number of patients in the door-boy line is 0
    the door-boy will wait on the doorboy_line_empty_cv 
    for a patient to show up. Once a patient is available,
    the door-boy will check if there are any available
    doctors. If there are, he will send the patient through.
    Otherwise, the door-boy will wait on the doctor_available_cv
    until a doctor becomes available.*/
    sprintf(name_text,"Door-boy [%d] is ready to work.\n", doorboy_index);
    Print_F(name_text, sizeof(name_text));
    while(true) {
        int i;
        Lock_Acquire(doorboy_line_lock);

        while(number_of_patients_in_doorboy_line == 0) {
            sprintf(name_text,"Door-boy [%d] waiting for a patient to arrive.\n", doorboy_index);
            Print_F(name_text, sizeof(name_text));
            Condition_Wait(doorboy_line_full_cv,doorboy_line_lock);
        }
        number_of_patients_in_doorboy_line--;       /*// we are reserving a patient so another
                                                    // door-boy won't try and find the same
                                                    // patient a doctor as well.*/
        Lock_Release(doorboy_line_lock);

        sprintf(name_text,"Door-boy [%d] acquired a patient.\n", doorboy_index);
        Print_F(name_text, sizeof(name_text));
        /*we now have a patient in our grasp muahahaha >;)*/
        Lock_Acquire(doctor_doorboy_lock);
        while(number_of_available_doctors == 0) {
            Condition_Wait(doctor_available_cv, doctor_doorboy_lock);
        }

        /*now we need to find a free doctor*/
        available_doctor = -2;
        for(i = 0; i < DOCTORS_COUNT; i++) {
            if(doctor_state[i] == 0) {
                available_doctor = i;
                doctor_state[i] = 1;
                number_of_available_doctors--;
                break;
            }
        }
        assert(available_doctor != -2);
        sprintf(name_text,"Door-boy [%d] acquired doctor [%d].\n", doorboy_index, available_doctor);
        Print_F(name_text, sizeof(name_text));

        /*now we need a guarantee that no other door-boy will alter the
        value of next_available_doctor while we wait for the next patient
        to read that value. As such we will hold onto the doctor_doorboy_lock
        so no one else can enter this critical section until after we have
        confirmation of a read from the patient.*/
        Lock_Acquire(doorboy_line_lock);
        next_available_doctor = available_doctor;
        Condition_Signal(doorboy_line_empty_cv, doorboy_line_lock);
        Condition_Wait(patient_read_next_available_doctor_cv, doorboy_line_lock);

        assert(next_available_doctor == -2);

        /*at this point we have confirmation that the patient has read
        the next_available doctor and we can release our locks and wait
        for the next patient*/
        
        Lock_Release(doctor_doorboy_lock);

        /*we will hold off for just a second
        to release our line lock to see if,
        we can go on break.*/
        if(number_of_patients_in_doorboy_line == 0) {
            /*let's go on break*/
            Condition_Wait(doorboy_break_cv, doorboy_line_lock);
        }
        Lock_Acquire(doorboy_line_lock);
    }
}


void doctor(const int doctor_index) {
    char name_text[40]; 
    int i;
    int patient_token;
    int consultation_time;
    int patient_sickness;
    int take_break;
    sprintf(name_text,"Doctor [%d] is ready to work.\n", doctor_index);
    Print_F(name_text, sizeof(name_text));
    while(true) {
        /*first we need to set the doctor state without 
        other threads getting in the way*/
        Lock_Acquire(doctor_doorboy_lock);
        doctor_state[doctor_index] = 0;

        number_of_available_doctors++;
        Condition_Signal(doctor_available_cv, doctor_doorboy_lock);
        
        sprintf(name_text,"Doctor [%d] acquiring doctor_lock.\n", doctor_index);    
        Print_F(name_text, sizeof(name_text));   
        Lock_Acquire(doctor_lock[doctor_index]); /*// we need to ensure that this doctor  */  
        Lock_Release(doctor_doorboy_lock); /*// will actually be waiting for their patient*/
        
        sprintf(name_text,"Doctor [%d] waiting for a patient.\n", doctor_index);
        Print_F(name_text, sizeof(name_text));
        Condition_Wait(doctor_cv[doctor_index], doctor_lock[doctor_index]);

        patient_token = patient_doctor_bucket[doctor_index];    /*// we will not clear out this value since we going to 
                                                                    // write back to the bucket before we give up the lock.*/
        
        sprintf(name_text,"Doctor [%d] is examining a Patient with Token [%d].\n", doctor_index, patient_token);
        Print_F(name_text, sizeof(name_text));
        Lock_Acquire(random_number_lock);
        consultation_time = (rand() % 20) + 10;
        patient_sickness = (rand() % 5);
        take_break = (rand() % 2);
        Lock_Release(random_number_lock);

        /*now that we have a patient, let's examine them*/
        for(i = 0; i < consultation_time; i++) {
        	Yield();
        }

        sprintf(name_text,"Doctor [%d] has determined that the Patient with Token [%d] [%s].\n",
            doctor_index, patient_token, diseases[patient_sickness]);
        Print_F(name_text, sizeof(name_text));
        if(patient_sickness != 0) {
            sprintf(name_text,"Doctor [%d] is prescribing [%s] to the Patient with Token [%d].\n",
                doctor_index, medicines[patient_sickness], patient_token);
            Print_F(name_text, sizeof(name_text));
        }

        /*put the patient sickness in the patient_illness_bucket
        and the patient_doctor_bucket. This is to inform the 
        cashier of the consultation fee and to tell the patient
        what is wrong with them so they can get the right medicine
        at the pharmacy.*/
        patient_doctor_bucket[doctor_index] = patient_sickness;
        patient_consultancy_fee[patient_token] = patient_sickness;

        /*inform the patient we're done and wait
        for a guarantee that the patient has
        seen their diagnosis and left.*/
        Condition_Signal(doctor_cv[doctor_index], doctor_lock[doctor_index]);
        while(patient_doctor_bucket[doctor_index] != -2) {
            Condition_Wait(doctor_cv[doctor_index], doctor_lock[doctor_index]);
        }
        
        Lock_Release(doctor_lock[doctor_index]);
    }
}


void cashier(const int cashier_index){
    char name_text[40]; 
    int my_patient_token = -2;
    int total_consultation_fee;
    sprintf(name_text,"Cashier [%d] is ready to work.\n", cashier_index);
    Print_F(name_text, sizeof(name_text));
    while(true){


        /*the cashier line mechanism is the same as the
        pharmacists. Consult the comments for the 
        pharmacist for more details.*/
        Lock_Acquire(cashier_line_lock);

        if(number_of_patients_in_cashier_line == 0) {
            /*let's go on break*/
            cashier_state[cashier_index] = 2;
            Condition_Wait(cashier_break_cv, cashier_line_lock);
        }

        cashier_state[cashier_index] = 0;
        cashier_available_count++;
        sprintf(name_text,"Cashier [%d] has signaled a Patient", cashier_index);
        Print_F(name_text, sizeof(name_text));
        Condition_Signal(cashier_line_empty_cv, cashier_line_lock);

        /*transition to the next critical
        section before moving out of this one.*/
        Lock_Acquire(cashier_lock[cashier_index]);
        Lock_Release(cashier_line_lock);
        /*now we wait for a customer*/
        Condition_Wait(cashier_cv[cashier_index], cashier_lock[cashier_index]);

        /*let's gather the patient data*/
        my_patient_token = patient_cashier_bucket[cashier_index];
        assert(my_patient_token != -2);
        sprintf(name_text,"Cashier [%d] gets Token [%d] from a Patient.\n", cashier_index, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        /*let's send this data back to the patient*/
        total_consultation_fee = patient_consultancy_fee[my_patient_token];
        patient_cashier_bucket[cashier_index] = total_consultation_fee;
        sprintf(name_text,"Cashier [%d] tells Patient with Token [%d] they owe $%d\n", cashier_index, my_patient_token, total_consultation_fee);
        Print_F(name_text, sizeof(name_text));
        Condition_Signal(cashier_cv[cashier_index], cashier_lock[cashier_index]);

        /*to ensure the patient saw the amount, we are going
        to wait until they clear the patient_cashier_bucket.
        Their viewing of the payment amount is consent for us
        to charge their card that we have on file. Therefore
        we don't need another interaction with the payment.*/
        while(patient_cashier_bucket[cashier_index] != -2){
            Condition_Wait(cashier_cv[cashier_index], cashier_lock[cashier_index]);
        }

        Lock_Acquire(cashier_payment_lock);
        total_cashier_fee += total_consultation_fee;
        sprintf(name_text,"Cashier [%d] receives fees from Patient with Token [%d] from a Patient.\n", cashier_index, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        Lock_Release(cashier_payment_lock);
        /*we're now done with this customer     */   
        Lock_Release(cashier_lock[cashier_index]);
    }
}


void pharmacist(const int pharmacist_index) {
    char name_text[40]; 
    int payment_for_medicine;
    char *patient_medicine;
    int patient_medicine_cost;
    int my_patient_prescription;
    sprintf(name_text,"Pharmacist [%d] is ready to work.\n", pharmacist_index);
    Print_F(name_text, sizeof(name_text));
    while(true) {

        int my_patient_token = -2;

        /*we are going to wait for the patient to come up to us
        it is easier to manage initial transfer of data to the
        pharmacist because it can be done in the pharmacist / patient
        critical section, we don't need to add synchronization bloat
        to the line area.*/
        Lock_Acquire(pharmacist_line_lock);

        if(pharmacist_line_length == 0) {
            /*we're going on break*/
            pharmacist_status[pharmacist_index] = 2;
            Condition_Wait(pharmacist_break_cv, pharmacist_line_lock);
        }

        pharmacist_status[pharmacist_index] = 0;
        pharmacist_available_count++;
        Condition_Signal(pharmacist_line_empty_cv, pharmacist_line_lock);
        /*we need to make sure that we are ready for the
        patient's signal before it is issued. Therefore
        we need to enter the second critical section
        before we release the line_lock.*/
        Lock_Acquire(pharmacist_lock[pharmacist_index]);
        Lock_Release(pharmacist_line_lock);

        /*now let's wait for a customer to come up to us*/
        Condition_Wait(pharmacist_cv[pharmacist_index], pharmacist_lock[pharmacist_index]);
        
        /*now we know the patient has arrived and we can proceed
        we also have a guarantee that the patient diagnosis is
        waiting for us in our bucket.*/
        my_patient_token = to_pharmacist_patient_token_bucket[pharmacist_index];
        my_patient_prescription = to_pharmacist_prescription_bucket[pharmacist_index];
        *patient_medicine = medicines[my_patient_prescription];
        patient_medicine_cost = medicine_costs[my_patient_prescription];

        assert(my_patient_token != -2);

        /*clear the buckets*/
        to_pharmacist_prescription_bucket[pharmacist_index] = -2;

        sprintf(name_text,"Pharmacist [%d] got prescription [%s] from patient with token [%d].\n", 
            pharmacist_index, patient_medicine, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        sprintf(name_text,"Pharmacist [%d] gives prescription [%s] to patient with token [%d].\n", 
            pharmacist_index, patient_medicine, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        sprintf(name_text,"Pharmacist [%d] tells patient with token [%d] they owe [%d].\n", 
            pharmacist_index, my_patient_token, patient_medicine_cost);
        Print_F(name_text, sizeof(name_text));

        /*let's send this data back to the patient*/
        to_patient_medicine_bucket[pharmacist_index] = my_patient_prescription;
        to_patient_medicine_bill_bucket[pharmacist_index] = patient_medicine_cost;

        /*now we let the patient know we're done
        and we wait for the payment response.*/
        Condition_Signal(pharmacist_cv[pharmacist_index], pharmacist_lock[pharmacist_index]);
        Condition_Wait(pharmacist_cv[pharmacist_index], pharmacist_lock[pharmacist_index]);

        sprintf(name_text,"Pharmacist [%d] received a response from patient [%d].\n",
            pharmacist_index, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        
        /*we now have a response from the patient with their payment*/
        Lock_Acquire(pharmacist_payment_lock);
        payment_for_medicine = to_pharmacist_medicine_payment_bucket[pharmacist_index];
        assert(payment_for_medicine != -2);

        sprintf(name_text,"Pharmacist [%d] gets money from patient with token [%d].\n", 
            pharmacist_index, my_patient_token);
        Print_F(name_text, sizeof(name_text));

        total_pharmacsit_money_collected += payment_for_medicine;
        to_pharmacist_medicine_payment_bucket[pharmacist_index] = -2;
        Lock_Release(pharmacist_payment_lock);

        Lock_Release(pharmacist_lock[pharmacist_index]);
    }
}

void hospital_manager(const int hospital_manager_index) {
    char name_text[40]; 
    int continue_running = true;
    int wait_time;
    int request_total_cashier_sales;
    int request_total_pharmacy_sales;
    while(true) {
        int i;
        /*pick a random amount of time to wait*/
        Lock_Acquire(random_number_lock);
        wait_time = (rand() % 75) + 25;
        request_total_cashier_sales = ((rand() % 2) == 0);
        request_total_pharmacy_sales = ((rand() % 2) == 0);
        Lock_Release(random_number_lock);
        
        for(i = 0; i < wait_time; i++) {
            Yield();
        }

        /*first let's check if our simulation is done*/
        Lock_Acquire(patient_count_mutex);
        if(patients_in_system == 0) {
            /*we can't return here to terminate the thread.
            Why? Because if we do, we will leak the lock.
            The thread the currently owns it (this) will terminate
            without ever calling release. So everyone else waiting
            to acquire this lock is screwed.*/
            continue_running = false;
        }
        Lock_Release(patient_count_mutex);
        if(!continue_running) {
            return;
        }

        /*now we hop in randomly to cause some ruckus*/

        if(request_total_pharmacy_sales) {
            Lock_Acquire(pharmacist_payment_lock);
            sprintf(name_text,"HospitalManager reports total sales in pharmacy are [%d].\n", total_pharmacsit_money_collected);
            Print_F(name_text, sizeof(name_text));
            Lock_Release(pharmacist_payment_lock);
        }

        if(request_total_cashier_sales) {
            Lock_Acquire(cashier_payment_lock);
            sprintf(name_text,"Hospital manager reports total consultancy fees are [%d].\n.", total_cashier_fee);
            Print_F(name_text, sizeof(name_text));
            Lock_Release(cashier_payment_lock);
        }

        /*let's check if there is someone waiting in line for the receptionist*/
        Lock_Acquire(receptionist_line_lock);
        for(i = 0; i < NUMBER_OF_RECEPTIONISTS; i++) {
            if(receptionist_line_length[i] > 2) {
                /*signal all the receptionists to come off break*/
                Condition_Signal(receptionist_break_cv, receptionist_line_lock);
                sprintf(name_text,"HospitalManager signaled a Receptionist to come off break.\n");
                Print_F(name_text, sizeof(name_text));
            }
        }
        Lock_Release(receptionist_line_lock);

        /*let's check if there is someone waiting for the door-boy*/
        Lock_Acquire(doorboy_line_lock);
        if(number_of_patients_in_doorboy_line != 0) {
            /*signal all the door-boys to off break*/
            Condition_Signal(doorboy_break_cv, doorboy_line_lock);
            sprintf(name_text,"HospitalManager signaled a DoorBoy to come off break.\n");
            Print_F(name_text, sizeof(name_text));
        }
        Lock_Release(doorboy_line_lock);

        /*let's check if there is someone waiting in line for the cashier*/
        Lock_Acquire(cashier_line_lock);
        if(number_of_patients_in_cashier_line != 0) {
            /*signal all the cashiers to come off break*/
            Condition_Signal(cashier_break_cv, cashier_line_lock);
            sprintf(name_text,"HospitalManager signaled a Cashier to come off break.\n");
            Print_F(name_text, sizeof(name_text));
        }
        Lock_Release(cashier_line_lock);

        /*let's check if there is someone waiting for the pharmacist*/
        Lock_Acquire(pharmacist_line_lock);
        if(pharmacist_line_length != 0) {
           /* signal all the pharmacists to come off break*/
            Condition_Signal(pharmacist_break_cv, pharmacist_line_lock);
            sprintf(name_text,"HospitalManager signaled a PharmacyClerk to come off break.\n");
            Print_F(name_text, sizeof(name_text));
        }
        Lock_Release(pharmacist_line_lock);
    }
}



void HmSimulation() {
    int i;
    char name_text[40];
    char hm_name[32]; 
    initialize();
    sprintf(name_text,"Number of Receptionists = [%d]\n", NUMBER_OF_RECEPTIONISTS);
    Print_F(name_text, sizeof(name_text));
    sprintf(name_text,"Number of Doctors = [%d]\n", DOCTORS_COUNT);
    Print_F(name_text, sizeof(name_text));
    sprintf(name_text,"Number of DoorBoys = [%d]\n", DOORBOYS_COUNT);
    Print_F(name_text, sizeof(name_text));
    sprintf(name_text,"Number of Cashiers = [%d]\n", CASHIERS_COUNT);
    Print_F(name_text, sizeof(name_text));
    sprintf(name_text,"Number of PharmacyClerks = [%d]\n", NUMBER_OF_PHARMACISTS);
    Print_F(name_text, sizeof(name_text));
    sprintf(name_text,"Number of Patients = [%d]\n", NUMBER_OF_PATIENTS);
    Print_F(name_text, sizeof(name_text));

    /*all shared data has been initialized at this point
    the initialize function solves the issue of threads
    accessing shared data before it has all been initialized*/

    
    for(i = 0; i < NUMBER_OF_RECEPTIONISTS; i++) {
        char thread_name[32];
        sprintf(thread_name, "Receptionist function %d", i);
        Fork(receptionist, thread_name, sizeof(thread_name));
    }

    for(i = 0; i < DOORBOYS_COUNT; i++) {
        char thread_name[32];
        sprintf(thread_name, "Door-boy function %d", i);
        Fork(doorboy, thread_name, sizeof(thread_name));
    }

    for(i = 0; i < DOCTORS_COUNT; i++) {
        char thread_name[32];
        sprintf(thread_name, "Doctor function %d", i);
        Fork(doctor, thread_name, sizeof(thread_name));
    }

    for(i = 0; i < CASHIERS_COUNT; i++){
        char thread_name[32];
        sprintf(thread_name, "Cashier function %d", i);
        Fork(cashier, thread_name, sizeof(thread_name));
    }

    for(i = 0; i < NUMBER_OF_PHARMACISTS; i++) {
        char thread_name[32];
        sprintf(thread_name, "Pharmacist function %d", i);
        Fork(pharmacist, thread_name, sizeof(thread_name));
    }
    
    sprintf(hm_name, "HospitalManager function");
    Fork(hospital_manager, hm_name, sizeof(hm_name));

    for(i = 0; i < NUMBER_OF_PATIENTS; i++) {
        char thread_name[32];
        sprintf(thread_name, "Patient function %d", i);
        Fork(patient, thread_name, sizeof(thread_name));
    }
}
