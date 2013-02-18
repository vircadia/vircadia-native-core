//
//  main.cpp
//  Domain Server 
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//
//  The Domain Server keeps a list of agents that have connected to it, and echoes that list of
//  agents out to agents when they check in.
//
//  The connection is stateless... the domain server will set you inactive if it does not hear from
//  you in LOGOFF_CHECK_INTERVAL milliseconds, meaning your info will not be sent to other users.
//
//  Each packet from an agent has as first character the type of server:
//
//  I - Interactive Agent
//  M - Audio Mixer
//

#include <iostream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include "UDPSocket.h"


const int DOMAIN_LISTEN_PORT = 40102;
const char DESTINATION_IP[] = "127.0.0.1";

const int MAX_PACKET_SIZE = 1500;
char packet_data[MAX_PACKET_SIZE];

const int MAX_AGENTS = 1000;
const int LOGOFF_CHECK_INTERVAL = 5000;

struct AgentList {
    char agentType;
    uint32_t ip;
    in_addr public_sin_addr;
    in_port_t public_port;
    char *private_addr;
    in_port_t private_port;
    float x, y, z;
    bool active;
    timeval time, connectTime;
} agents[MAX_AGENTS];

int num_agents = 0;
int lastActiveCount = 0;

UDPSocket domainSocket = UDPSocket(DOMAIN_LISTEN_PORT);

double diffclock(timeval clock1,timeval clock2)
{
	double diffms = (clock2.tv_sec - clock1.tv_sec) * 1000.0;
    diffms += (clock2.tv_usec - clock1.tv_usec) / 1000.0;   // us to ms
	return diffms;
}

int addAgent(uint32_t ip, in_port_t port, char *private_ip, unsigned short private_port, char agentType, float x, float y, float z) {
    //  Search for agent in list and add if needed 
    int i = 0;
    int is_new = 0; 
    while ((ip != agents[i].ip || port != agents[i].public_port) && (i < num_agents))  {
        i++;
    }
    if ((i == num_agents) || (agents[i].active == false)) is_new = 1;
    agents[i].ip = ip; 
    agents[i].x = x; 
    agents[i].y = y; 
    agents[i].z = z;
    agents[i].active = true;
    agents[i].public_sin_addr.s_addr = ip;
    agents[i].public_port = port;
    strcpy(agents[i].private_addr, private_ip);
    agents[i].private_port = private_port;
    agents[i].agentType = agentType;
    gettimeofday(&agents[i].time, NULL);
    if (is_new) gettimeofday(&agents[i].connectTime, NULL);
    if (i == num_agents) {
        num_agents++;
    }
    return is_new;
}

void update_agent_list(timeval now) {
    int i;
    //std::cout << "Checking agent list" << "\n";
    for (i = 0; i < num_agents; i++) {
        if ((diffclock(agents[i].time, now) > LOGOFF_CHECK_INTERVAL) &&
            agents[i].active) {
            std::cout << "Expired Agent type " << agents[i].agentType << " from " <<
            inet_ntoa(agents[i].public_sin_addr) << ":" << agents[i].public_port << "\n";
            agents[i].active = false; 
        }
    }
}

void send_agent_list(sockaddr_in *agentAddrPointer) {
    int i, length = 0;
    ssize_t sentBytes;
    char buffer[MAX_PACKET_SIZE];
    char * public_address;
    char public_portstring[10];
    char * private_address;
    char private_portstring[10];
    int numSent = 0;
    buffer[length++] = 'D';
    //std::cout << "send list to:  " << inet_ntoa(dest_address->sin_addr) << "\n";
    for (i = 0; i < num_agents; i++) {
        if (agents[i].active) {
            // Write the type of the agent
            buffer[length++] = agents[i].agentType;
            // Write agent's public IP address
            public_address = inet_ntoa(agents[i].public_sin_addr);
            memcpy(&buffer[length], public_address, strlen(public_address));
            length += strlen(public_address);
            //  Add public port number
            buffer[length++] = ' ';
            sprintf(public_portstring, "%hd", agents[i].public_port);
            memcpy(&buffer[length], public_portstring, strlen(public_portstring));
            length += strlen(public_portstring);
            // Write agent's private IP address
            buffer[length++] = ' ';
            private_address = agents[i].private_addr;
            memcpy(&buffer[length], private_address, strlen(private_address));
            length += strlen(private_address);
            // Add private port number
            buffer[length++] = ' ';
            sprintf(private_portstring, "%hd,", agents[i].private_port);
            memcpy(&buffer[length], private_portstring, strlen(private_portstring));
            length += strlen(private_portstring);
            numSent++;
        }
    }
    
    std::cout << "The sent buffer was " << buffer << "\n";
    
    if (length > 1) {
        sentBytes = domainSocket.send(agentAddrPointer, buffer, length);
        
        if (sentBytes < length)
            std::cout << "Error sending agent list!\n";
        else if (numSent != lastActiveCount) {
            std::cout << numSent << " Active Agents\n";
            lastActiveCount = numSent;
        }
    }
    
}

int main(int argc, const char * argv[])
{
    ssize_t receivedBytes = 0;
    timeval time, last_time;
    sockaddr_in agentAddress;
    
    gettimeofday(&last_time, NULL);
    
    while (true) {
        if (domainSocket.receive(&agentAddress, packet_data, &receivedBytes)) {
            float x,y,z;
            char agentType;
            char private_ip[50];
            unsigned short private_port;
            sscanf(packet_data, "%c %f,%f,%f,%s %hd", &agentType, &x, &y, &z, private_ip, &private_port);
            if (addAgent(agentAddress.sin_addr.s_addr, ntohs(agentAddress.sin_port), private_ip, private_port, agentType, x, y, z)) {
                std::cout << "Added Agent, type " << agentType << " from " <<
                inet_ntoa(agentAddress.sin_addr) << ":" << ntohs(agentAddress.sin_port) <<
                " (" << private_ip << ":" << private_port << ")" << "\n";
            }
            //  Reply with packet listing nearby active agents
            send_agent_list(&agentAddress);
        }
        
        gettimeofday(&time, NULL);
        if (diffclock(last_time, time) > LOGOFF_CHECK_INTERVAL) {
            gettimeofday(&last_time, NULL);
            update_agent_list(last_time);
        }
    }
    return 0;
}

