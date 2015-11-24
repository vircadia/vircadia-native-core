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

#ifndef hifi_AssignmentFactory_h
#define hifi_AssignmentFactory_h

#include <ThreadedAssignment.h>

class AssignmentFactory {
public:
    static ThreadedAssignment* unpackAssignment(ReceivedMessage& message);
};

#endif // hifi_AssignmentFactory_h
