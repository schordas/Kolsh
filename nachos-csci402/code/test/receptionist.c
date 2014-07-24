

void receptionist(const int receptionist_index) {
    char name_text[40]; 
    Sprint_F(name_text, "Receptionist [%d] is ready to work.\n", receptionist_index);
    Print_F(name_text, sizeof(name_text));
    while(true) {
        /*let's check if there is someone in line
        if there is we will tell them we are available
        if not, we will sleep till someone tells us they are in line*/
        Lock_Acquire(receptionist_line_lock);
        while(receptionist_line_length[receptionist_index] == 0) {
            /*no one is available, let's go to sleep*/
            Sprint_F(name_text, "Receptionist [%d] no one here, going to bed.\n", receptionist_index);
            Print_F(name_text, sizeof(name_text));
            receptionist_state[receptionist_index] = 0;
            condition_Wait(receptionist_line_empty_cv[receptionist_index], receptionist_line_lock);
        }
        
        /*someone is waiting in our line. Let's tell them we're ready*/
        receptionist_state[receptionist_index] = 1;
        Sprint_F(name_text, "Receptionist [%d] has signaled a Patient.\n", receptionist_index);
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
            Sprint_F(name_text, "Receptionist [%d] waiting for customer to take token.\n", receptionist_index);
            Print_F(name_text, sizeof(name_text));
            Condition_Wait(receptionist_cv[receptionist_index], receptionist_lock[receptionist_index]);
        }

        Lock_Release(receptionist_lock[receptionist_index]);

        Lock_Acquire(receptionist_line_lock);
        /*let's check if we can go on break*/
        if(receptionist_line_length == 0) {
            receptionist_state[receptionist_index] = 2;
            Sprint_F(name_text, "Receptionist [%d] is going on break.\n", receptionist_index);
            Print_F(name_text, sizeof(name_text));
            Condition_Wait(receptionist_break_cv, receptionist_line_lock);
            receptionist_state[receptionist_index] = 1;
            Sprint_F(name_text, "Receptionist [%d] is coming off break.\n", receptionist_index);
            Print_F(name_text, sizeof(name_text));
        }
        
        Lock_Release(receptionist_line_lock);
    }
}