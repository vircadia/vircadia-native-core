//
//  DataServer.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/20/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QUuid>

#include <PacketHeaders.h>
#include <HifiSockAddr.h>
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
    QByteArray receivedPacket;
    HifiSockAddr senderSockAddr;
    
    while (_socket.hasPendingDatagrams()) {
        receivedPacket.resize(_socket.pendingDatagramSize());
        _socket.readDatagram(receivedPacket.data(), _socket.pendingDatagramSize());
        
        PacketType requestType = packetTypeForPacket(receivedPacket);
        
        if ((requestType == PacketTypeDataServerPut || requestType == PacketTypeDataServerGet) &&
            packetVersionMatch(receivedPacket)) {
            
            QDataStream packetStream(receivedPacket);
            int numReceivedHeaderBytes = numBytesForPacketHeader(receivedPacket);
            packetStream.skipRawData(numReceivedHeaderBytes);
            
            // pull the sequence number used for this packet
            quint8 sequenceNumber = 0;
            
            packetStream >> sequenceNumber;
            
            // pull the UUID that we will need as part of the key
            QString userString;
            packetStream >> userString;
            QUuid parsedUUID(userString);
            
            if (parsedUUID.isNull()) {
                // we failed to parse a UUID, this means the user has sent us a username
                
                // ask redis for the UUID for this user
                redisReply* reply = (redisReply*) redisCommand(_redis, "GET user:%s", qPrintable(userString));
                
                if (reply->type == REDIS_REPLY_STRING) {
                    parsedUUID = QUuid(QString(reply->str));
                }
                
                if (!parsedUUID.isNull()) {
                    qDebug() << "Found UUID" << parsedUUID << "for username" << userString;
                } else {
                    qDebug() << "Failed UUID lookup for username" << userString;
                }
                
                freeReplyObject(reply);
                reply = NULL;
            }
            
            if (!parsedUUID.isNull()) {
                if (requestType == PacketTypeDataServerPut) {
                    
                    // pull the key and value that specifies the data the user is putting/getting
                    QString dataKey, dataValue;
                    
                    packetStream >> dataKey >> dataValue;
                    
                    qDebug("Sending command to redis: SET uuid:%s:%s %s",
                           qPrintable(uuidStringWithoutCurlyBraces(parsedUUID)),
                           qPrintable(dataKey), qPrintable(dataValue));
                    
                    redisReply* reply = (redisReply*) redisCommand(_redis, "SET uuid:%s:%s %s",
                                                                   qPrintable(uuidStringWithoutCurlyBraces(parsedUUID)),
                                                                   qPrintable(dataKey), qPrintable(dataValue));
                    
                    if (reply->type == REDIS_REPLY_STATUS && strcmp("OK", reply->str) == 0) {
                        // if redis stored the value successfully reply back with a confirm
                        // which is a reply packet with the sequence number
                        QByteArray replyPacket = byteArrayWithPopluatedHeader(PacketTypeDataServerConfirm);
                        
                        replyPacket.append(sequenceNumber);
                        
                        _socket.writeDatagram(replyPacket,
                                              senderSockAddr.getAddress(), senderSockAddr.getPort());
                    }
                    
                    freeReplyObject(reply);
                } else {
                    // setup a send packet with the returned data
                    // leverage the packetData sent by overwriting and appending
                    QByteArray sendPacket = byteArrayWithPopluatedHeader(PacketTypeDataServerSend);
                    
                    if (!receivedPacket.mid(numReceivedHeaderBytes).startsWith("uuid")) {
                        
                        const char MULTI_KEY_VALUE_SEPARATOR = '|';
                        
                        // pull the key that specifies the data the user is putting/getting, null terminate it
                        QString keyListString(receivedPacket.mid(numReceivedHeaderBytes));
                        QStringList keyList = keyListString.split(MULTI_KEY_VALUE_SEPARATOR);
                        
                        foreach (const QString& dataKey, keyList) {
                            qDebug("Sending command to redis: GET uuid:%s:%s",
                                   qPrintable(uuidStringWithoutCurlyBraces(parsedUUID)),
                                   qPrintable(dataKey));
                            redisReply* reply = (redisReply*) redisCommand(_redis, "GET uuid:%s:%s",
                                                                           qPrintable(uuidStringWithoutCurlyBraces(parsedUUID)),
                                                                           qPrintable(dataKey));
                            
                            if (reply->len) {
                                // copy the value that redis returned
                                sendPacket.append(reply->str, reply->len);
                                
                            } else {
                                // didn't find a value - insert a space
                                sendPacket.append(' ');
                            }
                            
                            // add the multi-value separator
                            sendPacket.append(MULTI_KEY_VALUE_SEPARATOR);
                            
                            freeReplyObject(reply);
                        }
                        
                        // null terminate the packet we're sending back (erases the trailing separator)
                        sendPacket[sendPacket.size() - 1] = '\0';
                    } else {
                        // user is asking for a UUID matching username, copy the UUID we found
                        sendPacket.append(uuidStringWithoutCurlyBraces(parsedUUID));
                        sendPacket.append('\0');
                    }
                    
                    // reply back with the send packet
                    _socket.writeDatagram(sendPacket, senderSockAddr.getAddress(), senderSockAddr.getPort());
                }
            }
        }
    }
}