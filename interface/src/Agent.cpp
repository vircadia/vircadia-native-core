//
//  Agent.cpp
//  interface
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
#include "Agent.h"
#include "Head.h" 
#include "util.h"


//  Structure to hold references to other agents that are nearby 

const int MAX_AGENTS = 100; 
struct AgentList {
    char address[255];
    unsigned short port;
    timeval pingStarted;
    int pingMsecs;
    Head head;
} agents[MAX_AGENTS];

int num_agents = 0; 

//
// Process an incoming domainserver packet telling you about other nearby agents,
// and returns the number of agents that were reported.
//
int update_agents(char * data, int length) {
    int numAgents = 0;
    std::string packet(data, length);
    size_t spot;
    size_t start_spot = 0;
    std::string address, port;
    unsigned short nPort = 0;
    spot = packet.find_first_of (",", 0);
    while (spot != std::string::npos) {
        std::string thisAgent = packet.substr(start_spot, spot-start_spot);
        if (thisAgent.find_first_of(":", 0) != std::string::npos) {
            address = thisAgent.substr(0, thisAgent.find_first_of(":", 0));
            port = thisAgent.substr(thisAgent.find_first_of(":", 0) + 1,
                                    thisAgent.length() - thisAgent.find_first_of(":", 0));
            nPort = atoi(port.c_str());
        }
        //std::cout << "IP: " << address << ", port: " << nPort << "\n";
        add_agent((char *)address.c_str(), nPort);
        numAgents++;
        start_spot = spot + 1;
        if (start_spot < packet.length())
            spot = packet.find_first_of (",", start_spot);
        else spot = std::string::npos;
    }
    return numAgents;
}

void render_agents() {
    for (int i = 0; i < num_agents; i++) {
        glm::vec3 pos = agents[i].head.getPos();
        glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);
        agents[i].head.render(0);
        glPopMatrix();
    }
}

//
//  Update a single agent with data received from that agent's IP address 
// 
void update_agent(char * address, unsigned short port, char * data, int length)
{
    //printf("%s:%d\n", address, port);
    for (int i = 0; i < num_agents; i++) {
        if ((strcmp(address, agents[i].address) == 0) && (agents[i].port == port)) {
            //  Update the agent 
            agents[i].head.recvBroadcastData(data, length);
        }
    }
}

//
//  Look for an agent by it's IP number, add if it does not exist in local list 
//
int add_agent(char * address, unsigned short port) {
    //std::cout << "Checking for " << IP->c_str() << "  ";
    for (int i = 0; i < num_agents; i++) {
        if ((strcmp(address, agents[i].address) == 0) && (agents[i].port == port)) {
            //std::cout << "Found agent!\n";
            return 0;
        }
    }
    if (num_agents < MAX_AGENTS) {
        strcpy(agents[num_agents].address, address);
        agents[num_agents].port = port;
        std::cout << "Added Agent # " << num_agents << " with Address " <<
         agents[num_agents].address << ":" << agents[num_agents].port << "\n";
        num_agents++;
        return 1;
    } else {
        std::cout << "Max agents reached fail!\n";
        return 0;
    } 
}

//
//  Broadcast data to all the other agents you are aware of, returns 1 if success 
//
int broadcastToAgents(UDPSocket *handle, char * data, int length) {
    int sent_bytes;
    //printf("broadcasting to %d agents\n", num_agents);
    for (int i = 0; i < num_agents; i++) {
        //printf("bcast to %s\n", agents[i].address);
        //
        //  STUPID HACK:  For some reason on OSX with NAT translation packets sent to localhost are
        //  received as from the NAT translated port but have to be sent to the local port number.
        //  
        //if (1)  //(strcmp("192.168.1.53",agents[i].address) == 0)
        //    sent_bytes = handle->send(agents[i].address, 40103, data, length);
        //else
        sent_bytes = handle->send(agents[i].address, agents[i].port, data, length);
        
        if (sent_bytes != length) {
            std::cout << "Broadcast packet fail!\n";
            return 0;
        } 
    }
    return 1;
}

//  Ping other agents to see how fast we are running
void pingAgents(UDPSocket *handle) {
    char payload[] = "P";
    for (int i = 0; i < num_agents; i++) {
        gettimeofday(&agents[i].pingStarted, NULL);
        handle->send(agents[i].address, agents[i].port, payload, 1);
        printf("\nSent Ping at %d usecs\n", agents[i].pingStarted.tv_usec);
    }
}

//  On receiving a ping reply, save that with the agent record
void setAgentPing(char * address, unsigned short port) {
    for (int i = 0; i < num_agents; i++) {
        if ((strcmp(address, agents[i].address) == 0) && (agents[i].port == port)) {
            timeval pingReceived;
            gettimeofday(&pingReceived, NULL);
            float pingMsecs = diffclock(&agents[i].pingStarted, &pingReceived);
            printf("Received ping at %d usecs, Agent ping = %3.1f\n", pingReceived.tv_usec, pingMsecs);
            agents[i].pingMsecs = pingMsecs;
        }
    }
}

