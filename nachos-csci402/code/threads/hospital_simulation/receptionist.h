/**
 * Project: Kolsh (CS_402)
 * Date: Tuesday, June 10, 2014
 * Description: Receptionist class for hospital management simulation
 */

#ifndef HOSPITAL_SIMULATION_RECEPTIONIST_H_
#define HOSPITAL_SIMULATION_RECEPTIONIST_H_

 class Receptionist {
 public:
 	Receptionist(char* debug_name_);
 	~Receptionist();
 	void run();
 private:
 	char* name;
 	int receptionist_state_;
 	int receptionist_token_;
 	int receptionist_line_count_;
 	Lock* receptionsit_lock_;
 	Condition* receptionist_cv_;
 	Lock * receptionist_line_lock_;
 };

 #endif // HOSPITAL_SIMULATION_RECEPTIONIST_H_