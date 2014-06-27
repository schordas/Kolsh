#include "synch.h"


// Patient / Receptionist Interaction (While in line)
// ------------------------------------------------------------------
Lock* receptionistLineLock;            // lock required to interact with the patient waiting lines
Condition *receptionistLineCV[5];       // condition variable used while the patient is waiting in line
int receptionistLineCount[5];           // count of the number of patients in each receptionists line
int receptionistState[5];               // Receptionist State
                                        //      0: available
                                        //      1: busy
                                        //      2: on break



// Patient / Receptionist Interaction (While at the counter)
// ------------------------------------------------------------------
int receptionistToken[5];           // individual for each receptionist / patient pair
Lock* receptionistLock[5];          // individual for each receptionist / patient pair
Condition* receptionistCV[5];       // individual for each receptionist / patient pair
int nextToken = 0;                  // unique token serial number. Shared amongst all.
Lock* tokenLock;                    // lock required to work with nextToken. Shared amongst all.




// Doctor / Door Boy 
// ------------------------------------------------------------------
Lock* doctorLock[3];
Condition* doctorCV[3];
Lock*


// Doctor / Cashier
// ------------------------------------------------------------------


// Doctor / Pa
// ------------------------------------------------------------------



// data that needs to be initialized
for (int i = 0; i < 5; i++) {
    receptionistLineCount[i] = 0;
    receptionistState[i] = 1;
}


void receptionist(int index) {
    // to avoid the necessity of looking up values from the array
    // each time, we are going to create local references to save
    // us some typing.
    const int receptionist_id_ = index;
    const Lock* receptionist_lock_ = receptionistLock[receptionist_id_];
    const Condition* receptionist_line_cv_ = receptionistLineCV[receptionist_id_];
    const Condition* receptionist_cv_ = receptionistCV[receptionist_id_];


    
    while(true) {
        receptionistLineLock->Acquire();
        receptionistState[receptionist_id_] = 0;
        if(receptionistLineCount[receptionist_id_] > 0) {
            // there is a patient waiting in line for us
            receptionistLineCV[receptionist_id_]->Signal(receptionistLineLock);
            receptionistState[receptionist_id_] = 1;
        }

        receptionist_lock_ -> Acquire();                   // to ensure that we get a wait before a signal we must
        receptionistLineLock -> Release();                         // enter the second critical section before we exit the first one
        receptionistCV[receptionist_id_] -> Wait(receptionist_lock_);       // --> We are now waiting for the patient to walk up to the counter

        // at this point the patient has arrived at the counter
        // let's being the token manipulation
        tokenLock -> Acquire();
        receptionistToken[receptionist_id_] = nextToken;
        nextToken ++;
        tokenLock -> Release();

        // now it's time for the patient to read the value off the token
        receptionist_cv_ -> Signal(receptionist_lock_);       // signal the waiting patient
        receptionist_cv_ -> Wait(receptionist_lock_);         // wait for the patient to read the value

        receptionist_lock_ -> Release();
    }
}

void patient(int index) {
    const int _patientID = index;
    
    int my_token;

    receptionistLineLock -> Acquire();

    // find the shortest line for a receptionist
    int shortest_line_count = receptionistLineCount[0];
    
    
}

void doctor(int index){
	const int doctor_id _= index;
	const Thread* doctor_Thread_[3];
	const Lock* doctor_Lock_[3];
	const Condition* doctor_CV_[3];
	const Lock* room_lock_[3];
	const Condition* room_CV_[3];
}