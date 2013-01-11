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

sockaddr_in address, dest_address;

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

void send_buffer_thread()
{
    // create our send socket
    int handle = create_socket();

    if (!handle) {
        std::cout << "Failed to create buffer send socket.\n";
        return 0;
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
    
    while (1) {
        received_bytes = recvfrom(handle, (char*)packet_data, MAX_PACKET_SIZE,
                                      0, (sockaddr*)&dest_address, sizeof(dest_address));
        if (received_bytes > 0) {
            // std::cout << "Packet from: " << inet_ntoa(dest_address.sin_addr) 
            // << " " << packet_data << "\n";
            float x,y,z;
            sscanf(packet_data, "%f,%f,%f", &x, &y, &z);
            if (addAgent(dest_address.sin_addr.s_addr, x, y, z)) {
                std::cout << "Added agent from IP: " << 
                inet_ntoa(dest_address.sin_addr) << "\n";
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

