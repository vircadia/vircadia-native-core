//
//  QTestExtensions.h
//  tests/jitter/src
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_JitterTests_h
#define hifi_JitterTests_h

#include <QtTest/QtTest>

class JitterTests : public QObject {
    Q_OBJECT
    
    private slots:
    void qTestsNotYetImplemented () {
        qDebug() << "TODO: Reimplement this using QtTest!\n"
        "(JitterTests takes commandline arguments (port numbers), and can be run manually by #define-ing RUN_MANUALLY in JitterTests.cpp)";
    }
};

#endif
