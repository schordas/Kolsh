/**
 * fork.c
 */

#include "syscall.h"

void my_thread() {
    int a = 5;
    return;
}

int main() {

    Print_F("hello from a user-program\n", sizeof("hello from a user-program\n"));

    Fork(my_thread);
    Halt();
}
