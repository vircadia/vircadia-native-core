//
//  AssignmentThread.cpp
//  assignment-client/src
//
//  Created by Stephen Birarda on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssignmentThread.h"

AssignmentThread::AssignmentThread(const SharedAssignmentPointer& assignment, QObject* parent) :
    QThread(parent),
    _assignment(assignment)
{
    
}
