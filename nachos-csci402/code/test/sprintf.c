#include "syscall.h"


int
main()
{
	char receptionist_lock_name[32];
	char *name = "Forked Func %d";
		/* Positive Test */
	Print_F("In userprogram, going to sprinf\n", 40);
    Sprint_f(receptionist_lock_name, name, 123);
	Print_F(receptionist_lock_name, sizeof(receptionist_lock_name));
	Print_F("In userprogram, after sprinf\n", 40);

	/* Negative Test */

    /* not reached */
	return 0;
}

