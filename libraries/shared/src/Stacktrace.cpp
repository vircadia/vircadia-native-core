//
//  Stacktrace.cpp
//  hifi
//
//  Created by Stephen Birarda on 5/6/13.
//
//

#include <signal.h>
#include <stdio.h>
#include <execinfo.h>

#include "Stacktrace.h"

const int NUMBER_OF_STACK_ENTRIES = 20;

void printStacktrace(int signal) {
    void* array[NUMBER_OF_STACK_ENTRIES];
    
    // get void*'s for all entries on the stack
    size_t size = backtrace(array, NUMBER_OF_STACK_ENTRIES);
    
    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", signal);
    backtrace_symbols_fd(array, size, 2);
    exit(1);
}