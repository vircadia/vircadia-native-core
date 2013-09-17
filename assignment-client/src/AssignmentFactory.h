//
//  AssignmentFactory.h
//  hifi
//
//  Created by Stephen Birarda on 9/17/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AssignmentFactory__
#define __hifi__AssignmentFactory__

#include "Assignment.h"

class AssignmentFactory {
public:
    static Assignment* unpackAssignment(const unsigned char* dataBuffer, int numBytes);
};

#endif /* defined(__hifi__AssignmentFactory__) */
