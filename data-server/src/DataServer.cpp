//
//  DataServer.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/20/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QUuid>

#include <PacketHeaders.h>
#include <UUID.h>

#include "DataServer.h"

const quint16 DATA_SERVER_LISTEN_PORT = 3282;

const char REDIS_HOSTNAME[] = "127.0.0.1";
const unsigned short REDIS_PORT = 6379;

DataServer::DataServer(int argc, char* argv[]) :
    QCoreApplication(argc, argv),
    _socket(),
    _redis(NULL)
{
    _socket.bind(QHostAddress::Any, DATA_SERVER_LISTEN_PORT);
    
    connect(&_socket, &QUdpSocket::readyRead, this, &DataServer::readPendingDatagrams);
    
    _redis = redisConnect(REDIS_HOSTNAME, REDIS_PORT);
    
    if (_redis && _redis->err) {
        if (_redis) {
            qDebug() << "Redis connection error:" << _redis->errstr;
        } else {
            qDebug("Redis connection error - can't allocate redis context.");
        }
        
        // couldn't connect to redis, bail out
        QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
    }
}

DataServer::~DataServer() {
    // disconnect from redis and free the context
    if (_redis) {
        redisFree(_redis);
    }
}

const int MAX_PACKET_SIZE = 1500;

void DataServer::readPendingDatagrams() {
    qint64 receivedBytes = 0;
    static unsigned char packetData[MAX_PACKET_SIZE];

    QHostAddress senderAddress;
    quint16 senderPort;
    
    while (_socket.hasPendingDatagrams() &&
           (receivedBytes = _socket.readDatagram(reinterpret_cast<char*>(packetData), MAX_PACKET_SIZE,
                                                 &senderAddress, &senderPort))) {
        if ((packetData[0] == PACKET_TYPE_DATA_SERVER_PUT || packetData[0] == PACKET_TYPE_DATA_SERVER_GET) &&
            packetVersionMatch(packetData)) {
                
            int readBytes = numBytesForPacketHeader(packetData);
            
            // pull the sequence number used for this packet
            quint8 sequenceNumber = 0;
            memcpy(&sequenceNumber, packetData + readBytes, sizeof(sequenceNumber));
            readBytes += sizeof(sequenceNumber);
                
            // pull the UUID that we will need as part of the key
            QString uuidString(reinterpret_cast<char*>(packetData + readBytes));
            QUuid parsedUUID(uuidString);
            
            if (parsedUUID.isNull()) {
                // we failed to parse a UUID, this means the user has sent us a username
                
                QString username(reinterpret_cast<char*>(packetData + readBytes));
                readBytes += username.size() + sizeof('\0');
            
                // ask redis for the UUID for this user
                redisReply* reply = (redisReply*) redisCommand(_redis, "GET user:%s", qPrintable(username));
                
                if (reply->type == REDIS_REPLY_STRING) {
                    parsedUUID = QUuid(QString(reply->str));
                }
                
                if (!parsedUUID.isNull()) {
                    qDebug() << "Found UUID" << parsedUUID << "for username" << username;
                } else {
                    qDebug() << "Failed UUID lookup for username" << username;
                }
                
                freeReplyObject(reply);
                reply = NULL;
            } else {
                readBytes += uuidString.size() + sizeof('\0');
            }
            
            if (!parsedUUID.isNull()) {
                // pull the number of keys the user has sent
                unsigned char numKeys = packetData[readBytes++];
                
                if (packetData[0] == PACKET_TYPE_DATA_SERVER_PUT) {
                    
                    // pull the key that specifies the data the user is putting/getting
                    QString dataKey(reinterpret_cast<char*>(packetData + readBytes));
                    readBytes += dataKey.size() + sizeof('\0');
                    
                    // grab the string value the user wants us to put, null terminate it
                    QString dataValue(reinterpret_cast<char*>(packetData + readBytes));
                    readBytes += dataValue.size() + sizeof('\0');
                    
                    qDebug("Sending command to redis: SET uuid:%s:%s %s",
                           qPrintable(uuidStringWithoutCurlyBraces(parsedUUID)),
                           qPrintable(dataKey), qPrintable(dataValue));
                    
                    redisReply* reply = (redisReply*) redisCommand(_redis, "SET uuid:%s:%s %s",
                                                                   qPrintable(uuidStringWithoutCurlyBraces(parsedUUID)),
                                                                   qPrintable(dataKey), qPrintable(dataValue));
                    
                    if (reply->type == REDIS_REPLY_STATUS && strcmp("OK", reply->str) == 0) {
                        // if redis stored the value successfully reply back with a confirm
                        // which is the sent packet with the header replaced
                        packetData[0] = PACKET_TYPE_DATA_SERVER_CONFIRM;
                        _socket.writeDatagram(reinterpret_cast<char*>(packetData), receivedBytes,
                                              senderAddress, senderPort);
                    }
                    
                    freeReplyObject(reply);
                } else {
                    // setup a send packet with the returned data
                    // leverage the packetData sent by overwriting and appending
                    int numSendPacketBytes = receivedBytes;
                    
                    packetData[0] = PACKET_TYPE_DATA_SERVER_SEND;
                    
                    if (strcmp((char*) packetData + readBytes, "uuid") != 0) {
                        
                        const char MULTI_KEY_VALUE_SEPARATOR = '|';
                        
                        // the user has sent one or more keys - make the associated requests
                        for (int j = 0; j < numKeys; j++) {
                            
                            // pull the key that specifies the data the user is putting/getting, null terminate it
                            int numDataKeyBytes = 0;
                            
                            // look for the key separator or the null terminator
                            while (packetData[readBytes + numDataKeyBytes] != MULTI_KEY_VALUE_SEPARATOR
                                   && packetData[readBytes + numDataKeyBytes] != '\0') {
                                numDataKeyBytes++;
                            }
                            
                            QString dataKey(QByteArray(reinterpret_cast<char*>(packetData + readBytes), numDataKeyBytes));
                            readBytes += dataKey.size() + sizeof('\0');
                            
                            qDebug("Sending command to redis: GET uuid:%s:%s",
                                   qPrintable(uuidStringWithoutCurlyBraces(parsedUUID)),
                                   qPrintable(dataKey));
                            redisReply* reply = (redisReply*) redisCommand(_redis, "GET uuid:%s:%s",
                                                               qPrintable(uuidStringWithoutCurlyBraces(parsedUUID)),
                                                               qPrintable(dataKey));
                            
                            if (reply->len) {
                                // copy the value that redis returned
                                memcpy(packetData + numSendPacketBytes, reply->str, reply->len);
                                numSendPacketBytes += reply->len;
                                
                            } else {
                                // didn't find a value - insert a space
                                packetData[numSendPacketBytes++] = ' ';
                            }
                            
                            // add the multi-value separator
                            packetData[numSendPacketBytes++] = MULTI_KEY_VALUE_SEPARATOR;
                            
                            freeReplyObject(reply);
                        }
                        
                        // null terminate the packet we're sending back (erases the trailing separator)
                        packetData[(numSendPacketBytes - 1)] = '\0';
                    } else {
                        // user is asking for a UUID matching username, copy the UUID we found
                        QString uuidString = uuidStringWithoutCurlyBraces(parsedUUID);
                        memcpy(packetData + numSendPacketBytes, qPrintable(uuidString), uuidString.size() + sizeof('\0'));
                        numSendPacketBytes += uuidString.size() + sizeof('\0');
                    }
                    
                    // reply back with the send packet
                    _socket.writeDatagram(reinterpret_cast<char*>(packetData), numSendPacketBytes,
                                          senderAddress, senderPort);
                    
                    
                }
            }
        }
    }
}