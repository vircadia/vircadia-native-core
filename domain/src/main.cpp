//
//  main.cpp
//  Domain Server 
//
//  Created by Philip Rosedale on 11/20/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
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

//  Domain server listens for packets on this port number
const int LISTENING_UDP_PORT = 40102;


const char DESTINATION_IP[] = "127.0.0.1";
sockaddr_in address, dest_address;
socklen_t destLength = sizeof( dest_address );


const int MAX_PACKET_SIZE = 1500;
char packet_data[MAX_PACKET_SIZE];

const int MAX_AGENTS = 1000;
const int LOGOFF_CHECK_INTERVAL = 2000;

struct AgentList {
    uint32_t ip;
    in_addr sin_addr;
    in_port_t port;
    float x, y, z;
    bool active;
    timeval time;
} agents[MAX_AGENTS];

int num_agents = 0;

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

int addAgent(uint32_t ip, in_port_t port, float x, float y, float z) {
    //  Search for agent in list and add if needed 
    int i = 0;
    int is_new = 0; 
    while ((ip != agents[i].ip) && (i < num_agents))  {
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
    gettimeofday(&agents[i].time, NULL);
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
            std::cout << "Expired Agent #" << i+1 << "\n";
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
    buffer[length++] = 'S';
    //std::cout << "send list to:  " << inet_ntoa(dest_address->sin_addr) << "\n";
    for (i = 0; i < num_agents; i++) {
        if (agents[i].active) {
            // Write agent's IP address
            address = inet_ntoa(agents[i].sin_addr);
            memcpy(&buffer[length], address, strlen(address));
            length += strlen(address);
            //  Add port number
            buffer[length++] = ':';
            sprintf(portstring, "%d\n", agents[i].port);
            memcpy(&buffer[length], portstring, strlen(portstring));
            length += strlen(portstring);
            //  Add comma separator between agents 
            buffer[length++] = ',';
        }
    }
    if (length > 1) {
        sent_bytes = (ssize_t) sendto( handle, (const char*)buffer, length,
                            0, (sockaddr *) dest_address, sizeof(sockaddr_in) );
        if (sent_bytes < length) 
            std::cout << "Error sending agent list!\n";
         else
            std::cout << "Agent list sent: " << buffer << "\n";
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
        std::cout << "DomainServer Started.  Waiting for packets.\n";
    }
    gettimeofday(&last_time, NULL);
    
    while (1) {
        received_bytes = recvfrom(handle, (char*)packet_data, MAX_PACKET_SIZE,
                                      0, (sockaddr*)&dest_address, &destLength );
        if (received_bytes > 0) {
            //std::cout << "Packet from: " << inet_ntoa(dest_address.sin_addr) 
            //<< " " << packet_data << "\n";
            float x,y,z;
            sscanf(packet_data, "%f,%f,%f", &x, &y, &z);
            if (addAgent(dest_address.sin_addr.s_addr, dest_address.sin_port, x, y, z)) {
                std::cout << "Added agent from IP, port: " << 
                inet_ntoa(dest_address.sin_addr) << "," << dest_address.sin_port << "\n";
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

