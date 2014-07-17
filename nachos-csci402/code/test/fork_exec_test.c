/**
 * Project:     NACHOS Kolsh OS
 * Date:        Tuesday, June 24, 2014
 * Description: This program tests the execution stream system calls.
 *                  Fork    called to fork a new process thread
 *                  Exit    called when a process thread finishes executing
 *                  Yield   called to yield the current thread running in the CPU
 *                  Exec    called to execute a new process
 *                  
 * We will be testing input output behavior in accordance to spec outlined in userprog/syscall.h
 *
 * Corner Cases
 *    1   we should not be able to allocate more than MAX_PROCESS_THREADS
 *    2   stack overflow should not be allowed (we haven't explicitly coded for
 *        this, but NACHOS should already have this in place).
 *      
 */

#include "syscall.h"
#define TRUE                1
#define FALSE               0
#define NULL                0
#define MAX_PROCESS_THREADS 100

void ASSERT(int assert_value, char *assert_message, int message_size, int expected, int rcvd) {
    if(!assert_value) {
        PrintF("Assertion Failed: ", sizeof("Assertion Failed: "));
        PrintF(assert_message, message_size);
        PrintF(" expected [", sizeof(" expected ["));
        PrintL(expected);
        PrintF("]", 1);
        PrintF(" rcvd [", sizeof(" rcvd ["));
        PrintL(rcvd);
        PrintF("]\n", 2);
        Halt();
        return;
    }
    return;
}

void hello_world() {
    PrintF("Hello World from a thread\n", sizeof("Hello World from a thread\n"));
    Exit(0);
    return;
}

void thread_yield() {
    PrintF("Hello World from a thread\n", sizeof("Hello World from a thread\n"));
    while(TRUE) {
        Yield();
    }
    Halt();
    return;
}

int main() {
    
    char *thread_name = "fork_test_thread_1";
    char *thread_name_2 = "fnt_3_yield- ";
    unsigned int i = 0;

    /**
     * FORK - POSITIVE TESTS
     *  fpt_1:  valid inputs                            expect: 0
     *  fpt_2:  valid function pointer, null otherwise  expect: 0
     */
    int fpt_1_result, fpt_2_result;

    /**
     * FORK - NEGATIVE TESTS
     *  fnt_1:  invalid function pointer (pointer to data)      expect: -1
     *  fnt_2:  invalid string pointer (pointer to function)    expect: -1
     *  fnt_3:  exceed max process thread                       expect: -2
     */
    int fnt_1_result, fnt_2_result, fnt_3_result;

    fpt_1_result = Fork(hello_world, 
                            "fork_test_thread_1",
                            sizeof("fork_test_thread_1"));
    ASSERT((fpt_1_result == 0), "fpt_1", sizeof("fpt_1"), 0, fpt_1_result);
    
    fpt_2_result = Fork(hello_world,
                            NULL,
                            NULL);
    ASSERT((fpt_2_result == 0), "fpt_2", sizeof("fpt_2"), 0, fpt_2_result);
    
    fnt_1_result = Fork((void *)thread_name, 
                            "fork_ntest_thread_1", 
                            sizeof("fork_ntest_thread_1"));
    ASSERT((fnt_1_result == -1), "fnt_1", sizeof("fnt_1"), -1, fnt_1_result);

    fnt_2_result = Fork(hello_world, 
                            (char *)hello_world,
                            sizeof("fork_ntest_thread_2"));
    ASSERT((fnt_2_result == -1), "fnt_2", sizeof("fnt_2"), -1, fnt_2_result);
    

    for(i = 0; i < 100; i++) {
        thread_name_2[12] = (char)(((int)'0')+i);
        PrintF(thread_name_2, 13);
        PrintF(" \n", 2);
        Fork(thread_yield, thread_name_2, 13);
    }
    fnt_3_result = Fork(thread_yield,
                            "fork_ntest_thread_3", 
                            sizeof("fork_ntest_thread_3"));
    ASSERT((fnt_3_result == -2), "fnt_3", sizeof("fnt_3"), -2, fnt_3_result);
    

    PrintF("ALL TESTS COMPLETED SUCCESSFULLY\n", sizeof("ALL TESTS COMPLETED SUCCESSFULLY\n"));

    Halt();
    
    return 0;
}
