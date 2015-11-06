//
//  main.cpp
//  tools/cache-extract/src
//
//  Created by Anthony Thibault on 11/6/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

#include <QtCore/QCoreApplication>
#include "CacheExtractApp.h"

int main (int argc, char** argv) {
    CacheExtractApp app(argc, argv);
    return app.exec();
}
