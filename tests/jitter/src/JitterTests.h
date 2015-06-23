//
//  JitterTests.h
//  hifi
//
//  Created by Seiji Emery on 6/23/15.
//
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
