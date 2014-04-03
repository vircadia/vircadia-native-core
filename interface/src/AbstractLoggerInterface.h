//
//  AbstractLoggerInterface.h
//  interface
//
//  Created by Stojce Slavkovski on 12/22/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AbstractLoggerInterface__
#define __interface__AbstractLoggerInterface__

#include <QtCore/QObject>
#include <QString>
#include <QStringList>

class AbstractLoggerInterface : public QObject {
    Q_OBJECT

public:
    AbstractLoggerInterface(QObject* parent = NULL) : QObject(parent) {};
    inline bool extraDebugging() { return _extraDebugging; };
    inline void setExtraDebugging(bool debugging) { _extraDebugging = debugging; };

    virtual void addMessage(QString) = 0;
    virtual QStringList getLogData() = 0;
    virtual void locateLog() = 0;

signals:
    void logReceived(QString message);

private:
    bool _extraDebugging;
};

#endif /* defined(__interface__AbstractLoggerInterface__) */
