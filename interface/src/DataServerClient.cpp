//
//  DataServerClient.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <NodeList.h>
#include <PacketHeaders.h>
#include <UDPSocket.h>

#include "DataServerClient.h"

const char DATA_SERVER_HOSTNAME[] = "data.highfidelity.io";
const unsigned short DATA_SERVER_PORT = 3282;
const sockaddr_in DATA_SERVER_SOCKET = socketForHostnameAndHostOrderPort(DATA_SERVER_HOSTNAME, DATA_SERVER_PORT);

void DataServerClient::putValueForKey(const char* key, const char* value) {
    unsigned char putPacket[MAX_PACKET_SIZE];
    
    // setup the header for this packet
    int numPacketBytes = populateTypeAndVersion(putPacket, PACKET_TYPE_DATA_SERVER_PUT);
    
    // pack the client UUID
    QByteArray rfcUUID = _clientUUID.toRfc4122();
    memcpy(putPacket + numPacketBytes, rfcUUID.constData(), rfcUUID.size());
    numPacketBytes += rfcUUID.size();
    
    // pack the key, null terminated
    strcpy((char*) putPacket + numPacketBytes, key);
    numPacketBytes += strlen(key);
    putPacket[numPacketBytes++] = '\0';
    
    // pack the value, null terminated
    strcpy((char*) putPacket + numPacketBytes, value);
    numPacketBytes += strlen(value);
    putPacket[numPacketBytes++] = '\0';
    
    // send this put request to the data server
    NodeList::getInstance()->getNodeSocket()->send((sockaddr*) &DATA_SERVER_SOCKET, putPacket, numPacketBytes);
}

void DataServerClient::getValueForKeyAndUUID(const char* key, QUuid &uuid) {
    unsigned char getPacket[MAX_PACKET_SIZE];
    
    // setup the header for this packet
    int numPacketBytes = populateTypeAndVersion(getPacket, PACKET_TYPE_DATA_SERVER_GET);
    
    // pack the UUID we're asking for data for
    QByteArray rfcUUID = uuid.toRfc4122();
    memcpy(getPacket + numPacketBytes, rfcUUID, rfcUUID.size());
    numPacketBytes += rfcUUID.size();
    
    // pack the key, null terminated
    strcpy((char*) getPacket + numPacketBytes, key);
    numPacketBytes += strlen(key);
    getPacket[numPacketBytes++] = '\0';
    
    // send the get to the data server
    NodeList::getInstance()->getNodeSocket()->send((sockaddr*) &DATA_SERVER_SOCKET, getPacket, numPacketBytes);
}

void DataServerClient::processConfirmFromDataServer(unsigned char* packetData, int numPacketBytes) {
    
}

void DataServerClient::processGetFromDataServer(unsigned char* packetData, int numPacketBytes) {
    
}
