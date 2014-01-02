//
//  AudioInjector.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AbstractAudioInterface.h"
#include "AudioRingBuffer.h"

#include "AudioInjector.h"

int abstractAudioPointerMeta = qRegisterMetaType<AbstractAudioInterface*>("AbstractAudioInterface*");

AudioInjector::AudioInjector(Sound* sound) :
    _sound(sound),
    _volume(1.0f),
    _shouldLoopback(true),
    _position(0.0f, 0.0f, 0.0f),
    _orientation()
{
    // we want to live on our own thread
    moveToThread(&_thread);
    connect(&_thread, SIGNAL(started()), this, SLOT(startDownload()));
    _thread.start();
}

void AudioInjector::injectViaThread(AbstractAudioInterface* localAudioInterface) {
    // use Qt::AutoConnection so that this is called on our thread, if appropriate
    QMetaObject::invokeMethod(this, "injectAudio", Qt::AutoConnection, Q_ARG(AbstractAudioInterface*, localAudioInterface));
}

const uchar MAX_INJECTOR_VOLUME = 0xFF;

void AudioInjector::injectAudio(AbstractAudioInterface* localAudioInterface) {
    
    QByteArray soundByteArray = _sound->getByteArray();
    
    // make sure we actually have samples downloaded to inject
    if (soundByteArray.size()) {
        // give our sample byte array to the local audio interface, if we have it, so it can be handled locally
        if (localAudioInterface) {
            // assume that localAudioInterface could be on a separate thread, use Qt::AutoConnection to handle properly
            QMetaObject::invokeMethod(localAudioInterface, "handleAudioByteArray",
                                      Qt::AutoConnection,
                                      Q_ARG(QByteArray, soundByteArray));
            
        }
        
        NodeList* nodeList = NodeList::getInstance();
        
        // setup the packet for injected audio
        unsigned char injectedAudioPacket[MAX_PACKET_SIZE];
        unsigned char* currentPacketPosition = injectedAudioPacket;
        
        int numBytesPacketHeader = populateTypeAndVersion(injectedAudioPacket, PACKET_TYPE_INJECT_AUDIO);
        currentPacketPosition += numBytesPacketHeader;
        
        // pack the session UUID for this Node
        QByteArray rfcSessionUUID = NodeList::getInstance()->getOwnerUUID().toRfc4122();
        memcpy(currentPacketPosition, rfcSessionUUID.constData(), rfcSessionUUID.size());
        currentPacketPosition += rfcSessionUUID.size();
        
        // pick a random UUID to use for this stream
        QUuid randomStreamUUID;
        QByteArray rfcStreamUUID = randomStreamUUID.toRfc4122();
        memcpy(currentPacketPosition, rfcStreamUUID, rfcStreamUUID.size());
        currentPacketPosition += rfcStreamUUID.size();
        
        // pack the flag for loopback
        memcpy(currentPacketPosition, &_shouldLoopback, sizeof(_shouldLoopback));
        currentPacketPosition += sizeof(_shouldLoopback);
        
        // pack the position for injected audio
        memcpy(currentPacketPosition, &_position, sizeof(_position));
        currentPacketPosition += sizeof(_position);
        
        // pack our orientation for injected audio
        memcpy(currentPacketPosition, &_orientation, sizeof(_orientation));
        currentPacketPosition += sizeof(_orientation);
        
        // pack zero for radius
        float radius = 0;
        memcpy(currentPacketPosition, &radius, sizeof(radius));
        currentPacketPosition += sizeof(radius);
        
        // pack 255 for attenuation byte
        uchar volume = MAX_INJECTOR_VOLUME * _volume;
        memcpy(currentPacketPosition, &volume, sizeof(volume));
        currentPacketPosition += sizeof(volume);
        
        timeval startTime = {};
        gettimeofday(&startTime, NULL);
        int nextFrame = 0;
        
        int currentSendPosition = 0;
        
        // loop to send off our audio in NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL byte chunks
        while (currentSendPosition < soundByteArray.size()) {
            
            int bytesToCopy = std::min(NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL,
                                       soundByteArray.size() - currentSendPosition);
            
            // copy the next NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL bytes to the packet
            memcpy(currentPacketPosition, soundByteArray.data() + currentSendPosition,
                   bytesToCopy);
            
            
            // grab our audio mixer from the NodeList, if it exists
            Node* audioMixer = nodeList->soloNodeOfType(NODE_TYPE_AUDIO_MIXER);
            
            if (audioMixer && nodeList->getNodeActiveSocketOrPing(audioMixer)) {
                // send off this audio packet
                nodeList->getNodeSocket().writeDatagram((char*) injectedAudioPacket,
                                                        (currentPacketPosition - injectedAudioPacket) + bytesToCopy,
                                                        audioMixer->getActiveSocket()->getAddress(),
                                                        audioMixer->getActiveSocket()->getPort());
            }
            
            currentSendPosition += bytesToCopy;
            
            // send two packets before the first sleep so the mixer can start playback right away
            
            if (currentSendPosition != bytesToCopy && currentSendPosition < soundByteArray.size()) {
                // not the first packet and not done
                // sleep for the appropriate time
                int usecToSleep = usecTimestamp(&startTime) + (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - usecTimestampNow();
                
                if (usecToSleep > 0) {
                    usleep(usecToSleep);
                }
            }
        }
        
    }
}