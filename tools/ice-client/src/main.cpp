//
//  main.cpp
//  tools/ice-client/src
//
//  Created by Seth Alves on 2016-9-16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "ICEClientApp.h"

using namespace std;

int main(int argc, char * argv[]) {
    ICEClientApp app(argc, argv);
    return app.exec();
}
