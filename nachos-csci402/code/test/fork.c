/**
 * fork.c
 */

#include "syscall.h"

void my_thread() {
    Print_F("hello from a Thread\n", sizeof("hello from a Thread\n"));
    Halt();
    return;
}

int main() {

    Print_F("hello from a user-program\n", sizeof("hello from a user-program\n"));

    Fork(my_thread);
    return 0;
}
