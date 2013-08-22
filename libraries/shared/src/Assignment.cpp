//
//  Assignment.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include "Assignment.h"

Assignment::Assignment(Assignment::Type type) : _type(type) {
    
}

Assignment::Assignment(Assignment::Type type, sockaddr_in& senderSocket) :
    _type(type),
    _senderSocket(senderSocket) {
    
}