/**
 * fork.c
 */

#include "syscall.h"

void my_thread() {
    int a = 5;
    return;
}

int main() {
    Fork(my_thread);
    Halt();
}
