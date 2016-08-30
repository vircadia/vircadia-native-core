//
//  main.cpp
//  tools/vhacd/src
//
//  Created by Virendra Singh on 2/20/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

// #include <stdio.h>
#include <iostream>
#include <iomanip>
#include <VHACD.h>
#include <string>
#include <vector>

#include "VHACDUtilApp.h"

using namespace std;
using namespace VHACD;


int main(int argc, char * argv[]) {
    VHACDUtilApp app(argc, argv);
    return app.getReturnCode();
}
