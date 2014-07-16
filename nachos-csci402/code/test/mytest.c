/* mytest.c
 *    Test program to do matrix multiplication on large arrays.
 *
 *    Intended to stress virtual memory system.
 *
 *    Ideally, we could read the matrices off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

#define Dim 	20	/* sum total of the arrays doesn't fit in 
			 * physical memory 
			 */



int
main()
{
	int number = 232;
	char* text = "Hello\n";
	Print_F(text,10);
	Print_F(number,10);
	Print_F(text,10);

    Exit(0);		/* and then we're done */
}
