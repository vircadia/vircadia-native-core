//
//  Agent.h
//  hifi
//
//  Created by Stephen Birarda on 7/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__Agent__
#define __hifi__Agent__

#include <QtCore/QObject>
#include <QtCore/QUrl>

#include <Assignment.h>

class Agent : public Assignment {
    Q_OBJECT
public:
    Agent(const unsigned char* dataBuffer, int numBytes);
    
    bool volatile _shouldStop;
    
    void run();
signals:
    void preSendCallback();
};

#endif /* defined(__hifi__Operative__) */
