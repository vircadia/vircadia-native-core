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

void DataServerClient::putValueForKeyAndUserString(const QString& key, const QString& value, const QString& userString) {
    // setup the header for this packet and push packetStream to desired spot
    QByteArray putPacket = byteArrayWithPopluatedHeader(PacketTypeDataServerPut);
    QDataStream packetStream(&putPacket, QIODevice::Append);
    
    // pack our data for the put packet
    packetStream << _sequenceNumber << userString << key << value;
    
    // add the putPacket to our vector of unconfirmed packets, will be deleted once put is confirmed
    _unmatchedPackets.insert(_sequenceNumber, putPacket);
    
    // send this put request to the data server
    NodeList::getInstance()->getNodeSocket().writeDatagram(putPacket, dataServerSockAddr().getAddress(),
                                                           dataServerSockAddr().getPort());
    
    // push the sequence number forwards
    _sequenceNumber++;
}

void DataServerClient::putValueForKeyAndUUID(const QString& key, const QString& value, const QUuid& uuid) {
    putValueForKeyAndUserString(key, value, uuidStringWithoutCurlyBraces(uuid));
}

void DataServerClient::getValueForKeyAndUUID(const QString& key, const QUuid &uuid, DataServerCallbackObject* callbackObject) {
    getValuesForKeysAndUUID(QStringList(key), uuid, callbackObject);
}

void DataServerClient::getValuesForKeysAndUUID(const QStringList& keys, const QUuid& uuid,
                                               DataServerCallbackObject* callbackObject) {
    if (!uuid.isNull()) {
        getValuesForKeysAndUserString(keys, uuidStringWithoutCurlyBraces(uuid), callbackObject);
    }
}

void DataServerClient::getValuesForKeysAndUserString(const QStringList& keys, const QString& userString,
                                                   DataServerCallbackObject* callbackObject) {
    if (!userString.isEmpty() && keys.size() <= UCHAR_MAX) {
        QByteArray getPacket = byteArrayWithPopluatedHeader(PacketTypeDataServerGet);
        QDataStream packetStream(&getPacket, QIODevice::Append);
        
        // pack our data for the getPacket
        packetStream << _sequenceNumber << userString << keys.join(MULTI_KEY_VALUE_SEPARATOR);
        
        // add the getPacket to our map of unconfirmed packets, will be deleted once we get a response from the nameserver
        _unmatchedPackets.insert(_sequenceNumber, getPacket);
        _callbackObjects.insert(_sequenceNumber, callbackObject);
        
        // send the get to the data server
        NodeList::getInstance()->getNodeSocket().writeDatagram(getPacket, dataServerSockAddr().getAddress(),
                                                               dataServerSockAddr().getPort());
        _sequenceNumber++;
    }
}

void DataServerClient::getValueForKeyAndUserString(const QString& key, const QString& userString,
                                                   DataServerCallbackObject* callbackObject) {
    getValuesForKeysAndUserString(QStringList(key), userString, callbackObject);
}

void DataServerClient::processConfirmFromDataServer(const QByteArray& packet) {
    removeMatchedPacketFromMap(packet);
}

void DataServerClient::processSendFromDataServer(const QByteArray& packet) {
    // pull the user string from the packet so we know who to associate this with
    QDataStream packetStream(packet);
    packetStream.skipRawData(numBytesForPacketHeader(packet));
    
    quint8 sequenceNumber = 0;
    packetStream >> sequenceNumber;
    
    if (_callbackObjects.find(sequenceNumber) != _callbackObjects.end()) {
        // remove the packet from our two maps, it's matched
        DataServerCallbackObject* callbackObject = _callbackObjects.take(sequenceNumber);
        _unmatchedPackets.remove(sequenceNumber);
        
        QString userString, keyListString, valueListString;
        
        packetStream >> userString >> keyListString >> valueListString;
        
        callbackObject->processDataServerResponse(userString, keyListString.split(MULTI_KEY_VALUE_SEPARATOR),
                                                  valueListString.split(MULTI_KEY_VALUE_SEPARATOR));
    }
}

void DataServerClient::processMessageFromDataServer(const QByteArray& packet) {
    switch (packetTypeForPacket(packet)) {
        case PacketTypeDataServerSend:
            processSendFromDataServer(packet);
            break;
        case PacketTypeDataServerConfirm:
            processConfirmFromDataServer(packet);
            break;
        default:
            break;
    }
}

void DataServerClient::removeMatchedPacketFromMap(const QByteArray& packet) {
    quint8 sequenceNumber = packet[numBytesForPacketHeader(packet)];
    
    // attempt to remove a packet with this sequence number from the QMap of unmatched packets
    _unmatchedPackets.remove(sequenceNumber);
}

void DataServerClient::resendUnmatchedPackets() {
    if (_unmatchedPackets.size() > 0) {
        qDebug() << "Resending" << _unmatchedPackets.size() << "packets to the data server.";
        
        foreach (const QByteArray& packet, _unmatchedPackets) {
            // send the unmatched packet to the data server
            NodeList::getInstance()->getNodeSocket().writeDatagram(packet.data(), packet.size(),
                                                                   dataServerSockAddr().getAddress(),
                                                                   dataServerSockAddr().getPort());
        }
    }
}
