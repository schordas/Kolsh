/**
 * Project:     NACHOS Kolsh OS
 * Date:        Tuesday, June 24, 2014
 * Description: This program tests the execution stream system calls.
 *                  Fork    called to fork a new process thread
 *                  Exit    called when a process thread finishes executing
 *                  Yield   called to yield the current thread running in the CPU
 *                  Exec    called to execute a new process
 *                  
 *
 *
 * We will be testing input output behavior in accordance to the following:
 *
 * FORK
 * Function Signature:
 *     int Fork(void *func_ptr, char *name, int name_size);
 * 
 * Parameters:
 * void *func_ptr  A pointer to the function that will be executed in
 *                 a new thread. This address must be non-zero, 
 *                 non-negative, and lie in the address space of
 *                 the caller.
 * 
 * char *name      Will be used to name the forked thread. If the string
 *                 literal "DEFAULT" is passed in, the name will default
 *                 to [process name]-thread[thread_id]. The supplied 
 *                 char pointer must be non-zero, non-negative, and lie 
 *                 in the address space of the caller.
 * 
 * int name_size   The size of the name string not including the null
 *                 terminator. This value must be non-zero, and non-negative.
 * 
 * Return Value
 *     int         If non-negative, it is the thread id of the newly forked thread.
 *                 Otherwise the error codes are as follows
 *                 -1  invalid user input
 *                 -2  max process threads allocated
 *
 * Corner Cases
 *    1   we should not be able to allocate more than MAX_PROCESS_THREADS
 *    2   stack overflow should not be allowed (we haven't explicitly coded for
 *        this, but NACHOS should already have this in place).
 *      
 */


#include "syscall.h"
#define TRUE    1
#define FALSE   0

void ASSERT(int assert_value, char *assert_message, int message_size) {
    if(!assert_value) {
        Print_F("Assertion Failed: ", sizeof("Assertion Failed: "));
        Print_F(assert_message, message_size);
        Halt();
    }
}

void thread_yield() {
    Print_F("NEW THREAD RUNNING\n", sizeof("NEW THREAD RUNNING\n"));
    while(TRUE) {
        Yield();
    }
}

int main() {
    
    /**
     * FORK - NEGATIVE TESTS
     * besides the parameter under test, all other values will be valid
     *  ft_1:  invalid function pointer (negative address)
     *  ft_2:  invalid function pointer (pointer to data)
     *  ft_3:  invalid string pointer (negative address)
     *  ft_4:  invalid string pointer (pointer to function)
     *  ft_5:  invalid string length (negative length)
     */
    int ft_1_result, ft_2_result, ft_3_result, ft_4_result, ft_5_result;

    int negative_pointer = 0xffffffff;
    

    
    
    ASSERT((5 == 4), "5 == 4\n", sizeof("5 == 4\n"));

    Halt();
    
    return 0;
}

