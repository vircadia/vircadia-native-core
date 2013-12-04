//
//  AssignmentClient.h
//  hifi
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AssignmentClient__
#define __hifi__AssignmentClient__

#include <QtCore/QCoreApplication>

#include "ThreadedAssignment.h"

class AssignmentClient : public QCoreApplication {
    Q_OBJECT
public:
    AssignmentClient(int &argc, char **argv,
                     Assignment::Type requestAssignmentType = Assignment::AllTypes,
                     const HifiSockAddr& customAssignmentServerSocket = HifiSockAddr(),
                     const char* requestAssignmentPool = NULL);
private slots:
    void sendAssignmentRequest();
    void readPendingDatagrams();
    void assignmentCompleted();
private:
    Assignment _requestAssignment;
    ThreadedAssignment* _currentAssignment;
};

#endif /* defined(__hifi__AssignmentClient__) */
