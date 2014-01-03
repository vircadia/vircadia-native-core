//
//  AudioInjector.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AbstractAudioInterface.h"
#include "AudioRingBuffer.h"

#include "AudioInjector.h"

int abstractAudioPointerMeta = qRegisterMetaType<AbstractAudioInterface*>("AbstractAudioInterface*");

AudioInjector::AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions) :
    _sound(sound),
    _options(injectorOptions)
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
        bool loopbackFlag = (_options.getLoopbackAudioInterface() == NULL);
        memcpy(currentPacketPosition, &loopbackFlag, sizeof(loopbackFlag));
        currentPacketPosition += sizeof(loopbackFlag);
        
        // pack the position for injected audio
        memcpy(currentPacketPosition, &_options.getPosition(), sizeof(_options.getPosition()));
        currentPacketPosition += sizeof(_options.getPosition());
        
        // pack our orientation for injected audio
        memcpy(currentPacketPosition, &_options.getOrientation(), sizeof(_options.getOrientation()));
        currentPacketPosition += sizeof(_options.getOrientation());
        
        // pack zero for radius
        float radius = 0;
        memcpy(currentPacketPosition, &radius, sizeof(radius));
        currentPacketPosition += sizeof(radius);
        
        // pack 255 for attenuation byte
        uchar volume = MAX_INJECTOR_VOLUME * _options.getVolume();
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
    
    emit finished();
}