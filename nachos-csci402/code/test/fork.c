/* fork.c
 *  Simple program to test whether running a user program works.
 *  
 *  Just do a "syscall" that shuts down the OS.
 *
 */

#include "syscall.h"


int myfunc() {
    Print_F("hello world from a thread.\n", sizeof("hello world from a thread.\n"));
    return 0;
}

int main() {
    
    Print_F("hello world.\n", sizeof("hello world.\n"));
    
    Fork(myfunc, "Hello", sizeof("Hello"));
    
    return 0;
}
