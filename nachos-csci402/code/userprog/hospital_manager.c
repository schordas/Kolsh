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
            Sprint_F(name_text,"HospitalManager reports total sales in pharmacy are [%d].\n", total_pharmacsit_money_collected);
            Print_F(name_text, sizeof(name_text));
            Lock_Release(pharmacist_payment_lock);
        }

        if(request_total_cashier_sales) {
            Lock_Acquire(cashier_payment_lock);
            Sprint_F(name_text,"Hospital manager reports total consultancy fees are [%d].\n.", total_cashier_fee);
            Print_F(name_text, sizeof(name_text));
            Lock_Release(cashier_payment_lock);
        }

        /*let's check if there is someone waiting in line for the receptionist*/
        Lock_Acquire(receptionist_line_lock);
        for(i = 0; i < NUMBER_OF_RECEPTIONISTS; i++) {
            if(receptionist_line_length[i] > 2) {
                /*signal all the receptionists to come off break*/
                Condition_Signal(receptionist_break_cv, receptionist_line_lock);
                Sprint_F(name_text,"HospitalManager signaled a Receptionist to come off break.\n");
                Print_F(name_text, sizeof(name_text));
            }
        }
        Lock_Release(receptionist_line_lock);

        /*let's check if there is someone waiting for the door-boy*/
        Lock_Acquire(doorboy_line_lock);
        if(number_of_patients_in_doorboy_line != 0) {
            /*signal all the door-boys to off break*/
            Condition_Signal(doorboy_break_cv, doorboy_line_lock);
            Sprint_F(name_text,"HospitalManager signaled a DoorBoy to come off break.\n");
            Print_F(name_text, sizeof(name_text));
        }
        Lock_Release(doorboy_line_lock);

        /*let's check if there is someone waiting in line for the cashier*/
        Lock_Acquire(cashier_line_lock);
        if(number_of_patients_in_cashier_line != 0) {
            /*signal all the cashiers to come off break*/
            Condition_Signal(cashier_break_cv, cashier_line_lock);
            Sprint_F(name_text,"HospitalManager signaled a Cashier to come off break.\n");
            Print_F(name_text, sizeof(name_text));
        }
        Lock_Release(cashier_line_lock);

        /*let's check if there is someone waiting for the pharmacist*/
        Lock_Acquire(pharmacist_line_lock);
        if(pharmacist_line_length != 0) {
           /* signal all the pharmacists to come off break*/
            Condition_Signal(pharmacist_break_cv, pharmacist_line_lock);
            Sprint_F(name_text,"HospitalManager signaled a PharmacyClerk to come off break.\n");
            Print_F(name_text, sizeof(name_text));
        }
        Lock_Release(pharmacist_line_lock);
    }
}