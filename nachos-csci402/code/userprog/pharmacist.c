void pharmacist(const int pharmacist_index) {
    char name_text[40]; 
    int payment_for_medicine;
    char *patient_medicine;
    int patient_medicine_cost;
    int my_patient_prescription;
    Sprint_F(name_text,"Pharmacist [%d] is ready to work.\n", pharmacist_index);
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

        Sprint_F(name_text,"Pharmacist [%d] got prescription [%s] from patient with token [%d].\n", 
            pharmacist_index, patient_medicine, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        Sprint_F(name_text,"Pharmacist [%d] gives prescription [%s] to patient with token [%d].\n", 
            pharmacist_index, patient_medicine, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        Sprint_F(name_text,"Pharmacist [%d] tells patient with token [%d] they owe [%d].\n", 
            pharmacist_index, my_patient_token, patient_medicine_cost);
        Print_F(name_text, sizeof(name_text));

        /*let's send this data back to the patient*/
        to_patient_medicine_bucket[pharmacist_index] = my_patient_prescription;
        to_patient_medicine_bill_bucket[pharmacist_index] = patient_medicine_cost;

        /*now we let the patient know we're done
        and we wait for the payment response.*/
        Condition_Signal(pharmacist_cv[pharmacist_index], pharmacist_lock[pharmacist_index]);
        Condition_Wait(pharmacist_cv[pharmacist_index], pharmacist_lock[pharmacist_index]);

        Sprint_F(name_text,"Pharmacist [%d] received a response from patient [%d].\n",
            pharmacist_index, my_patient_token);
        Print_F(name_text, sizeof(name_text));
        
        /*we now have a response from the patient with their payment*/
        Lock_Acquire(pharmacist_payment_lock);
        payment_for_medicine = to_pharmacist_medicine_payment_bucket[pharmacist_index];
        assert(payment_for_medicine != -2);

        Sprint_F(name_text,"Pharmacist [%d] gets money from patient with token [%d].\n", 
            pharmacist_index, my_patient_token);
        Print_F(name_text, sizeof(name_text));

        total_pharmacsit_money_collected += payment_for_medicine;
        to_pharmacist_medicine_payment_bucket[pharmacist_index] = -2;
        Lock_Release(pharmacist_payment_lock);

        Lock_Release(pharmacist_lock[pharmacist_index]);
    }
}