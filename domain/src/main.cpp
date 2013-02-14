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


const int LISTENING_UDP_PORT = 40102;
const char DESTINATION_IP[] = "127.0.0.1";
sockaddr_in address, dest_address;
socklen_t destLength = sizeof( dest_address );


const int MAX_PACKET_SIZE = 1500;
char packet_data[MAX_PACKET_SIZE];

const int MAX_AGENTS = 1000;
const int LOGOFF_CHECK_INTERVAL = 5000;

struct AgentList {
    char agentType;
    uint32_t ip;
    in_addr sin_addr;
    in_port_t port;
    float x, y, z;
    bool active;
    timeval time, connectTime;
} agents[MAX_AGENTS];

int num_agents = 0;

int lastActiveCount = 0;

double diffclock(timeval clock1,timeval clock2)
{
	double diffms = (clock2.tv_sec - clock1.tv_sec) * 1000.0;
    diffms += (clock2.tv_usec - clock1.tv_usec) / 1000.0;   // us to ms
	return diffms;
}

int network_init()
{
    //  Create socket 
    int handle = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    
    if ( handle <= 0 )
    {
        printf( "failed to create socket\n" );
        return false;
    }
    
    //  Bind socket to port 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( (unsigned short) LISTENING_UDP_PORT );
    
    if ( bind( handle, (const sockaddr*) &address, sizeof(sockaddr_in) ) < 0 )
    {
        printf( "failed to bind socket\n" );
        return false;
    }
    
    //  Set socket as non-blocking
    int nonBlocking = 1;
    if ( fcntl( handle, F_SETFL, O_NONBLOCK, nonBlocking ) == -1 )
    {
        printf( "failed to set non-blocking socket\n" );
        return false;
    }
    
    dest_address.sin_family = AF_INET;
    dest_address.sin_addr.s_addr = inet_addr(DESTINATION_IP);
    dest_address.sin_port = htons( (unsigned short) LISTENING_UDP_PORT );
        
    return handle;
}

int addAgent(uint32_t ip, in_port_t port, char agentType, float x, float y, float z) {
    //  Search for agent in list and add if needed 
    int i = 0;
    int is_new = 0; 
    while ((ip != agents[i].ip || port != agents[i].port) && (i < num_agents))  {
        i++;
    }
    if ((i == num_agents) || (agents[i].active == false)) is_new = 1;
    agents[i].ip = ip; 
    agents[i].x = x; 
    agents[i].y = y; 
    agents[i].z = z;
    agents[i].active = true;
    agents[i].sin_addr.s_addr = ip;
    agents[i].port = port;
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
            inet_ntoa(agents[i].sin_addr) << ":" << agents[i].port << "\n";
            agents[i].active = false; 
        }
    }
}

void send_agent_list(int handle, sockaddr_in * dest_address) {
    int i, length = 0;
    ssize_t sent_bytes;
    char buffer[MAX_PACKET_SIZE];
    char * address;
    char portstring[10];
    int numSent = 0;
    buffer[length++] = 'D';
    //std::cout << "send list to:  " << inet_ntoa(dest_address->sin_addr) << "\n";
    for (i = 0; i < num_agents; i++) {
        if (agents[i].active) {
            // Write the type of the agent
            buffer[length++] = agents[i].agentType;
            // Write agent's IP address
            address = inet_ntoa(agents[i].sin_addr);
            memcpy(&buffer[length], address, strlen(address));
            length += strlen(address);
            //  Add port number
            buffer[length++] = ' ';
            sprintf(portstring, "%d\n", agents[i].port);
            memcpy(&buffer[length], portstring, strlen(portstring));
            length += strlen(portstring);
            //  Add comma separator between agents 
            buffer[length++] = ',';
            numSent++;
        }
    }
    if (length > 1) {
        sent_bytes = (ssize_t) sendto( handle, (const char*)buffer, length,
                            0, (sockaddr *) dest_address, sizeof(sockaddr_in) );
        if (sent_bytes < length) 
            std::cout << "Error sending agent list!\n";
        else if (numSent != lastActiveCount) {
            std::cout << numSent << " Active Agents\n";
            lastActiveCount = numSent;
        }
    }
    
}

int main(int argc, const char * argv[])
{
    ssize_t received_bytes = 0;
    //int sent_bytes = 0;
    //int packet_size = 0;
    timeval time, last_time; 
    
    int handle = network_init();
    if (!handle) {
        std::cout << "Failed to create network.\n";
        return 0;
    } else {
        std::cout << "DomainServer started, listening on port " << LISTENING_UDP_PORT << "\n";
    }
    gettimeofday(&last_time, NULL);
    
    while (1) {
        received_bytes = recvfrom(handle, (char*)packet_data, MAX_PACKET_SIZE,
                                      0, (sockaddr*)&dest_address, &destLength );
        if (received_bytes > 0) {
            //std::cout << "Packet from: " << inet_ntoa(dest_address.sin_addr) 
            //<< " " << packet_data << "\n";
            float x,y,z;
            char agentType; 
            sscanf(packet_data, "%c %f,%f,%f", &agentType, &x, &y, &z);
            if (addAgent(dest_address.sin_addr.s_addr, ntohs(dest_address.sin_port), agentType, x, y, z)) {
                std::cout << "Added Agent, type " << agentType << " from " <<
                inet_ntoa(dest_address.sin_addr) << ":" << ntohs(dest_address.sin_port) << "\n";
            }
            //  Reply with packet listing nearby active agents
            send_agent_list(handle, &dest_address);
        }
        gettimeofday(&time, NULL);
        if (diffclock(last_time, time) > LOGOFF_CHECK_INTERVAL) {
            gettimeofday(&last_time, NULL);
            update_agent_list(last_time);
        }
            
        usleep(10000);
    }
    return 0;
}

