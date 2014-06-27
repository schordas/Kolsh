/* exec.c
 *  Simple program to test whether running a user program works.
 *  
 *  Opening a new process
 *
 */

#include "syscall.h"

int
main()
{
	int return_id;
	/* Positive Test */
	Print_F("In userprogram, going to exec\n", 40);
    return_id = Exec("../test/fork", sizeof("../test/fork"));
	Print_F("In userprogram, after exec\n", 40);
	/* Negative Test */

    /* not reached */
    return 0;
}

