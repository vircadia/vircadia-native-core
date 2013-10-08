//
//  DataServerClient.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QUrl>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <UDPSocket.h>
#include <UUID.h>

#include "Application.h"
#include "Profile.h"

#include "DataServerClient.h"


std::map<unsigned char*, int> DataServerClient::_unmatchedPackets;

const char DATA_SERVER_HOSTNAME[] = "127.0.0.1";
const unsigned short DATA_SERVER_PORT = 3282;
const sockaddr_in DATA_SERVER_SOCKET = socketForHostnameAndHostOrderPort(DATA_SERVER_HOSTNAME, DATA_SERVER_PORT);

void DataServerClient::putValueForKey(const char* key, const char* value) {
    Profile* userProfile = Application::getInstance()->getProfile();
    QString clientString = userProfile->getUUID().isNull() ? userProfile->getUsername() : userProfile->getUUID().toString();
    
    Application::getInstance();
    
    unsigned char* putPacket = new unsigned char[MAX_PACKET_SIZE];
    
    // setup the header for this packet
    int numPacketBytes = populateTypeAndVersion(putPacket, PACKET_TYPE_DATA_SERVER_PUT);
    
    // pack the client UUID, null terminated
    memcpy(putPacket + numPacketBytes, clientString.toLocal8Bit().constData(), clientString.toLocal8Bit().size());
    numPacketBytes += clientString.toLocal8Bit().size();
    putPacket[numPacketBytes++] = '\0';
    
    // pack the key, null terminated
    strcpy((char*) putPacket + numPacketBytes, key);
    numPacketBytes += strlen(key);
    putPacket[numPacketBytes++] = '\0';
    
    // pack the value, null terminated
    strcpy((char*) putPacket + numPacketBytes, value);
    numPacketBytes += strlen(value);
    putPacket[numPacketBytes++] = '\0';
    
    // add the putPacket to our vector of unconfirmed packets, will be deleted once put is confirmed
    _unmatchedPackets.insert(std::pair<unsigned char*, int>(putPacket, numPacketBytes));
    
    // send this put request to the data server
    NodeList::getInstance()->getNodeSocket()->send((sockaddr*) &DATA_SERVER_SOCKET, putPacket, numPacketBytes);
}

void DataServerClient::getValueForKeyAndUUID(const char* key, QUuid &uuid) {
    if (!uuid.isNull()) {
        QString uuidString = uuidStringWithoutCurlyBraces(uuid);
        getValueForKeyAndUserString(key, uuidString);
    }
}

void DataServerClient::getValueForKeyAndUserString(const char* key, QString& userString) {
    unsigned char getPacket[MAX_PACKET_SIZE];
    
    // setup the header for this packet
    int numPacketBytes = populateTypeAndVersion(getPacket, PACKET_TYPE_DATA_SERVER_GET);
    
    // pack the user string (could be username or UUID string), null-terminate
    memcpy(getPacket + numPacketBytes, userString.toLocal8Bit().constData(), userString.toLocal8Bit().size());
    numPacketBytes += userString.toLocal8Bit().size();
    getPacket[numPacketBytes++] = '\0';
    
    // pack the key, null terminated
    strcpy((char*) getPacket + numPacketBytes, key);
    int numKeyBytes = strlen(key);
    
    if (numKeyBytes > 0) {
        numPacketBytes += numKeyBytes;
        getPacket[numPacketBytes++] = '\0';
    }
    
    // add the getPacket to our vector of uncofirmed packets, will be deleted once we get a response from the nameserver
    _unmatchedPackets.insert(std::pair<unsigned char*, int>(getPacket, numPacketBytes));
    
    // send the get to the data server
    NodeList::getInstance()->getNodeSocket()->send((sockaddr*) &DATA_SERVER_SOCKET, getPacket, numPacketBytes);
}

void DataServerClient::getClientValueForKey(const char* key) {
    getValueForKeyAndUserString(key, Application::getInstance()->getProfile()->getUsername());
}

void DataServerClient::processConfirmFromDataServer(unsigned char* packetData, int numPacketBytes) {
    removeMatchedPacketFromMap(packetData, numPacketBytes);
}

void DataServerClient::processSendFromDataServer(unsigned char* packetData, int numPacketBytes) {
    // pull the user string from the packet so we know who to associate this with
    int numHeaderBytes = numBytesForPacketHeader(packetData);
        
    QString userString(QByteArray((char*) packetData + numHeaderBytes, strlen((char*) packetData + numHeaderBytes)));
    
    QUuid userUUID(userString);
    
    if (userUUID.isNull()) {
        // the user string was a username
        // for now assume this means that it is for our avatar
        
        char* dataKeyPosition = (char*) packetData + numHeaderBytes + sizeof(userString);
        char* dataValuePosition =  dataKeyPosition + strlen(dataKeyPosition) + sizeof(char);
        
        QString dataValueString(QByteArray(dataValuePosition,
                                           numPacketBytes - ((unsigned char*) dataValuePosition - packetData)));
        
        if (strcmp(dataKeyPosition, DataServerKey::FaceMeshURL) == 0) {
            // pull the user's face mesh and set it on the Avatar instance
            
            
            qDebug("Changing user's face model URL to %s\n", dataValueString.toLocal8Bit().constData());
            QMetaObject::invokeMethod(&Application::getInstance()->getAvatar()->getHead().getBlendFace(),
                                      "setModelURL",
                                      Q_ARG(QUrl, QUrl(dataValueString)));
        } else if (strcmp(dataKeyPosition, DataServerKey::UUID) == 0) {
            // this is the user's UUID - set it on the profile
            Application::getInstance()->getProfile()->setUUID(dataValueString);
        }
    } else {
        // user string was UUID, find matching avatar and associate data
        
    }
    
    // remove the matched packet from  our map so it isn't re-sent to the data-server
    removeMatchedPacketFromMap(packetData, numPacketBytes);
}

void DataServerClient::processMessageFromDataServer(unsigned char* packetData, int numPacketBytes) {
    switch (packetData[0]) {
        case PACKET_TYPE_DATA_SERVER_SEND:
            processSendFromDataServer(packetData, numPacketBytes);
            break;
        case PACKET_TYPE_DATA_SERVER_CONFIRM:
            processConfirmFromDataServer(packetData, numPacketBytes);
            break;
        default:
            break;
    }
}

void DataServerClient::removeMatchedPacketFromMap(unsigned char* packetData, int numPacketBytes) {
    for  (std::map<unsigned char*, int>::iterator mapIterator = _unmatchedPackets.begin();
          mapIterator != _unmatchedPackets.end();
          ++mapIterator) {
        if (memcmp(mapIterator->first + sizeof(PACKET_TYPE),
                   packetData + sizeof(PACKET_TYPE),
                   numPacketBytes - sizeof(PACKET_TYPE)) == 0) {
            
            // this is a match - remove the confirmed packet from the vector and delete associated member
            // so it isn't sent back out
            delete[] mapIterator->first;
            _unmatchedPackets.erase(mapIterator);
            
            // we've matched the packet - bail out
            break;
        }
    }
}

void DataServerClient::resendUnmatchedPackets() {
    for  (std::map<unsigned char*, int>::iterator mapIterator = _unmatchedPackets.begin();
          mapIterator != _unmatchedPackets.end();
          ++mapIterator) {
        // send the unmatched packet to the data server
        NodeList::getInstance()->getNodeSocket()->send((sockaddr*) &DATA_SERVER_SOCKET,
                                                       mapIterator->first,
                                                       mapIterator->second);
    }
}
