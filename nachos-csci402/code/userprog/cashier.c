void cashier(const int cashier_index){
    char name_text[40]; 
    int my_patient_token = -2;
    int total_consultation_fee;
    Sprint_F(name_text,"Cashier [%d] is ready to work.\n", cashier_index);
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
        Sprint_F(name_text,"Cashier [%d] has signaled a Patient", cashier_index);
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
        Sprint_F(name_text,"Cashier [%d] gets Token [%d] from a Patient.\n", cashier_index, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        /*let's send this data back to the patient*/
        total_consultation_fee = patient_consultancy_fee[my_patient_token];
        patient_cashier_bucket[cashier_index] = total_consultation_fee;
        Sprint_F(name_text,"Cashier [%d] tells Patient with Token [%d] they owe $%d\n", cashier_index, my_patient_token, total_consultation_fee);
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
        Sprint_F(name_text,"Cashier [%d] receives fees from Patient with Token [%d] from a Patient.\n", cashier_index, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        Lock_Release(cashier_payment_lock);
        /*we're now done with this customer     */   
        Lock_Release(cashier_lock[cashier_index]);
    }
}