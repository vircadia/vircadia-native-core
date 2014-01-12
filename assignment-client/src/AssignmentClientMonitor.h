//
//  AssignmentClientMonitor.h
//  hifi
//
//  Created by Stephen Birarda on 1/10/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AssignmentClientMonitor__
#define __hifi__AssignmentClientMonitor__

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

#include <Assignment.h>

extern const char* NUM_FORKS_PARAMETER;

class AssignmentClientMonitor : public QCoreApplication {
    Q_OBJECT
public:
    AssignmentClientMonitor(int &argc, char **argv, int numAssignmentClientForks);
private slots:
    void childProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
private:
    void spawnChildClient();
    
    QStringList _childArguments;
};

#endif /* defined(__hifi__AssignmentClientMonitor__) */
