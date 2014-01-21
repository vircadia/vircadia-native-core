//
//  DataServerClient.cpp
//  hifi
//
//  Created by Stephen Birarda on 10/7/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QUrl>
#include <QtNetwork/QUdpSocket>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <UUID.h>

#include "avatar/Profile.h"

#include "DataServerClient.h"

QMap<quint8, QByteArray> DataServerClient::_unmatchedPackets;
QMap<quint8, DataServerCallbackObject*> DataServerClient::_callbackObjects;
QString DataServerClient::_clientIdentifier;
quint8 DataServerClient::_sequenceNumber = 0;

const char MULTI_KEY_VALUE_SEPARATOR = '|';

const char DATA_SERVER_HOSTNAME[] = "localhost";
const unsigned short DATA_SERVER_PORT = 3282;

const HifiSockAddr& DataServerClient::dataServerSockAddr() {
    static HifiSockAddr dsSockAddr = HifiSockAddr(DATA_SERVER_HOSTNAME, DATA_SERVER_PORT);
    return dsSockAddr;
}

void DataServerClient::putValueForKey(const QString& key, const char* value) {
    if (!_clientIdentifier.isEmpty()) {
        
        unsigned char putPacket[MAX_PACKET_SIZE];
        
        // setup the header for this packet
        int numPacketBytes = populateTypeAndVersion(putPacket, PACKET_TYPE_DATA_SERVER_PUT);
        
        // pack the sequence number
        memcpy(putPacket + numPacketBytes, &_sequenceNumber, sizeof(_sequenceNumber));
        numPacketBytes += sizeof(_sequenceNumber);
        
        // pack the client UUID, null terminated
        memcpy(putPacket + numPacketBytes, qPrintable(_clientIdentifier), _clientIdentifier.toLocal8Bit().size());
        numPacketBytes += _clientIdentifier.toLocal8Bit().size();
        putPacket[numPacketBytes++] = '\0';

        // pack a 1 to designate that we are putting a single value
        putPacket[numPacketBytes++] = 1;

        // pack the key, null terminated
        strcpy((char*) putPacket + numPacketBytes, key.toLocal8Bit().constData());
        numPacketBytes += key.size();
        putPacket[numPacketBytes++] = '\0';

        // pack the value, null terminated
        strcpy((char*) putPacket + numPacketBytes, value);
        numPacketBytes += strlen(value);
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
}

void DataServerClient::getValueForKeyAndUUID(const QString& key, const QUuid &uuid, DataServerCallbackObject* callbackObject) {
    getValuesForKeysAndUUID(QStringList(key), uuid, callbackObject);
}

void DataServerClient::getValuesForKeysAndUUID(const QStringList& keys, const QUuid& uuid, DataServerCallbackObject* callbackObject) {
    if (!uuid.isNull()) {
        getValuesForKeysAndUserString(keys, uuidStringWithoutCurlyBraces(uuid), callbackObject);
    }
}

void DataServerClient::getValuesForKeysAndUserString(const QStringList& keys, const QString& userString,
                                                     DataServerCallbackObject* callbackObject) {
    if (!userString.isEmpty() && keys.size() <= UCHAR_MAX) {
        unsigned char getPacket[MAX_PACKET_SIZE];
        
        // setup the header for this packet
        int numPacketBytes = populateTypeAndVersion(getPacket, PACKET_TYPE_DATA_SERVER_GET);
        
        // pack the sequence number
        memcpy(getPacket + numPacketBytes, &_sequenceNumber, sizeof(_sequenceNumber));
        numPacketBytes += sizeof(_sequenceNumber);

        // pack the user string (could be username or UUID string), null-terminate
        memcpy(getPacket + numPacketBytes, userString.toLocal8Bit().constData(), userString.toLocal8Bit().size());
        numPacketBytes += userString.toLocal8Bit().size();
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

void DataServerClient::getClientValueForKey(const QString& key, DataServerCallbackObject* callbackObject) {
    if (!_clientIdentifier.isEmpty()) {
        getValuesForKeysAndUserString(QStringList(key), _clientIdentifier, callbackObject);
    } else {
        qDebug() << "There is no client identifier set for the DataServerClient.";
    }
}

void DataServerClient::processConfirmFromDataServer(unsigned char* packetData) {
    removeMatchedPacketFromMap(packetData);
}

void DataServerClient::processSendFromDataServer(unsigned char* packetData, int numPacketBytes) {
    // pull the user string from the packet so we know who to associate this with
    int numHeaderBytes = numBytesForPacketHeader(packetData);
    
    _sequenceNumber = *(packetData + numHeaderBytes);
    
    if (_callbackObjects.find(_sequenceNumber) != _callbackObjects.end()) {
        DataServerCallbackObject* callbackObject = _callbackObjects.take(_sequenceNumber);
        
        char* userStringPosition = (char*) packetData + numHeaderBytes + sizeof(_sequenceNumber);
        QString userString(userStringPosition);
        
        char* keysPosition = userStringPosition + userString.size() + sizeof('\0') + sizeof(unsigned char);
        char* valuesPosition =  keysPosition + strlen(keysPosition) + sizeof('\0');
        
        QStringList keyList = QString(keysPosition).split(MULTI_KEY_VALUE_SEPARATOR);
        QStringList valueList = QString(valuesPosition).split(MULTI_KEY_VALUE_SEPARATOR);
        
        callbackObject->processDataServerResponse(userString, keyList, valueList);
    }

    
    // user string was UUID, find matching avatar and associate data
//    for (int i = 0; i < keyList.size(); i++) {
//        if (valueList[i] != " ") {
//            if (keyList[i] == DataServerKey::FaceMeshURL) {
    
//                if (userUUID.isNull() || userUUID == Application::getInstance()->getProfile()->getUUID()) {
//                    qDebug("Changing user's face model URL to %s\n", valueList[i].toLocal8Bit().constData());
//                    Application::getInstance()->getProfile()->setFaceModelURL(QUrl(valueList[i]));
//                } else {
//                    // mesh URL for a UUID, find avatar in our list
//                    
//                    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
//                        if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
//                            Avatar* avatar = (Avatar *) node->getLinkedData();
//                            
//                            if (avatar->getUUID() == userUUID) {
//                                QMetaObject::invokeMethod(&avatar->getHead().getFaceModel(),
//                                                          "setURL", Q_ARG(QUrl, QUrl(valueList[i])));
//                            }
//                        }
//                    }
//                }
//            } else if (keyList[i] == DataServerKey::SkeletonURL) {
    
//                if (userUUID.isNull() || userUUID == Application::getInstance()->getProfile()->getUUID()) {
//                    qDebug("Changing user's skeleton URL to %s\n", valueList[i].toLocal8Bit().constData());
//                    Application::getInstance()->getProfile()->setSkeletonModelURL(QUrl(valueList[i]));
//                } else {
//                    // skeleton URL for a UUID, find avatar in our list
//                    foreach (const SharedNodePointer& node, NodeList::getInstance()->getNodeHash()) {
//                        if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
//                            Avatar* avatar = (Avatar *) node->getLinkedData();
//                            
//                            if (avatar->getUUID() == userUUID) {
//                                QMetaObject::invokeMethod(&avatar->getSkeletonModel(), "setURL",
//                                                          Q_ARG(QUrl, QUrl(valueList[i])));
//                            }
//                        }
//                    }
//                }
//            } else if (keyList[i] == DataServerKey::Domain && keyList[i + 1] == DataServerKey::Position &&
//                    keyList[i + 2] == DataServerKey::Orientation && valueList[i] != " " &&
//                    valueList[i + 1] != " " && valueList[i + 2] != " ") {
//                
//                QStringList coordinateItems = valueList[i + 1].split(',');
//                QStringList orientationItems = valueList[i + 2].split(',');
    
//                if (coordinateItems.size() == 3 && orientationItems.size() == 3) {
//                    
//                    // send a node kill request, indicating to other clients that they should play the "disappeared" effect
//                    NodeList::getInstance()->sendKillNode(&NODE_TYPE_AVATAR_MIXER, 1);
//                    
//                    qDebug() << "Changing domain to" << valueList[i].toLocal8Bit().constData() <<
//                        ", position to" << valueList[i + 1].toLocal8Bit().constData() <<
//                        ", and orientation to" << valueList[i + 2].toLocal8Bit().constData() <<
//                        "to go to" << userString << "\n";
//                    
//                    NodeList::getInstance()->setDomainHostname(valueList[i]);
//                    
//                    // orient the user to face the target
//                    glm::quat newOrientation = glm::quat(glm::radians(glm::vec3(orientationItems[0].toFloat(),
//                        orientationItems[1].toFloat(), orientationItems[2].toFloat()))) *
//                            glm::angleAxis(180.0f, 0.0f, 1.0f, 0.0f);
//                    Application::getInstance()->getAvatar()->setOrientation(newOrientation);
//                    
//                    // move the user a couple units away
//                    const float DISTANCE_TO_USER = 2.0f;
//                    glm::vec3 newPosition = glm::vec3(coordinateItems[0].toFloat(), coordinateItems[1].toFloat(),
//                        coordinateItems[2].toFloat()) - newOrientation * IDENTITY_FRONT * DISTANCE_TO_USER;
//                    Application::getInstance()->getAvatar()->setPosition(newPosition);
//                }
                
//            } else if (keyList[i] == DataServerKey::UUID) {
                // this is the user's UUID - set it on the profile
//                Application::getInstance()->getProfile()->setUUID(valueList[i]);
    
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
