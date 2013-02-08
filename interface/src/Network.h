//
//  Network.h
//  interface
//
//  Created by Philip Rosedale on 8/27/12.
//  Copyright (c) 2012 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Network__
#define __interface__Network__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include "Util.h"

//  Port to use for communicating UDP with other nearby agents 
const int MAX_PACKET_SIZE = 1500;
const int UDP_PORT = 30001; 
const char DESTINATION_IP[] = "127.0.0.1";

//  Address and port of domainserver process to advertise other agents 
const char DOMAINSERVER_IP[] = "127.0.0.1";
const int DOMAINSERVER_PORT = 40000;

//  Randomly send a ping packet every N packets sent
const int PING_PACKET_COUNT = 20;      

int network_init();
int network_send(int handle, char * packet_data, int packet_size);
int network_receive(int handle, in_addr * from_addr, char * packet_data, int delay /*msecs*/);
timeval network_send_ping(int handle);
int notify_domainserver(int handle, float x, float y, float z);

#endif
