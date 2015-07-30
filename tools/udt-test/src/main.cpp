//
//  main.cpp
//  tools/udt-test/src
//
//  Created by Stephen Birarda on 7/30/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <QtCore/QCoreApplication>

#include "UDTTest.h"

int main(int argc, char* argv[]) {
    UDTTest app(argc, argv);
    return app.exec();
}

