//
//  main.cpp
//  tests/audio/src
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioRingBufferTests.h"
#include <stdio.h>

int main(int argc, char** argv) {
    AudioRingBufferTests::runAllTests();
    printf("all tests passed.  press enter to exit\n");
    getchar();
    return 0;
}
