//
//  AssignmentThread.h
//  assignment-client/src
//
//  Created by Stephen Birarda on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__AssignmentThread__
#define __hifi__AssignmentThread__

#include <QtCore/QThread>

#include <ThreadedAssignment.h>

class AssignmentThread : public QThread {
public:
    AssignmentThread(const SharedAssignmentPointer& assignment, QObject* parent);
private:
    SharedAssignmentPointer _assignment;
};

#endif /* defined(__hifi__AssignmentThread__) */
