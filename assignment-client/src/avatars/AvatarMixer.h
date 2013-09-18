//
//  AvatarMixer.h
//  hifi
//
//  Created by Stephen Birarda on 9/5/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarMixer__
#define __hifi__AvatarMixer__

#include <Assignment.h>

/// Handles assignments of type AvatarMixer - distribution of avatar data to various clients
class AvatarMixer : public Assignment {
public:
    AvatarMixer(const unsigned char* dataBuffer, int numBytes);
    
    /// runs the avatar mixer
    void run();
};

#endif /* defined(__hifi__AvatarMixer__) */
