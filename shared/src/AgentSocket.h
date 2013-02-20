//
//  AgentSocket.h
//  hifi
//
//  Created by Stephen Birarda on 2/19/13.
//
//

#ifndef __hifi__AgentSocket__
#define __hifi__AgentSocket__

#include <iostream>

class AgentSocket {
    public:
        AgentSocket();
        AgentSocket(const AgentSocket &otherAgentSocket);
        AgentSocket& operator=(const AgentSocket &otherAgentSocket);
        ~AgentSocket();
        char *address;
        unsigned short port;
};

#endif /* defined(__hifi__AgentSocket__) */
