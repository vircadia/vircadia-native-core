//
//  Logstash.cpp
//  hifi
//
//  Created by Stephen Birarda on 6/11/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <cstring>
#include <cstdio>
#include <netdb.h>

#include "SharedUtil.h"
#include "AgentList.h"

#include "Logstash.h"

sockaddr_in Logstash::logstashSocket = {};

sockaddr* Logstash::socket() {
    
    if (logstashSocket.sin_addr.s_addr == 0) {
        // we need to construct the socket object
        
        // assume IPv4
        logstashSocket.sin_family = AF_INET;
        
        // use the constant port
        logstashSocket.sin_port = htons(LOGSTASH_UDP_PORT);
        
        // lookup the IP address for the constant hostname
        struct hostent* logstashHostInfo;
        if ((logstashHostInfo = gethostbyname(LOGSTASH_HOSTNAME))) {
            memcpy(&logstashSocket.sin_addr, logstashHostInfo->h_addr_list[0], logstashHostInfo->h_length);
        } else {
            printf("Failed to lookup logstash IP - will try again on next log attempt.\n");
        }
    }
    
    return (sockaddr*) &logstashSocket;
}

bool Logstash::shouldSendStats() {
    static bool shouldSendStats = isInEnvironment("production");
    return shouldSendStats;
}

void Logstash::stashValue(char statType, const char* key, float value) {
    static char logstashPacket[MAX_PACKET_SIZE];
    
    // load up the logstash packet with the key and the passed float value
    // send it to 4 decimal places
    int numPacketBytes = sprintf(logstashPacket, "%c %s %.4f", statType, key, value);
    
    AgentList *agentList = AgentList::getInstance();
    
    if (agentList) {
        agentList->getAgentSocket()->send(socket(), logstashPacket, numPacketBytes);
    }
}