//
//  AssignmentClientMonitor.cpp
//  assignment-client/src
//
//  Created by Stephen Birarda on 1/10/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <signal.h>

#include <LogHandler.h>
#include <ShutdownEventListener.h>

#include "AssignmentClientMonitor.h"

const char* NUM_FORKS_PARAMETER = "-n";

const QString ASSIGNMENT_CLIENT_MONITOR_TARGET_NAME = "assignment-client-monitor";

AssignmentClientMonitor::AssignmentClientMonitor(int &argc, char **argv, int numAssignmentClientForks) :
    QCoreApplication(argc, argv)
{    
    // start the Logging class with the parent's target name
    LogHandler::getInstance().setTargetName(ASSIGNMENT_CLIENT_MONITOR_TARGET_NAME);
    
    // setup a shutdown event listener to handle SIGTERM or WM_CLOSE for us
#ifdef _WIN32
    installNativeEventFilter(&ShutdownEventListener::getInstance());
#else
    ShutdownEventListener::getInstance();
#endif
    
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

AssignmentClientMonitor::~AssignmentClientMonitor() {
    stopChildProcesses();
}

void AssignmentClientMonitor::stopChildProcesses() {
    
    QList<QPointer<QProcess> >::Iterator it = _childProcesses.begin();
    while (it != _childProcesses.end()) {
        if (!it->isNull()) {
            qDebug() << "Monitor is terminating child process" << it->data();
            
            // don't re-spawn this child when it goes down
            disconnect(it->data(), 0, this, 0);
            
            it->data()->terminate();
            it->data()->waitForFinished();
        }
        
        it = _childProcesses.erase(it);
    }
}

void AssignmentClientMonitor::spawnChildClient() {
    QProcess *assignmentClient = new QProcess(this);
    
    _childProcesses.append(QPointer<QProcess>(assignmentClient));
    
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
    
    // remove the old process from our list of child processes
    qDebug() << "need to remove" << QPointer<QProcess>(qobject_cast<QProcess*>(sender()));
    _childProcesses.removeOne(QPointer<QProcess>(qobject_cast<QProcess*>(sender())));
    
    spawnChildClient();
}
