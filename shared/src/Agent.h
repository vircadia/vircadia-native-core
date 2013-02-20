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
#include "AgentSocket.h"
#include "AgentData.h"

class Agent {
    public:
        Agent();
        Agent(AgentSocket *agentPublicSocket, AgentSocket *agentLocalSocket, char agentType);
        Agent(const Agent &otherAgent);
        Agent& operator=(Agent otherAgent);
        ~Agent();
        
        bool matches(AgentSocket *otherPublicSocket, AgentSocket *otherLocalSocket, char otherAgentType);
        AgentSocket *publicSocket, *localSocket, *activeSocket;
        char type;
        timeval pingStarted;
        int pingMsecs;
        bool isSelf;
        AgentData *linkedData;
    private:
        void swap(Agent &first, Agent &second);
};

#endif /* defined(__hifi__Agent__) */
