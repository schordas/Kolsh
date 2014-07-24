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

    Sprint_F(name_text, "Patient [%d] has arrived at the hospital.\n", patient_index);
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
    Sprint_F(name_text,"Patient [%d] is waiting for Receptionist [%d].\n", patient_index, my_receptionist_index);
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
        Sprint_F(name_text,"Patient [%d] is waiting on receptionist [%d]\n", patient_index, my_receptionist_index);
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

    Sprint_F(name_text,"Patient [%d] has received Token [%d] from receptionist [%d].\n", 
        patient_index, my_patient_token, my_receptionist_index);
    Print_F(name_text, sizeof(name_text));    
    /*now we are going to get in line with the door-boy
    always waiting in doctor's offices aren't we?*/
    
    Lock_Acquire(doorboy_line_lock);
    number_of_patients_in_doorboy_line++;
    Sprint_F(name_text,"Patient [%d] is waiting on a DoorBoy\n", my_patient_token);
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
    Sprint_F(name_text,"Patient [%d] has been told by Door Boy to go to examining room [%d].\n", 
        my_patient_token, my_doctor_index);
    Print_F(name_text, sizeof(name_text));
    

    /*let's tell our doctor we're coming to him.*/
    Lock_Acquire(doctor_lock[my_doctor_index]);
    Sprint_F(name_text,"Patient [%d] is going to examining room [%d].\n", 
        my_patient_token, my_doctor_index);
    Print_F(name_text, sizeof(name_text));
    Condition_Signal(doctor_cv[my_doctor_index], doctor_lock[my_doctor_index]);

    /*we are now with the doctor. let's give him
    our token and wait till the consultation is over.
    since we hold the doctor_lock right now, we know the
    doctor will always see our token in the bucket.*/
    patient_doctor_bucket[my_doctor_index] = my_patient_token;
    Sprint_F(name_text,"Patient [%d] is waiting to be examined by the doctor in Examining Room [%d].\n", 
        patient_index, my_doctor_index);
    Print_F(name_text, sizeof(name_text));
    Condition_Wait(doctor_cv[my_doctor_index], doctor_lock[my_doctor_index]);

    Sprint_F(name_text,"Patient [%d] is done with a consultation.\n", patient_index);
    Print_F(name_text, sizeof(name_text));

    /*the doctor has finished his consultation with me now.
    let's pick our diagnosis and then go pay the cashier
    for the consultation*/
    my_patient_diagnosis = patient_doctor_bucket[my_doctor_index];
    Sprint_F(name_text,"Patient [%d] [%s] in examining room [%d]",
        patient_index, diseases[my_patient_diagnosis], my_doctor_index);
    Print_F(name_text, sizeof(name_text));
    patient_doctor_bucket[my_doctor_index] = -2;
    
    /*inform the doctor we are leaving*/
    Condition_Signal(doctor_cv[my_doctor_index], doctor_lock[my_doctor_index]);
    Lock_Release(doctor_lock[my_doctor_index]);

    Sprint_F(name_text,"Patient [%d] is leaving examining room [%d]", patient_index, my_doctor_index);
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

    Sprint_F(name_text,"Patient [%d] is waiting to see cashier. [%d]\n", patient_index, my_cashier_index);
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

    Sprint_F(name_text,"Patient [%d] is paying their consultancy fees of [%d]",
        patient_index, my_consultation_fee);
    Print_F(name_text, sizeof(name_text));

    Condition_Signal(cashier_cv[my_cashier_index], cashier_lock[my_cashier_index]);
    Lock_Release(cashier_lock[my_cashier_index]);


    Sprint_F(name_text,"Patient [%d] is leaving Cashier [%d]",
        patient_index, my_cashier_index);
    Print_F(name_text, sizeof(name_text));


    /*let's go off to the pharmacy*/
    Lock_Acquire(pharmacist_line_lock);
    
    pharmacist_line_length++;
    Sprint_F(name_text,"Patient [%d] is in line at the pharmacy.\n", patient_index);
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

    Sprint_F(name_text,"Patient [%d] is waiting to see pharmacy clerk. [%d]\n", 
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

    Sprint_F(name_text,"Patient [%d] is paying their prescription fees of [%d].\n", 
        patient_index, my_medicine_cost);
    Print_F(name_text, sizeof(name_text));
    
    /*clear the buckets*/
    to_patient_medicine_bucket[my_pharmacist_index] = 0;
    to_patient_medicine_bill_bucket[my_pharmacist_index] = -2;

    /*let's pay the pharmacist and be on our way*/
    to_pharmacist_medicine_payment_bucket[my_pharmacist_index] = my_medicine_cost;

    Condition_Signal(pharmacist_cv[my_pharmacist_index], pharmacist_lock[my_pharmacist_index]);

    Sprint_F(name_text,"Patient [%d] is leaving pharmacy clerk [%d].\n", 
        patient_index, my_pharmacist_index);
    Print_F(name_text, sizeof(name_text));
    
    Lock_Release(pharmacist_lock[my_pharmacist_index]);

    /*that's it
    we are now free to leave the hospital!!*/

    Sprint_F(name_text,"Patient [%d] is leaving the Hospital.\n", patient_index);
    Print_F(name_text, sizeof(name_text));

    Lock_Acquire(patient_count_mutex);
    patients_in_system--;
    if(patients_in_system == 0) {
        Sprint_F(name_text,"\nALL PATIENTS EXITED THE SIMULATION\n\n");
        Print_F(name_text, sizeof(name_text));
    }
    Lock_Release(patient_count_mutex);

}