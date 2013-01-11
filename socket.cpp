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

const int UDP_PORT = 55443; 
const int MAX_PACKET_SIZE = 1024;

const float SAMPLE_RATE = 22050.0;
const int SAMPLES_PER_PACKET = 512;

const int MAX_AGENTS = 1000;
const int LOGOFF_CHECK_INTERVAL = 1000;

sockaddr_in address, dest_address;
socklen_t destLength = sizeof(dest_address);

struct AgentList {
    uint32_t ip;
    in_addr sin_addr;
    unsigned short port;
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

int create_socket()
{
    //  Create socket 
    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (handle <= 0) {
        printf( "failed to create socket\n" );
        return false;
    }

    return handle;
}

int network_init()
{
    int handle = create_socket();
    
    //  Bind socket to port 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( (unsigned short) UDP_PORT );
    
    if (bind(handle, (const sockaddr*) &address, sizeof(sockaddr_in)) < 0) {
        printf( "failed to bind socket\n" );
        return false;
    }   
        
    return handle;
}

int addAgent(uint32_t ip, unsigned short port) {
    //  Search for agent in list and add if needed 
    int i = 0;
    int is_new = 0; 
    
    while ((ip != agents[i].ip) && (i < num_agents))  {
        i++; 
    }
    
    if ((i == num_agents) || (agents[i].active == false)) is_new = 1;
    
    agents[i].ip = ip; 
    agents[i].port = port; 
    agents[i].active = true;
    agents[i].sin_addr.s_addr = ip;
    gettimeofday(&agents[i].time, NULL);
    
    if (i == num_agents) {
        num_agents++;
    }

    return is_new;
}

void update_agent_list(timeval now) {
    int i;
    // std::cout << "Checking agent list" << "\n";
    for (i = 0; i < num_agents; i++) {
        if ((diffclock(agents[i].time, now) > LOGOFF_CHECK_INTERVAL) &&
            agents[i].active) {
            std::cout << "Expired Agent #" << i << "\n";
            agents[i].active = false; 
        }
    }
}

void send_buffer_thread()
{
    // create our send socket
    int handle = create_socket();

    if (!handle) {
        std::cout << "Failed to create buffer send socket.\n";
        // return 0;
    } else {
        std::cout << "Buffer send socket created.\n";
    }

    while (1) {
        // sleep for the length of a packet of audio
        sleep((SAMPLES_PER_PACKET/SAMPLE_RATE) * pow(10, 6));

        // send out whatever we have in the buffer as mixed audio
        // to our recent clients

    }
}

int main(int argc, const char * argv[])
{
    int received_bytes = 0;
    timeval time, last_time; 
    
    int handle = network_init();
    
    if (!handle) {
        std::cout << "Failed to create listening socket.\n";
        return 0;
    } else {
        std::cout << "Network Started.  Waiting for packets.\n";
    }

    gettimeofday(&last_time, NULL);

    char packet_data[MAX_PACKET_SIZE];
    
    while (1) {
        received_bytes = recvfrom(handle, (char*)packet_data, MAX_PACKET_SIZE,
                                      0, (sockaddr*)&dest_address, &destLength);
        if (received_bytes > 0) {
            // std::cout << "Packet from: " << inet_ntoa(dest_address.sin_addr) 
            // << " " << packet_data << "\n";
            
            if (addAgent(dest_address.sin_addr.s_addr, dest_address.sin_port)) {
                std::cout << "Added agent: " << 
                inet_ntoa(dest_address.sin_addr) << " on " <<
                dest_address.sin_port << "\n";
            }
        }

        gettimeofday(&time, NULL);
        
        if (diffclock(last_time, time) > LOGOFF_CHECK_INTERVAL) {
            gettimeofday(&last_time, NULL);
            update_agent_list(last_time);
        }
    }

    return 0;
}

