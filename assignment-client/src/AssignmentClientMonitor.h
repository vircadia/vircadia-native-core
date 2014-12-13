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

class AssignmentClientMonitor : public QCoreApplication {
    Q_OBJECT
public:
    AssignmentClientMonitor(int &argc, char **argv, int numAssignmentClientForks);
    ~AssignmentClientMonitor();
    
    void stopChildProcesses();
private slots:
    void childProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
private:
    void spawnChildClient();
    QList<QPointer<QProcess> > _childProcesses;
    
    QStringList _childArguments;
};

#endif // hifi_AssignmentClientMonitor_h
