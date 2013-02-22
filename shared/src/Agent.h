//
//  Agent.h
//  hifi
//
//  Created by Stephen Birarda on 2/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__Agent__
#define __hifi__Agent__

#include <iostream>
#include "AgentData.h"
#include <sys/socket.h>

class Agent {
    public:
        Agent();
        Agent(sockaddr *agentPublicSocket, sockaddr *agentLocalSocket, char agentType);
        Agent(const Agent &otherAgent);
        Agent& operator=(Agent otherAgent);
        bool operator==(const Agent& otherAgent);
        ~Agent();
    
        bool matches(sockaddr *otherPublicSocket, sockaddr *otherLocalSocket, char otherAgentType);
    
        sockaddr *publicSocket, *localSocket, *activeSocket;
        char type;
        timeval pingStarted;
        int pingMsecs;
        double lastRecvTimeUsecs;
        bool isSelf;
        AgentData *linkedData;
    
        friend std::ostream& operator<<(std::ostream& os, const Agent* agent);
    private:
        void swap(Agent &first, Agent &second);
};

std::ostream& operator<<(std::ostream& os, const Agent* agent);

#endif /* defined(__hifi__Agent__) */
