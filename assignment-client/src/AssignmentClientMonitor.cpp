//
//  AssignmentClientMonitor.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/10/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <Logging.h>

#include "AssignmentClientMonitor.h"

const char* NUM_FORKS_PARAMETER = "-n";

const QString ASSIGNMENT_CLIENT_MONITOR_TARGET_NAME = "assignment-client-monitor";

AssignmentClientMonitor::AssignmentClientMonitor(int &argc, char **argv, int numAssignmentClientForks) :
    QCoreApplication(argc, argv)
{
    // start the Logging class with the parent's target name
    Logging::setTargetName(ASSIGNMENT_CLIENT_MONITOR_TARGET_NAME);
    
    _childArguments = arguments();
    
    // remove the parameter for the number of forks so it isn't passed to the child forked processes
    int forksParameterIndex = _childArguments.indexOf(NUM_FORKS_PARAMETER);
    
    // this removes both the "-n" parameter and the number of forks passed
    _childArguments.removeAt(forksParameterIndex);
    _childArguments.removeAt(forksParameterIndex);
    
    // use QProcess to fork off a process for each of the child assignment clients
    for (int i = 0; i < numAssignmentClientForks; i++) {
        spawnChildClient();
    }
}

void AssignmentClientMonitor::spawnChildClient() {
    QProcess *assignmentClient = new QProcess(this);
    
    // make sure that the output from the child process appears in our output
    assignmentClient->setProcessChannelMode(QProcess::ForwardedChannels);
    
    assignmentClient->start(applicationFilePath(), _childArguments);
    
    // link the child processes' finished slot to our childProcessFinished slot
    connect(assignmentClient, SIGNAL(finished(int, QProcess::ExitStatus)), this,
            SLOT(childProcessFinished(int, QProcess::ExitStatus)));
    
    qDebug() << "Spawned a child client with PID" << assignmentClient->pid();
}

void AssignmentClientMonitor::childProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qDebug("Replacing dead child assignment client with a new one");
    spawnChildClient();
}