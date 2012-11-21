//
//  network.cpp
//  interface
//
//  Created by Philip Rosedale on 8/27/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include "network.h"
 

//  Implementation of optional delay behavior using a ring buffer
const int MAX_DELAY_PACKETS = 300;
char delay_buffer[MAX_PACKET_SIZE*MAX_DELAY_PACKETS];
timeval delay_time_received[MAX_DELAY_PACKETS];
int delay_size_received[MAX_DELAY_PACKETS];
int next_to_receive = 0;
int next_to_send = 0;

sockaddr_in address, dest_address, spaceserver_address, from;
socklen_t fromLength = sizeof( from );

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
    address.sin_port = htons( (unsigned short) UDP_PORT );
    
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
    
    // Setup desination address 
    
    /*unsigned int ip_address;
    if (!inet_pton(AF_INET, DESTINATION_IP, &ip_address))
    {
        printf("failed to translate destination IP address\n");
        return false;
    }*/
                           
    dest_address.sin_family = AF_INET;
    dest_address.sin_addr.s_addr = inet_addr(DESTINATION_IP);
    dest_address.sin_port = htons( (unsigned short) UDP_PORT );

    spaceserver_address.sin_family = AF_INET;
    spaceserver_address.sin_addr.s_addr = inet_addr(SPACESERVER_IP);
    spaceserver_address.sin_port = htons( (unsigned short) SPACESERVER_PORT );

    from.sin_family = AF_INET;
    //from.sin_addr.s_addr = htonl(ip_address);
    from.sin_port = htons( (unsigned short) UDP_PORT );
    
    
    return handle;
}

//  Send a ping packet and mark the time sent
timeval network_send_ping(int handle) {
    timeval check;
    char packet_data[] = "P";
    sendto(handle, (const char*)packet_data, 1,
           0, (sockaddr*)&dest_address, sizeof(sockaddr_in) );
    gettimeofday(&check, NULL);
    return check; 
}

int notify_spaceserver(int handle, float x, float y, float z) {
    char data[100];
    sprintf(data, "%f,%f,%f", x, y, z);
    //std::cout << "sending: " << data << "\n";
    int packet_size = strlen(data);
    int sent_bytes = sendto( handle, (const char*)data, packet_size,
                            0, (sockaddr*)&spaceserver_address, sizeof(sockaddr_in) );
    
    if ( sent_bytes != packet_size )
    {
        printf( "failed to send to spaceserver: return value = %d\n", sent_bytes );
        return false;
    }
    return sent_bytes;
}

int network_send(int handle, char * packet_data, int packet_size)
{
    int sent_bytes = sendto( handle, (const char*)packet_data, packet_size,
                            0, (sockaddr*)&dest_address, sizeof(sockaddr_in) );

    if ( sent_bytes != packet_size )
    {
        printf( "failed to send packet: return value = %d\n", sent_bytes );
        return false;
    }
    return sent_bytes;
}

int network_receive(int handle, char * packet_data, int delay /*msecs*/)
{
    int received_bytes = recvfrom(handle, (char*)packet_data, MAX_PACKET_SIZE,
                                  0, (sockaddr*)&dest_address, &fromLength );
    if (!delay) {
        // No delay set, so just return packets immediately!
        return received_bytes;
    } else {
        timeval check;
        gettimeofday(&check, NULL);
        if (received_bytes > 0) {
            // First write received data into ring buffer
            delay_time_received[next_to_receive] = check;
            delay_size_received[next_to_receive] = received_bytes;
            memcpy(&delay_buffer[next_to_receive*MAX_PACKET_SIZE], packet_data, received_bytes);
            next_to_receive++;
            if (next_to_receive == MAX_DELAY_PACKETS) next_to_receive = 0;
        }
        // Then check if next to be sent is past due, send if so 
        if ((next_to_receive != next_to_send) && 
            (diffclock(delay_time_received[next_to_send], check) > delay)) {
            int returned_bytes = delay_size_received[next_to_send];
            memcpy(packet_data, 
                   &delay_buffer[next_to_send*MAX_PACKET_SIZE], 
                   returned_bytes); 
            next_to_send++;
            if (next_to_send == MAX_DELAY_PACKETS) next_to_send = 0;
            return returned_bytes;
        } else {
            return 0;
        }
    }
        
        
    

}
