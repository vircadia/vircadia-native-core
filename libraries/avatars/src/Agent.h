//
//  Agent.h
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Agent__
#define __hifi__Agent__

#import "AudioInjector.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SharedUtil.h"

#include <QtCore/QUrl>

class Agent {
public:    
    bool volatile shouldStop;
    
    void run(QUrl scriptUrl);
private:
};

#endif /* defined(__hifi__Operative__) */
