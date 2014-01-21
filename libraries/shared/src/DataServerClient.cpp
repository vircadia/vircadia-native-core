//
//  DataServerClient.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QUrl>
#include <QtNetwork/QUdpSocket>

#include "NodeList.h"
#include "PacketHeaders.h"
#include "UUID.h"

#include "DataServerClient.h"

QMap<quint8, QByteArray> DataServerClient::_unmatchedPackets;
QMap<quint8, DataServerCallbackObject*> DataServerClient::_callbackObjects;
quint8 DataServerClient::_sequenceNumber = 0;

const char MULTI_KEY_VALUE_SEPARATOR = '|';

const char DATA_SERVER_HOSTNAME[] = "localhost";
const unsigned short DATA_SERVER_PORT = 3282;

const HifiSockAddr& DataServerClient::dataServerSockAddr() {
    static HifiSockAddr dsSockAddr = HifiSockAddr(DATA_SERVER_HOSTNAME, DATA_SERVER_PORT);
    return dsSockAddr;
}

void DataServerClient::putValueForKeyAndUsername(const QString& key, const QString& value, const QString& clientIdentifier) {
    unsigned char putPacket[MAX_PACKET_SIZE];
    
    // setup the header for this packet
    int numPacketBytes = populateTypeAndVersion(putPacket, PACKET_TYPE_DATA_SERVER_PUT);
    
    // pack the sequence number
    memcpy(putPacket + numPacketBytes, &_sequenceNumber, sizeof(_sequenceNumber));
    numPacketBytes += sizeof(_sequenceNumber);
    
    // pack the client UUID, null terminated
    memcpy(putPacket + numPacketBytes, qPrintable(clientIdentifier), clientIdentifier.toLocal8Bit().size());
    numPacketBytes += clientIdentifier.toLocal8Bit().size();
    putPacket[numPacketBytes++] = '\0';
    
    // pack a 1 to designate that we are putting a single value
    putPacket[numPacketBytes++] = 1;
    
    // pack the key, null terminated
    strcpy((char*) putPacket + numPacketBytes, qPrintable(key));
    numPacketBytes += key.size();
    putPacket[numPacketBytes++] = '\0';
    
    // pack the value, null terminated
    strcpy((char*) putPacket + numPacketBytes, qPrintable(value));
    numPacketBytes += value.size();
    putPacket[numPacketBytes++] = '\0';
    
    // add the putPacket to our vector of unconfirmed packets, will be deleted once put is confirmed
    _unmatchedPackets.insert(_sequenceNumber, QByteArray((char*) putPacket, numPacketBytes));
    
    // send this put request to the data server
    NodeList::getInstance()->getNodeSocket().writeDatagram((char*) putPacket, numPacketBytes,
                                                           dataServerSockAddr().getAddress(),
                                                           dataServerSockAddr().getPort());
    
    // push the sequence number forwards
    _sequenceNumber++;
}

void DataServerClient::putValueForKeyAndUUID(const QString& key, const QString& value, const QUuid& uuid) {
    putValueForKeyAndUsername(key, value, uuidStringWithoutCurlyBraces(uuid));
}

void DataServerClient::getValueForKeyAndUUID(const QString& key, const QUuid &uuid, DataServerCallbackObject* callbackObject) {
    getValuesForKeysAndUUID(QStringList(key), uuid, callbackObject);
}

void DataServerClient::getValuesForKeysAndUUID(const QStringList& keys, const QUuid& uuid, DataServerCallbackObject* callbackObject) {
    if (!uuid.isNull()) {
        getValuesForKeysAndUsername(keys, uuidStringWithoutCurlyBraces(uuid), callbackObject);
    }
}

void DataServerClient::getValuesForKeysAndUsername(const QStringList& keys, const QString& clientIdentifier,
                                                   DataServerCallbackObject* callbackObject) {
    if (!clientIdentifier.isEmpty() && keys.size() <= UCHAR_MAX) {
        unsigned char getPacket[MAX_PACKET_SIZE];
        
        // setup the header for this packet
        int numPacketBytes = populateTypeAndVersion(getPacket, PACKET_TYPE_DATA_SERVER_GET);
        
        // pack the sequence number
        memcpy(getPacket + numPacketBytes, &_sequenceNumber, sizeof(_sequenceNumber));
        numPacketBytes += sizeof(_sequenceNumber);

        // pack the user string (could be username or UUID string), null-terminate
        memcpy(getPacket + numPacketBytes, clientIdentifier.toLocal8Bit().constData(), clientIdentifier.size());
        numPacketBytes += clientIdentifier.size();
        getPacket[numPacketBytes++] = '\0';

        // pack one byte to designate the number of keys
        getPacket[numPacketBytes++] = keys.size();

        QString keyString = keys.join(MULTI_KEY_VALUE_SEPARATOR);

        // pack the key string, null terminated
        strcpy((char*) getPacket + numPacketBytes, keyString.toLocal8Bit().constData());
        numPacketBytes += keyString.size() + sizeof('\0');

        // add the getPacket to our map of unconfirmed packets, will be deleted once we get a response from the nameserver
        _unmatchedPackets.insert(_sequenceNumber, QByteArray((char*) getPacket, numPacketBytes));
        _callbackObjects.insert(_sequenceNumber, callbackObject);
        
        // send the get to the data server
        NodeList::getInstance()->getNodeSocket().writeDatagram((char*) getPacket, numPacketBytes,
                                                               dataServerSockAddr().getAddress(),
                                                               dataServerSockAddr().getPort());
                
        _sequenceNumber++;
    }
}

void DataServerClient::getClientValueForKey(const QString& key, const QString& clientIdentifier,
                                            DataServerCallbackObject* callbackObject) {
    getValuesForKeysAndUsername(QStringList(key), clientIdentifier, callbackObject);
}

void DataServerClient::processConfirmFromDataServer(unsigned char* packetData) {
    removeMatchedPacketFromMap(packetData);
}

void DataServerClient::processSendFromDataServer(unsigned char* packetData, int numPacketBytes) {
    // pull the user string from the packet so we know who to associate this with
    int numHeaderBytes = numBytesForPacketHeader(packetData);
    
    quint8 sequenceNumber = *(packetData + numHeaderBytes);
    
    if (_callbackObjects.find(_sequenceNumber) != _callbackObjects.end()) {
        // remove the packet from our two maps, it's matched
        DataServerCallbackObject* callbackObject = _callbackObjects.take(sequenceNumber);
        _unmatchedPackets.remove(sequenceNumber);
        
        char* userStringPosition = (char*) packetData + numHeaderBytes + sizeof(sequenceNumber);
        QString userString(userStringPosition);
        
        char* keysPosition = userStringPosition + userString.size() + sizeof('\0') + sizeof(unsigned char);
        char* valuesPosition =  keysPosition + strlen(keysPosition) + sizeof('\0');
        
        QStringList keyList = QString(keysPosition).split(MULTI_KEY_VALUE_SEPARATOR);
        QStringList valueList = QString(valuesPosition).split(MULTI_KEY_VALUE_SEPARATOR);
        
        callbackObject->processDataServerResponse(userString, keyList, valueList);
    }
}

void DataServerClient::processMessageFromDataServer(unsigned char* packetData, int numPacketBytes) {
    switch (packetData[0]) {
        case PACKET_TYPE_DATA_SERVER_SEND:
            processSendFromDataServer(packetData, numPacketBytes);
            break;
        case PACKET_TYPE_DATA_SERVER_CONFIRM:
            processConfirmFromDataServer(packetData);
            break;
        default:
            break;
    }
}

void DataServerClient::removeMatchedPacketFromMap(unsigned char* packetData) {
    quint8 sequenceNumber = *(packetData + numBytesForPacketHeader(packetData));
    
    // attempt to remove a packet with this sequence number from the QMap of unmatched packets
    _unmatchedPackets.remove(sequenceNumber);
}

void DataServerClient::resendUnmatchedPackets() {
    foreach (const QByteArray& packet, _unmatchedPackets) {
        // send the unmatched packet to the data server
        NodeList::getInstance()->getNodeSocket().writeDatagram(packet.data(), packet.size(),
                                                               dataServerSockAddr().getAddress(),
                                                               dataServerSockAddr().getPort());
    }
}
