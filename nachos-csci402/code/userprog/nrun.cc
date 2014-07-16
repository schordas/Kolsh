//-------------------------------------------------------------------------
// This is the program we used to call NACHOS with different -rs arguments
// when we took the class.  It proves very helpful in seeing if there are
// any circumstances which will result in a fatal error (seg. fault, etc).
// 
// This version simply runs whatever filename is passed from the command
// line with rs values of 1-9999.  Feel free to modify it and use it
// in anyway which will help you to test your project.
//
// Designed by Will Page and Ryan Cunningham for CSCI402 (c) 2003
// Modified by Nikhil Handyal
// University of Southern California
// Updated: Wednesday, July 16, 2014
//-------------------------------------------------------------------------
// compile: g++ nrun.cc -o nrun
// usage: nrun <nachos testcase>
// ex: nrun ../test/halt 
//
// The last output stream will be saved in "nrun.log"
//-------------------------------------------------------------------------
// Note: there is no sanity checking performed on the command line values.
//          also, -rs xxxx is hard-coded for < 10000 rs iterations, but it is
//          simple to change the code.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STARTRS     1
#define ENDRS       9999
#define WhenToPrint 100

int main(int argc, char **argv) {
    char nachos_command[50];
    char *grep_command = "grep \"ALL TESTS COMPLETED SUCCESSFULLY\" nrun.log > /dev/null";
    char *program_under_test;
    int error = 0;

    argv++;
    program_under_test = *argv;

    printf("CSCI402 USC NACHOS RS RUNNER\n");
    printf("aka NRUN: best to run and then go get some coffee.\n");

    printf("\nRunning nachos -rs xxxx %s >! nrun.log\n", program_under_test);
    printf("StartRS: [%d]\n", STARTRS);
    printf("EndRS:\t [%d]\n\n", ENDRS);

    for(int i = STARTRS; i < ENDRS && !error; i++) {
        sprintf(nachos_command, "nachos -rs %d -x %s > nrun.log",i, program_under_test);

        if(i % WhenToPrint == 0) {
            printf("Running -rs %d...\n", i);
        }

        system(nachos_command);
        error = system(grep_command);

        if(error) {
            printf("ERROR FOUND WITH RS [%d]\n", i);
            system("cat nrun.log");
        }
    }

    return error;
}
