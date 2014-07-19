#include "syscall.h"
#define TRUE                1
#define FALSE               0
#define NULL                0


int main() {
    int process_id = -1;

    PrintF("Hello World from exec root\n", sizeof("Hello World from exec root\n"));

    process_id = Exec("../test/fork", sizeof("../test/fork"));

    Exit(0);
}