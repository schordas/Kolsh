/* fork.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 */

#include "syscall.h"

int a[3];



int myfunc(){
	my_printf("Inside myfunc\n", 20);
	return 0;
}

int
main()
{
	char* text = "Hello";
	my_printf("In userprogram, going to fork\n", 40);
    Fork(myfunc, text);
	
	my_printf("In userprogram, after fork\n", 40);
    /* not reached */
	return 0;
}

