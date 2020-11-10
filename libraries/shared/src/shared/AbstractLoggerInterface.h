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
    inline bool showSourceDebugging() { return _showSourceDebugging; }
    inline bool extraDebugging() { return _extraDebugging; }
    inline bool debugPrint() { return _debugPrint; }
    inline bool infoPrint() { return _infoPrint; }
    inline bool criticalPrint() { return _criticalPrint; }
    inline bool warningPrint() { return _warningPrint; }
    inline bool suppressPrint() { return _suppressPrint; }
    inline bool fatalPrint() { return _fatalPrint; }
    inline bool unknownPrint() { return _unknownPrint; }
    inline void setShowSourceDebugging(bool showSourceDebugging) { _showSourceDebugging = showSourceDebugging; }
    inline void setExtraDebugging(bool extraDebugging) { _extraDebugging = extraDebugging; }
    inline void setDebugPrint(bool debugPrint) { _debugPrint = debugPrint; }
    inline void setInfoPrint(bool infoPrint) { _infoPrint = infoPrint; }
    inline void setCriticalPrint(bool criticalPrint) { _criticalPrint = criticalPrint; }
    inline void setWarningPrint(bool warningPrint) { _warningPrint = warningPrint; }
    inline void setSuppressPrint(bool suppressPrint) { _suppressPrint = suppressPrint; }
    inline void setFatalPrint(bool fatalPrint) { _fatalPrint = fatalPrint; }
    inline void setUnknownPrint(bool unknownPrint) { _unknownPrint = unknownPrint; }

    virtual void addMessage(const QString&) = 0;
    virtual QString getLogData() = 0;
    virtual void locateLog() = 0;
    virtual void sync() {}

signals:
    void logReceived(QString message);

private:
    bool _showSourceDebugging{ false };
    bool _extraDebugging{ false };
    bool _debugPrint{ true };
    bool _infoPrint{ true };
    bool _criticalPrint{ true };
    bool _warningPrint{ true };
    bool _suppressPrint{ true };
    bool _fatalPrint{ true };
    bool _unknownPrint{ true };
};

#endif // hifi_AbstractLoggerInterface_h
