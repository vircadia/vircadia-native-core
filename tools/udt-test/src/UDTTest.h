//
//  UDTTest.h
//  tools/udt-test/src
//
//  Created by Stephen Birarda on 2015-07-30.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_UDTTest_h
#define hifi_UDTTest_h

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>

#include <udt/socket.h>

class UDTTest : public QCoreApplication {
public:
    UDTTest(int& argc, char** argv);
private:
    void parseArguments();
    
    QCommandLineParser _argumentParser;
    udt::Socket _socket;
};

#endif // hifi_UDTTest_h
