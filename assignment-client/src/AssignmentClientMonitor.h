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

#include <Assignment.h>

extern const char* NUM_FORKS_PARAMETER;

class AssignmentClientChildData {
 public:
    AssignmentClientChildData(QString childType);
    ~AssignmentClientChildData();

    QString getChildType() { return _childType; }

 private:
    QString _childType;
    // ... timestamp
};


class AssignmentClientMonitor : public QCoreApplication {
    Q_OBJECT
public:
    AssignmentClientMonitor(int &argc, char **argv, const unsigned int numAssignmentClientForks);
    ~AssignmentClientMonitor();
    
    void stopChildProcesses();
private slots:
    void childProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void readPendingDatagrams();
    void checkSpares();

private:
    void spawnChildClient();
    QList<QPointer<QProcess> > _childProcesses;
    
    QStringList _childArguments;
    QHash<QString, AssignmentClientChildData*> _childStatus;

    QTimer _checkSparesTimer; // every few seconds see if it need fewer or more spare children
};

#endif // hifi_AssignmentClientMonitor_h
