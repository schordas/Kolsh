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
    Sprint_F(name_text,"Door-boy [%d] is ready to work.\n", doorboy_index);
    Print_F(name_text, sizeof(name_text));
    while(true) {
        int i;
        Lock_Acquire(doorboy_line_lock);

        while(number_of_patients_in_doorboy_line == 0) {
            Sprint_F(name_text,"Door-boy [%d] waiting for a patient to arrive.\n", doorboy_index);
            Print_F(name_text, sizeof(name_text));
            Condition_Wait(doorboy_line_full_cv,doorboy_line_lock);
        }
        number_of_patients_in_doorboy_line--;       /*// we are reserving a patient so another
                                                    // door-boy won't try and find the same
                                                    // patient a doctor as well.*/
        Lock_Release(doorboy_line_lock);

        Sprint_F(name_text,"Door-boy [%d] acquired a patient.\n", doorboy_index);
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
        Sprint_F(name_text,"Door-boy [%d] acquired doctor [%d].\n", doorboy_index, available_doctor);
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