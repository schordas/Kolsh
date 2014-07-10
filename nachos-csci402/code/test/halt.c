/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

int main() {
    int sum = 0;
    int divisible = 5;
    int result = 0;
    int i = 0;
    
    Print_F("In userprogram, going to halt\n", sizeof("In userprogram, going to halt\n"));
    
    for(i = 0; i < 10000; i++) {
        sum = sum + i;
        result = sum / divisible;
    }

    Print_F("In userprogram, going to halt 2\n", sizeof("In userprogram, going to halt 2\n"));

    Halt();
    /* not reached */
}
