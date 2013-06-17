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