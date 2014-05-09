//
//  AudioInjector.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AbstractAudioInterface.h"
#include "AudioRingBuffer.h"

#include "AudioInjector.h"

AudioInjector::AudioInjector(QObject* parent) :
    QObject(parent),
    _sound(NULL),
    _options(),
    _shouldStop(false)
{
    
}

AudioInjector::AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions) :
    _sound(sound),
    _options(injectorOptions),
    _shouldStop(false)
{
    
}

const uchar MAX_INJECTOR_VOLUME = 0xFF;

void AudioInjector::injectAudio() {
    
    QByteArray soundByteArray = _sound->getByteArray();
    
    // make sure we actually have samples downloaded to inject
    if (soundByteArray.size()) {
        // give our sample byte array to the local audio interface, if we have it, so it can be handled locally
        if (_options.getLoopbackAudioInterface()) {
            // assume that localAudioInterface could be on a separate thread, use Qt::AutoConnection to handle properly
            QMetaObject::invokeMethod(_options.getLoopbackAudioInterface(), "handleAudioByteArray",
                                      Qt::AutoConnection,
                                      Q_ARG(QByteArray, soundByteArray));
            
        }
        
        NodeList* nodeList = NodeList::getInstance();
        
        // setup the packet for injected audio
        QByteArray injectAudioPacket = byteArrayWithPopulatedHeader(PacketTypeInjectAudio);
        QDataStream packetStream(&injectAudioPacket, QIODevice::Append);
        
        packetStream << QUuid::createUuid();
        
        // pack the flag for loopback
        uchar loopbackFlag = (uchar) (!_options.getLoopbackAudioInterface());
        packetStream << loopbackFlag;
        
        // pack the position for injected audio
        packetStream.writeRawData(reinterpret_cast<const char*>(&_options.getPosition()), sizeof(_options.getPosition()));
        
        // pack our orientation for injected audio
        packetStream.writeRawData(reinterpret_cast<const char*>(&_options.getOrientation()), sizeof(_options.getOrientation()));
        
        // pack zero for radius
        float radius = 0;
        packetStream << radius;
        
        // pack 255 for attenuation byte
        quint8 volume = MAX_INJECTOR_VOLUME * _options.getVolume();
        packetStream << volume;
        
        QElapsedTimer timer;
        timer.start();
        int nextFrame = 0;
        
        int currentSendPosition = 0;
        
        int numPreAudioDataBytes = injectAudioPacket.size();
        
        // loop to send off our audio in NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL byte chunks
        while (currentSendPosition < soundByteArray.size() && !_shouldStop) {
            
            int bytesToCopy = std::min(NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL,
                                       soundByteArray.size() - currentSendPosition);
            
            // resize the QByteArray to the right size
            injectAudioPacket.resize(numPreAudioDataBytes + bytesToCopy);
            
            // copy the next NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL bytes to the packet
            memcpy(injectAudioPacket.data() + numPreAudioDataBytes, soundByteArray.data() + currentSendPosition, bytesToCopy);
            
            // grab our audio mixer from the NodeList, if it exists
            SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
            
            // send off this audio packet
            nodeList->writeDatagram(injectAudioPacket, audioMixer);
            
            currentSendPosition += bytesToCopy;
            
            // send two packets before the first sleep so the mixer can start playback right away
            
            if (currentSendPosition != bytesToCopy && currentSendPosition < soundByteArray.size()) {
                // not the first packet and not done
                // sleep for the appropriate time
                int usecToSleep = (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - timer.nsecsElapsed() / 1000;
                
                if (usecToSleep > 0) {
                    usleep(usecToSleep);
                }
            }
        }
    }
    
    emit finished();
}
