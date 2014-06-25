/* fork.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 */

#include <stdio.h>
#include "syscall.h"


int myfunc(){
	return 0;
}

int main() {

    printf("hello world.\n");

	char* text = "Hello";
    Fork(myfunc, text);
	
	
    /* not reached */
	return 0;
}

