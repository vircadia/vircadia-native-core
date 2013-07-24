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

#include <QtCore/QObject>
#include <QtCore/QUrl>

class Agent : public QObject {
    Q_OBJECT
public:
    Agent();
    
    bool volatile _shouldStop;
    
    void run(QUrl scriptUrl);
signals:
    void preSendCallback();
};

#endif /* defined(__hifi__Operative__) */
