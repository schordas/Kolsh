/**
 * fork.c
 */

#include "syscall.h"

#define NULL 0

void my_thread() {
    PrintF("hello from a Thread\n", sizeof("hello from a Thread\n"));
    Exit(0);
}

int main() {

    PrintF("hello from a user-program\n", sizeof("hello from a user-program\n"));

    PrintF("Forking Thread 0\n", sizeof("Forking Thread 0\n"));
    Fork(my_thread, NULL, NULL);
    PrintF("Forking Thread 1\n", sizeof("Forking Thread 1\n"));
    Fork(my_thread, NULL, NULL);
    PrintF("Forking Thread 2\n", sizeof("Forking Thread 2\n"));
    Fork(my_thread, "thread 2", sizeof("thread 2"));
    PrintF("Forking Thread 3\n", sizeof("Forking Thread 3\n"));
    Fork(my_thread, NULL, NULL);
    PrintF("Forking Thread 4\n", sizeof("Forking Thread 4\n"));
    Fork(my_thread, NULL, NULL);
    PrintF("Forking Thread 5\n", sizeof("Forking Thread 5\n"));
    Fork(my_thread, NULL, NULL);
    PrintF("Forking Thread 6\n", sizeof("Forking Thread 6\n"));
    Fork(my_thread, NULL, NULL);
    PrintF("Forking Thread 7\n", sizeof("Forking Thread 7\n"));
    Fork(my_thread, NULL, NULL);
    PrintF("Forking Thread 8\n", sizeof("Forking Thread 8\n"));
    Fork(my_thread, NULL, NULL);
    PrintF("Forking Thread 9\n", sizeof("Forking Thread 9\n"));
    Fork(my_thread, NULL, NULL);

    return 0;
}
