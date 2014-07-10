/**
 * fork.c
 */

#include "syscall.h"

void my_thread() {
    Print_F("hello from a Thread\n", sizeof("hello from a Thread\n"));
    Exit(0);
}

int main() {

    Print_F("hello from a user-program\n", sizeof("hello from a user-program\n"));

    Print_F("Forking Thread 0\n", sizeof("Forking Thread 0\n"));
    Fork(my_thread);
    Print_F("Forking Thread 1\n", sizeof("Forking Thread 1\n"));
    Fork(my_thread);
    Print_F("Forking Thread 2\n", sizeof("Forking Thread 2\n"));
    Fork(my_thread);
    Print_F("Forking Thread 3\n", sizeof("Forking Thread 3\n"));
    Fork(my_thread);
    Print_F("Forking Thread 4\n", sizeof("Forking Thread 4\n"));
    Fork(my_thread);
    Print_F("Forking Thread 5\n", sizeof("Forking Thread 5\n"));
    Fork(my_thread);
    Print_F("Forking Thread 6\n", sizeof("Forking Thread 6\n"));
    Fork(my_thread);
    Print_F("Forking Thread 7\n", sizeof("Forking Thread 7\n"));
    Fork(my_thread);
    Print_F("Forking Thread 8\n", sizeof("Forking Thread 8\n"));
    Fork(my_thread);
    Print_F("Forking Thread 9\n", sizeof("Forking Thread 9\n"));
    Fork(my_thread);

    return 0;
}
