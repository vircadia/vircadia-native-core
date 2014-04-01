//
//  AssignmentThread.cpp
//  hifi
//
//  Created by Stephen Birarda on 2014-03-28.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "AssignmentThread.h"

AssignmentThread::AssignmentThread(const SharedAssignmentPointer& assignment, QObject* parent) :
    QThread(parent),
    _assignment(assignment)
{
    
}