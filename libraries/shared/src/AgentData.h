//
//  AgentData.h
//  hifi
//
//  Created by Stephen Birarda on 2/19/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_AgentData_h
#define hifi_AgentData_h

class Agent;

class AgentData {
public:
    AgentData(Agent* owningAgent);
    virtual ~AgentData() = 0;
    virtual int parseData(unsigned char* sourceBuffer, int numBytes) = 0;
protected:
    Agent* _owningAgent;
};

#endif
