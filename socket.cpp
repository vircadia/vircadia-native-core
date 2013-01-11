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
#include <thread>

using namespace std;

const int UDP_PORT = 55443; 

sockaddr_in address, dest_address;
socklen_t destLength = sizeof( dest_address );

double diffclock(timeval clock1,timeval clock2)
{
    double diffms = (clock2.tv_sec - clock1.tv_sec) * 1000.0;
    diffms += (clock2.tv_usec - clock1.tv_usec) / 1000.0;   // us to ms
    return diffms;
}

int network_init()
{
    //  Create socket 
    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if (handle <= 0) {
        printf( "failed to create socket\n" );
        return false;
    }
    
    //  Bind socket to port 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( (unsigned short) UDP_PORT );
    
    if (bind(handle, (const sockaddr*) &address, sizeof(sockaddr_in)) < 0) {
        printf( "failed to bind socket\n" );
        return false;
    }
    
    //  Set socket as non-blocking
    int nonBlocking = 1;
    if (fcntl( handle, F_SETFL, O_NONBLOCK, nonBlocking ) == -1) {
        printf( "failed to set non-blocking socket\n" );
        return false;
    }
    
    dest_address.sin_family = AF_INET;
    dest_address.sin_addr.s_addr = inet_addr(DESTINATION_IP);
    dest_address.sin_port = htons( (unsigned short) UDP_PORT );
        
    return handle;
}

int main(int argc, const char * argv[])
{
    int received_bytes = 0;
    timeval time, last_time; 
    
    int handle = network_init();
    
    if (!handle) {
        cout << "Failed to create network.\n";
        return 0;
    } else {
        cout << "Network Started.  Waiting for packets.\n";
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
            if (addAgent(dest_address.sin_addr.s_addr, x, y, z)) {
                cout << "Added agent from IP: " << 
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

