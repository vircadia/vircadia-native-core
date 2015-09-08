//
//  AssignmentClientMonitor.h
//  assignment-client/src
//
//  Created by Stephen Birarda on 1/10/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssignmentClientMonitor_h
#define hifi_AssignmentClientMonitor_h

#include <QtCore/QCoreApplication>
#include <QtCore/qpointer.h>
#include <QtCore/QProcess>
#include <QtCore/QDateTime>

#include <Assignment.h>

#include "AssignmentClientChildData.h"

extern const char* NUM_FORKS_PARAMETER;

class AssignmentClientMonitor : public QObject {
    Q_OBJECT
public:
    AssignmentClientMonitor(const unsigned int numAssignmentClientForks, const unsigned int minAssignmentClientForks,
                            const unsigned int maxAssignmentClientForks, Assignment::Type requestAssignmentType,
                            QString assignmentPool, quint16 listenPort, QUuid walletUUID, QString assignmentServerHostname,
                            quint16 assignmentServerPort);
    ~AssignmentClientMonitor();

    void stopChildProcesses();
private slots:
    void checkSpares();
    void childProcessFinished();
    void handleChildStatusPacket(QSharedPointer<NLPacket> packet);
    
public slots:
    void aboutToQuit();

private:
    void spawnChildClient();
    void simultaneousWaitOnChildren(int waitMsecs);

    QTimer _checkSparesTimer; // every few seconds see if it need fewer or more spare children

    const unsigned int _numAssignmentClientForks;
    const unsigned int _minAssignmentClientForks;
    const unsigned int _maxAssignmentClientForks;

    Assignment::Type _requestAssignmentType;
    QString _assignmentPool;
    QUuid _walletUUID;
    QString _assignmentServerHostname;
    quint16 _assignmentServerPort;

    QMap<qint64, QProcess*> _childProcesses;
};

#endif // hifi_AssignmentClientMonitor_h
