//
//  UDTTest.h
//  tools/udt-test/src
//
//  Created by Stephen Birarda on 2015-07-30.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_UDTTest_h
#define hifi_UDTTest_h


#include <random>

#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>

#include <udt/Constants.h>
#include <udt/Socket.h>

class UDTTest : public QCoreApplication {
    Q_OBJECT
public:
    UDTTest(int& argc, char** argv);

public slots:
    void refillPacket() { sendPacket(); } // adds a new packet to the queue when we are told one is sent
    void sampleStats();
    
private:
    void parseArguments();
    void handlePacketList(std::unique_ptr<udt::PacketList> packetList);
    
    void sendInitialPackets(); // fills the queue with packets to start
    void sendPacket(); // constructs and sends a packet according to the test parameters
    
    QCommandLineParser _argumentParser;
    udt::Socket _socket;
    
    HifiSockAddr _target; // the target for sent packets
    
    int _minPacketSize { udt::MAX_PACKET_SIZE };
    int _maxPacketSize { udt::MAX_PACKET_SIZE };
    int _maxSendBytes { -1 }; // the number of bytes to send to the target before stopping
    int _maxSendPackets { -1 }; // the number of packets to send to the target before stopping
    
    bool _sendReliable { true }; // whether packets are sent reliably or unreliably
    bool _sendOrdered { false }; // whether to send ordered packets
    
    int _messageSize { 10000000 }; // number of bytes per message while sending ordered
    
    std::random_device _randomDevice;
    std::mt19937 _generator { _randomDevice() }; // random number generator for ordered data testing
    std::uniform_int_distribution<uint64_t> _distribution { 1, UINT64_MAX }; // producer of random integer values
    
    int _totalQueuedPackets { 0 }; // keeps track of the number of packets we have already queued
    int _totalQueuedBytes { 0 }; // keeps track of the number of bytes we have already queued
    
    int _statsInterval { 100 }; // recording interval for stats in milliseconds
};

#endif // hifi_UDTTest_h
