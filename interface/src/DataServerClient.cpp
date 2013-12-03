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

#include "Application.h"
#include "avatar/Profile.h"

#include "DataServerClient.h"

std::map<unsigned char*, int> DataServerClient::_unmatchedPackets;

const char MULTI_KEY_VALUE_SEPARATOR = '|';

const char DATA_SERVER_HOSTNAME[] = "data.highfidelity.io";
const unsigned short DATA_SERVER_PORT = 3282;


const HifiSockAddr& DataServerClient::dataServerSockAddr() {
    static HifiSockAddr dsSockAddr = HifiSockAddr(DATA_SERVER_HOSTNAME, DATA_SERVER_PORT);
    return dsSockAddr;
}

void DataServerClient::putValueForKey(const QString& key, const char* value) {
    QString clientString = Application::getInstance()->getProfile()->getUserString();
    if (!clientString.isEmpty()) {
        
        unsigned char* putPacket = new unsigned char[MAX_PACKET_SIZE];
        
        // setup the header for this packet
        int numPacketBytes = populateTypeAndVersion(putPacket, PACKET_TYPE_DATA_SERVER_PUT);
        
        // pack the client UUID, null terminated
        memcpy(putPacket + numPacketBytes, clientString.toLocal8Bit().constData(), clientString.toLocal8Bit().size());
        numPacketBytes += clientString.toLocal8Bit().size();
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
        // _unmatchedPackets.insert(std::pair<unsigned char*, int>(putPacket, numPacketBytes));
        
        // send this put request to the data server
        NodeList::getInstance()->getNodeSocket().writeDatagram((char*) putPacket, numPacketBytes,
                                                               dataServerSockAddr().getAddress(),
                                                               dataServerSockAddr().getPort());
    }
}

void DataServerClient::getValueForKeyAndUUID(const QString& key, const QUuid &uuid) {
    getValuesForKeysAndUUID(QStringList(key), uuid);
}

void DataServerClient::getValuesForKeysAndUUID(const QStringList& keys, const QUuid& uuid) {
    if (!uuid.isNull()) {
        getValuesForKeysAndUserString(keys, uuidStringWithoutCurlyBraces(uuid));
    }
}

void DataServerClient::getValuesForKeysAndUserString(const QStringList& keys, const QString& userString) {
    if (!userString.isEmpty() && keys.size() <= UCHAR_MAX) {
        unsigned char* getPacket = new unsigned char[MAX_PACKET_SIZE];
        
        // setup the header for this packet
        int numPacketBytes = populateTypeAndVersion(getPacket, PACKET_TYPE_DATA_SERVER_GET);
        
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
        
        // add the getPacket to our vector of uncofirmed packets, will be deleted once we get a response from the nameserver
        // _unmatchedPackets.insert(std::pair<unsigned char*, int>(getPacket, numPacketBytes));
        
        // send the get to the data server
        NodeList::getInstance()->getNodeSocket().writeDatagram((char*) getPacket, numPacketBytes,
                                                               dataServerSockAddr().getAddress(),
                                                               dataServerSockAddr().getPort());
    }
}

void DataServerClient::getClientValueForKey(const QString& key) {
    getValuesForKeysAndUserString(QStringList(key), Application::getInstance()->getProfile()->getUserString());
}

void DataServerClient::processConfirmFromDataServer(unsigned char* packetData, int numPacketBytes) {
    removeMatchedPacketFromMap(packetData, numPacketBytes);
}

void DataServerClient::processSendFromDataServer(unsigned char* packetData, int numPacketBytes) {
    // pull the user string from the packet so we know who to associate this with
    int numHeaderBytes = numBytesForPacketHeader(packetData);
    
    char* userStringPosition = (char*) packetData + numHeaderBytes;
    
    QString userString(QByteArray(userStringPosition, strlen(userStringPosition)));
    
    QUuid userUUID(userString);
    
    char* keysPosition = (char*) packetData + numHeaderBytes + strlen(userStringPosition)
        + sizeof('\0') + sizeof(unsigned char);
    char* valuesPosition =  keysPosition + strlen(keysPosition) + sizeof('\0');
    
    QStringList keyList = QString(keysPosition).split(MULTI_KEY_VALUE_SEPARATOR);
    QStringList valueList = QString(valuesPosition).split(MULTI_KEY_VALUE_SEPARATOR);
    
    // user string was UUID, find matching avatar and associate data
    for (int i = 0; i < keyList.size(); i++) {
        if (valueList[i] != " ") {
            if (keyList[i] == DataServerKey::FaceMeshURL) {
                
                if (userUUID.isNull() || userUUID == Application::getInstance()->getProfile()->getUUID()) {
                    qDebug("Changing user's face model URL to %s\n", valueList[i].toLocal8Bit().constData());
                    Application::getInstance()->getProfile()->setFaceModelURL(QUrl(valueList[i]));
                } else {
                    // mesh URL for a UUID, find avatar in our list
                    NodeList* nodeList = NodeList::getInstance();
                    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                        if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
                            Avatar* avatar = (Avatar *) node->getLinkedData();
                            
                            if (avatar->getUUID() == userUUID) {
                                QMetaObject::invokeMethod(&avatar->getHead().getFaceModel(),
                                    "setURL", Q_ARG(QUrl, QUrl(valueList[i])));
                            }
                        }
                    }
                }
            } else if (keyList[i] == DataServerKey::SkeletonURL) {
                
                if (userUUID.isNull() || userUUID == Application::getInstance()->getProfile()->getUUID()) {
                    qDebug("Changing user's skeleton URL to %s\n", valueList[i].toLocal8Bit().constData());
                    Application::getInstance()->getProfile()->setSkeletonModelURL(QUrl(valueList[i]));
                } else {
                    // skeleton URL for a UUID, find avatar in our list
                    NodeList* nodeList = NodeList::getInstance();
                    for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                        if (node->getLinkedData() != NULL && node->getType() == NODE_TYPE_AGENT) {
                            Avatar* avatar = (Avatar *) node->getLinkedData();
                            
                            if (avatar->getUUID() == userUUID) {
                                QMetaObject::invokeMethod(&avatar->getSkeletonModel(), "setURL",
                                    Q_ARG(QUrl, QUrl(valueList[i])));
                            }
                        }
                    }
                }
            } else if (keyList[i] == DataServerKey::Domain && keyList[i + 1] == DataServerKey::Position
                       && valueList[i] != " " && valueList[i + 1] != " ") {
                
                QStringList coordinateItems = valueList[i + 1].split(',');
                
                if (coordinateItems.size() == 3) {
                    
                    // send a node kill request, indicating to other clients that they should play the "disappeared" effect
                    NodeList::getInstance()->sendKillNode(&NODE_TYPE_AVATAR_MIXER, 1);
                    
                    qDebug() << "Changing domain to" << valueList[i].toLocal8Bit().constData() <<
                        "and position to" << valueList[i + 1].toLocal8Bit().constData() <<
                        "to go to" << userString << "\n";
                    
                    NodeList::getInstance()->setDomainHostname(valueList[i]);
                    
                    glm::vec3 newPosition(coordinateItems[0].toFloat(),
                                          coordinateItems[1].toFloat(),
                                          coordinateItems[2].toFloat());
                    Application::getInstance()->getAvatar()->setPosition(newPosition);
                }
                
            } else if (keyList[i] == DataServerKey::UUID) {
                // this is the user's UUID - set it on the profile
                Application::getInstance()->getProfile()->setUUID(valueList[i]);
            }
        }
    }
    
    // remove the matched packet from  our map so it isn't re-sent to the data-server
    // removeMatchedPacketFromMap(packetData, numPacketBytes);
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
        NodeList::getInstance()->getNodeSocket().writeDatagram((char*) mapIterator->first, mapIterator->second,
                                                               dataServerSockAddr().getAddress(),
                                                               dataServerSockAddr().getPort());
    }
}
