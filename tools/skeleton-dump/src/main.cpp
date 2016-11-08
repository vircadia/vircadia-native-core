//
//  main.cpp
//  tools/skeleton-dump/src
//
//  Created by Anthony Thibault on 11/4/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <QDebug>

#include "SkeletonDumpApp.h"

int main(int argc, char * argv[]) {
    SkeletonDumpApp app(argc, argv);
    return app.getReturnCode();
}
