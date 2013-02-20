//
//  AgentSocket.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/19/13.
//
//

#include "AgentSocket.h"

#define MAX_ADDRESS_SIZE 255

AgentSocket::AgentSocket() {
    address = new char[MAX_ADDRESS_SIZE];
}

AgentSocket::AgentSocket(const AgentSocket &otherAgentSocket) {
    address = new char[MAX_ADDRESS_SIZE];
    strcpy(address, otherAgentSocket.address);
    port = otherAgentSocket.port;
}

AgentSocket& AgentSocket::operator=(const AgentSocket &otherAgentSocket) {
    
    if (this != &otherAgentSocket) {
        delete address;
        address = new char[MAX_ADDRESS_SIZE];
        strcpy(address, otherAgentSocket.address);
        port = otherAgentSocket.port;
    }
    
    return *this;
}

AgentSocket::~AgentSocket() {
    delete address;
}