//
//  AudioMixerDatagramProcessor.cpp
//  assignment-client/src
//
//  Created by Stephen Birarda on 2014-08-14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>

#include <HifiSockAddr.h>
#include <NodeList.h>

#include "AudioMixerDatagramProcessor.h"

AudioMixerDatagramProcessor::AudioMixerDatagramProcessor(QUdpSocket& nodeSocket, QThread* previousNodeSocketThread) :
    _nodeSocket(nodeSocket),
    _previousNodeSocketThread(previousNodeSocketThread)
{
    
}

AudioMixerDatagramProcessor::~AudioMixerDatagramProcessor() {    
    // return the node socket to its previous thread
    _nodeSocket.moveToThread(_previousNodeSocketThread);
}

void AudioMixerDatagramProcessor::readPendingDatagrams() {
    
    HifiSockAddr senderSockAddr;
    static QByteArray incomingPacket;
    
    // read everything that is available
    while (_nodeSocket.hasPendingDatagrams()) {
        incomingPacket.resize(_nodeSocket.pendingDatagramSize());
        
        // just get this packet off the stack
        _nodeSocket.readDatagram(incomingPacket.data(), incomingPacket.size(),
                                  senderSockAddr.getAddressPointer(), senderSockAddr.getPortPointer());
        
        // emit the signal to tell AudioMixer it needs to process a packet
        emit packetRequiresProcessing(incomingPacket, senderSockAddr);
    }
}
