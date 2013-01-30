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

//  Structure to hold references to other agents that are nearby 

const int MAX_AGENTS = 100; 
struct AgentList {
    in_addr sin_addr;
    Head head;
} agents[MAX_AGENTS];
int num_agents = 0; 

//
// Process an incoming spaceserver packet telling you about other nearby agents 
//
void update_agents(char * data, int length) {
    std::string packet(data, length);
    std::cout << " Update Agents, string: " << packet << "\n";
    size_t spot;
    size_t start_spot = 0;
    spot = packet.find_first_of (",", 0);
    while (spot != std::string::npos) {
        std::string IPstring = packet.substr(start_spot, spot-start_spot);
        //std::cout << "Found " << num_agents <<
        //" with IP " << IPstring << " from " << start_spot << " to " << spot << "\n";
        //  Add the IP address to the agent table
        add_agent(&IPstring);
        start_spot = spot + 1;
        if (start_spot < packet.length())
            spot = packet.find_first_of (",", start_spot);
        else spot = std::string::npos;
    }
}

void render_agents() {
    for (int i = 0; i < num_agents; i++) {
        glm::vec3 pos = agents[i].head.getPos();
        glPushMatrix();
        glTranslatef(pos.x, pos.y, pos.z);
        agents[i].head.render();
        glPopMatrix();
    }
}

//
//  Update a single agent with data received from that agent's IP address 
// 
void update_agent(in_addr addr, char * data, int length)
{
    std::cout << "Looking for agent: " << inet_ntoa(addr) << "\n";
    for (int i = 0; i < num_agents; i++) {
        if (agents[i].sin_addr.s_addr == addr.s_addr) {
            //  Update the agent 
            agents[i].head.recvBroadcastData(data, length);
        }
    }
}

//
//  Look for an agent by it's IP number, add if it does not exist in local list 
//
int add_agent(std::string * IP) {
    in_addr_t addr = inet_addr(IP->c_str());
    //std::cout << "Checking for " << IP->c_str() << "  ";
    for (int i = 0; i < num_agents; i++) {
        if (agents[i].sin_addr.s_addr == addr) {
            //std::cout << "Found!\n";
            return 0;
        }
    }
    if (num_agents < MAX_AGENTS) {
        agents[num_agents].sin_addr.s_addr = addr;
        std::cout << "Added Agent # " << num_agents << " with IP " << 
        inet_ntoa(agents[num_agents].sin_addr) << "\n";
        num_agents++;
        return 1;
    } else {
        std::cout << "Max agents reached fail!\n";
        return 0;
    } 
}

//
//  Broadcast data to all the other agents you are aware of, returns 1 for success 
//
int broadcast_to_agents(int handle, char * data, int length) {
    sockaddr_in dest_address;
    dest_address.sin_family = AF_INET;
    dest_address.sin_port = htons( (unsigned short) UDP_PORT );

    int sent_bytes;
    for (int i = 0; i < num_agents; i++) {
        dest_address.sin_addr.s_addr = agents[i].sin_addr.s_addr;
        sent_bytes = sendto( handle, (const char*)data, length,
                                0, (sockaddr*)&dest_address, sizeof(sockaddr_in) );
        if (sent_bytes != length) {
            std::cout << "Broadcast packet fail!\n";
            return 0;
        } 
    }
    return 1;
}
