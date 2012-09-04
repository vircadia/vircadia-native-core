//
//  network.cpp
//  interface
//
//  Created by Philip Rosedale on 8/27/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "network.h"
 

const int UDP_PORT = 30000; 
const char DESTINATION_IP[] = "127.0.0.1";

sockaddr_in address, dest_address, from;
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
    
    from.sin_family = AF_INET;
    //from.sin_addr.s_addr = htonl(ip_address);
    from.sin_port = htons( (unsigned short) UDP_PORT );
    
    
    return handle;
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

int network_receive(int handle, char * packet_data)
{
    int received_bytes = recvfrom(handle, (char*)packet_data, MAX_PACKET_SIZE,
                                  0, (sockaddr*)&dest_address, &fromLength );
    
    return received_bytes;

}
