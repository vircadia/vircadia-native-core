//
//  AbstractLoggerInterface.h
//  interface/src
//
//  Created by Stojce Slavkovski on 12/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AbstractLoggerInterface_h
#define hifi_AbstractLoggerInterface_h

#include <QtCore/QObject>
#include <QString>
#include <QStringList>

class AbstractLoggerInterface : public QObject {
    Q_OBJECT

public:
    static AbstractLoggerInterface* get();
    AbstractLoggerInterface(QObject* parent = NULL);
    ~AbstractLoggerInterface();
    inline bool extraDebugging() { return _extraDebugging; }
    inline bool debugPrint() { return _debugPrint; }
    inline bool infoPrint() { return _infoPrint; }
    inline bool criticalPrint() { return _criticalPrint; }
    inline void setExtraDebugging(bool debugging) { _extraDebugging = debugging; }
    inline void setDebugPrint(bool debugging) { _debugPrint = debugging; }
    inline void setInfoPrint(bool debugging) { _infoPrint = debugging; }
    inline void setCriticalPrint(bool debugging) { _criticalPrint = debugging; }

    virtual void addMessage(const QString&) = 0;
    virtual QString getLogData() = 0;
    virtual void locateLog() = 0;
    virtual void sync() {}

signals:
    void logReceived(QString message);

private:
    bool _extraDebugging{ false };
    bool _debugPrint{ false };
    bool _infoPrint{ false };
    bool _criticalPrint{ true };
};

#endif // hifi_AbstractLoggerInterface_h
