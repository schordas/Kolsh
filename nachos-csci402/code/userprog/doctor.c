void doctor(const int doctor_index) {
    char name_text[40]; 
    int i;
    int patient_token;
    int consultation_time;
    int patient_sickness;
    int take_break;
    Sprint_F(name_text,"Doctor [%d] is ready to work.\n", doctor_index);
    Print_F(name_text, sizeof(name_text));
    while(true) {
        /*first we need to set the doctor state without 
        other threads getting in the way*/
        Lock_Acquire(doctor_doorboy_lock);
        doctor_state[doctor_index] = 0;

        number_of_available_doctors++;
        Condition_Signal(doctor_available_cv, doctor_doorboy_lock);
        
        Sprint_F(name_text,"Doctor [%d] acquiring doctor_lock.\n", doctor_index);    
        Print_F(name_text, sizeof(name_text));   
        Lock_Acquire(doctor_lock[doctor_index]); /*// we need to ensure that this doctor  */  
        Lock_Release(doctor_doorboy_lock); /*// will actually be waiting for their patient*/
        
        Sprint_F(name_text,"Doctor [%d] waiting for a patient.\n", doctor_index);
        Print_F(name_text, sizeof(name_text));
        Condition_Wait(doctor_cv[doctor_index], doctor_lock[doctor_index]);

        patient_token = patient_doctor_bucket[doctor_index];    /*// we will not clear out this value since we going to 
                                                                    // write back to the bucket before we give up the lock.*/
        
        Sprint_F(name_text,"Doctor [%d] is examining a Patient with Token [%d].\n", doctor_index, patient_token);
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

        Sprint_F(name_text,"Doctor [%d] has determined that the Patient with Token [%d] [%s].\n",
            doctor_index, patient_token, diseases[patient_sickness]);
        Print_F(name_text, sizeof(name_text));
        if(patient_sickness != 0) {
            Sprint_F(name_text,"Doctor [%d] is prescribing [%s] to the Patient with Token [%d].\n",
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