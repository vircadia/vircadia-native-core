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
#include "Util.h"


//  Structure to hold references to other agents that are nearby 

const int MAX_AGENTS = 100; 
struct AgentList {
    char address[255];
    unsigned short port;
    timeval pingStarted;
    int pingMsecs;
    char agentType;
    bool isSelf;
    Head head;
} agents[MAX_AGENTS];

int num_agents = 0; 


char * getAgentAddress(int agentNumber) {
    if (agentNumber < getAgentCount()) return agents[agentNumber].address;
    else return NULL;
}

int getAgentCount() {
    return num_agents;
}

int getAgentPing(int agentNumber) {
    if (agentNumber < getAgentCount()) return agents[agentNumber].pingMsecs;
    else return -1;
}

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
    char agentType;
    unsigned short nPort = 0;
    unsigned int iPort = 0;
    spot = packet.find_first_of (",", 0);
    while (spot != std::string::npos) {
        std::string thisAgent = packet.substr(start_spot, spot-start_spot);
        //std::cout << "raw string: " << thisAgent << "\n";
        sscanf(thisAgent.c_str(), "%c %s %u", &agentType, address.c_str(), &iPort);
        nPort = (unsigned short) iPort; 
        add_agent((char *)address.c_str(), nPort, agentType);
        numAgents++;
        start_spot = spot + 1;
        if (start_spot < packet.length())
            spot = packet.find_first_of (",", start_spot);
        else spot = std::string::npos;
    }
    return numAgents;
}

//  Render the heads of the agents
void render_agents(int renderSelf, float * myLocation) {
    for (int i = 0; i < num_agents; i++) {
        glm::vec3 pos = agents[i].head.getPos();
        glPushMatrix();
        if (!agents[i].isSelf || renderSelf) {
            glTranslatef(-pos.x, -pos.y, -pos.z);
            agents[i].head.render(0, myLocation);
        }
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
            if ((strcmp(address, "127.0.0.1") == 0) && (port == AGENT_UDP_PORT)) {
                agents[i].isSelf = true;
            } else agents[i].isSelf = false;
        }
    }
}

//
//  Look for an agent by it's IP number, add if it does not exist in local list 
//
int add_agent(char * address, unsigned short port, char agentType) {
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
        agents[num_agents].agentType = agentType;
        std::cout << "Added Agent # " << num_agents << " with Address " <<
            agents[num_agents].address << ":" << agents[num_agents].port << " T: " <<
            agentType << "\n";
        
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
int broadcastToAgents(UDPSocket *handle, char * data, int length, int sendToSelf) {
    int sent_bytes;
    //printf("broadcasting to %d agents\n", num_agents);
    for (int i = 0; i < num_agents; i++) {       
        if  (sendToSelf || (((strcmp((char *)"127.0.0.1", agents[i].address) != 0)
            && (agents[i].port != AGENT_UDP_PORT))))
            
            if (agents[i].agentType != 'M') {
                //std::cout << "broadcasting my agent data to:  " << agents[i].address << " : " << agents[i].port << "\n";

                sent_bytes = handle->send(agents[i].address, agents[i].port, data, length);
                if (sent_bytes != length) {
                    std::cout << "Broadcast to agents FAILED\n";
                    return 0;
                }
            }
    }
    return 1;
}

//  Ping other agents to see how fast we are running
void pingAgents(UDPSocket *handle) {
    char payload[] = "P";
    for (int i = 0; i < num_agents; i++) {
        if (agents[i].agentType != 'M') {
            gettimeofday(&agents[i].pingStarted, NULL);
            handle->send(agents[i].address, agents[i].port, payload, 1);
            //printf("\nSent Ping at %d usecs\n", agents[i].pingStarted.tv_usec);
        }
        
    }
}

//  On receiving a ping reply, save that with the agent record
void setAgentPing(char * address, unsigned short port) {
    for (int i = 0; i < num_agents; i++) {
        if ((strcmp(address, agents[i].address) == 0) && (agents[i].port == port)) {
            timeval pingReceived;
            gettimeofday(&pingReceived, NULL);
            float pingMsecs = diffclock(&agents[i].pingStarted, &pingReceived);
            //printf("Received ping at %d usecs, Agent ping = %3.1f\n", pingReceived.tv_usec, pingMsecs);
            agents[i].pingMsecs = pingMsecs;
        }
    }
}

void kludgyMixerUpdate(Audio audio) {
    for (int i = 0; i < num_agents; i++) {
        if (agents[i].agentType == 'M') {
            audio.updateMixerParams(agents[i].address, agents[i].port);
        }
    }
}

