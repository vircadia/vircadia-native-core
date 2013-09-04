//
//  Assignment.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "Assignment.h"

Assignment::Assignment(Assignment::Direction direction, Assignment::Type type, const char* pool) :
    _direction(direction),
    _type(type),
    _pool(pool)
{
    
}