//
//  Oven.h
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Oven_h
#define hifi_Oven_h

#include <QtWidgets/QApplication>

#include <TBBHelpers.h>

#include <atomic>

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Oven*>(QCoreApplication::instance()))

class OvenMainWindow;

class Oven : public QApplication {
    Q_OBJECT

public:
    Oven(int argc, char* argv[]);
    ~Oven();

    OvenMainWindow* getMainWindow() const { return _mainWindow; }

    QThread* getFBXBakerThread();
    QThread* getNextWorkerThread();

private:
    void setupWorkerThreads(int numWorkerThreads);
    void setupFBXBakerThread();

    OvenMainWindow* _mainWindow;
    QThread* _fbxBakerThread;
    QList<QThread*> _workerThreads;

    std::atomic<uint> _nextWorkerThreadIndex;
    int _numWorkerThreads;
};


#endif // hifi_Oven_h
