//
//  AssignmentFactory.h
//  assignment-client/src
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__AssignmentFactory__
#define __hifi__AssignmentFactory__

#include <ThreadedAssignment.h>

class AssignmentFactory {
public:
    static ThreadedAssignment* unpackAssignment(const QByteArray& packet);
};

#endif /* defined(__hifi__AssignmentFactory__) */
