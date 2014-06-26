/* exit.c
 *	Simple program to test whether running a user program works.
 *	
 *	exiting a thread 
 *
 */

#include "syscall.h"

int forked(){
	my_printf("In forked function, now going to exit\n", 40);
	Exit(0);
}

int
main()
{
	int return_id;
	char *name = "Forked Func";
	/* Positive Test */
	my_printf("In userprogram, going to fork\n", 40);
    Fork(forked, name);
	my_printf("In userprogram, after fork\n", 40);
	/* Negative Test */

    /* not reached */
	return 0;
}

