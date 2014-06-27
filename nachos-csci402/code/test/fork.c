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
    
    Fork(myfunc, "Thread 1", sizeof("Thread 1"));
    Fork(myfunc, "Thread 2", sizeof("Thread 2"));
    Fork(myfunc, "Thread 3", sizeof("Thread 3"));
    Fork(myfunc, "Thread 4", sizeof("Thread 4"));
    
    return 0;
}