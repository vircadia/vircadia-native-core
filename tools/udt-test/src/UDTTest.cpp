//
//  UDTTest.cpp
//  tools/udt-test/src
//
//  Created by Stephen Birarda on 2015-07-30.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UDTTest.h"

#include <QtCore/QDebug>

const QCommandLineOption PORT_OPTION { "p", "listening port for socket (defaults to random)", "port", 0 };

UDTTest::UDTTest(int& argc, char** argv) :
    QCoreApplication(argc, argv)
{
    parseArguments();
    
    _socket.bind(QHostAddress::LocalHost, _argumentParser.value(PORT_OPTION).toUInt());
    qDebug() << "Test socket is listening on" << _socket.localPort();
}

void UDTTest::parseArguments() {
    // use a QCommandLineParser to setup command line arguments and give helpful output
    _argumentParser.setApplicationDescription("High Fidelity Assignment Client");
    _argumentParser.addHelpOption();
    
    const QCommandLineOption helpOption = _argumentParser.addHelpOption();
    
    _argumentParser.addOption(PORT_OPTION);
    
    if (!_argumentParser.parse(QCoreApplication::arguments())) {
        qCritical() << _argumentParser.errorText();
        _argumentParser.showHelp();
        Q_UNREACHABLE();
    }
    
    if (_argumentParser.isSet(helpOption)) {
        _argumentParser.showHelp();
        Q_UNREACHABLE();
    }
}
