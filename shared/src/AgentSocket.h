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
        AgentSocket& operator=(AgentSocket otherAgentSocket);
        ~AgentSocket();
        
        
    
        char *address;
        unsigned short port;
    private:
        void swap(AgentSocket &first, AgentSocket &second);
};

inline std::ostream& operator<<(std::ostream & Str, AgentSocket const &socket) {
    Str << socket.address << ":" << socket.port;
    return Str;
}

#endif /* defined(__hifi__AgentSocket__) */
