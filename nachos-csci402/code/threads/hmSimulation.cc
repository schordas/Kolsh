#include "synch.h"


// Patient / Receptionist Interaction (While in line)
// ------------------------------------------------------------------
Lock* receptioninstLineLock;
Condition *receptionistLineCV[5];
int receptionistLineCount[5];
int receptionistState[5];       /**
                                 * Receptionist State
                                 *  0: available
                                 *  1: busy
                                 *  2: on break
                                 */



// Patient / Receptionist Interaction (While at the counter)
// ------------------------------------------------------------------
int receptionistToken[5];
Lock* receptionistLock[5];
Condition* receptionistCV[5];
int nextToken = 0;
Lock* tokenLock;



// let's initialize all of our global data
for (int i = 0; i < 5; i++) {
    receptionistLineCount[i] = 0;
    receptionistState[i] = 1;
}

void receptionist(int index) {
    
}