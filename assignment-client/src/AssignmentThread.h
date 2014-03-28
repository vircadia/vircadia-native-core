//
//  AssignmentThread.h
//  hifi
//
//  Created by Stephen Birarda on 2014-03-28.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
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
