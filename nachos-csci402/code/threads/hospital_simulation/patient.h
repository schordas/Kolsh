/**
 * Project: Kolsh (CS_402)
 * Date: Tuesday, June 10, 2014
 * Description: Patient class for hospital management simulation
 */

#ifndef HOSPITAL_SIMULATION_PATIENT_H_
#define HOSPITAL_SIMULATION_PATIENT_H_



class Patient {
public:
    Patient(char* name);
    ~Patient();
    void run();

private:
    char* name;
    int my_token_
    int shortest_receptionist_line_length_
    int shortest_receptionist_line_index_
};

#endif // HOSPITAL_SIMULATION_PATIENT_H_

