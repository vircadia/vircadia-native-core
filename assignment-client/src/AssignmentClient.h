//
//  AssignmentClient.h
//  assignment-client/src
//
//  Created by Stephen Birarda on 11/25/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__AssignmentClient__
#define __hifi__AssignmentClient__

#include <QtCore/QCoreApplication>

#include "ThreadedAssignment.h"

class AssignmentClient : public QCoreApplication {
    Q_OBJECT
public:
    AssignmentClient(int &argc, char **argv);
private slots:
    void sendAssignmentRequest();
    void readPendingDatagrams();
    void assignmentCompleted();
    void handleAuthenticationRequest();
private:
    Assignment _requestAssignment;
    SharedAssignmentPointer _currentAssignment;
};

#endif /* defined(__hifi__AssignmentClient__) */
