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
    _shouldStop(false),
    _currentSendPosition(0),
    _loudness(0.0f)
{
}

AudioInjector::AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions) :
    _sound(sound),
    _options(injectorOptions),
    _shouldStop(false),
    _currentSendPosition(0),
    _loudness(0.0f)
{
}

void AudioInjector::setOptions(AudioInjectorOptions& options) {
    _options = options;
}

float AudioInjector::getLoudness() {
    return _loudness;
}

const uchar MAX_INJECTOR_VOLUME = 0xFF;

void AudioInjector::injectAudio() {
    QByteArray soundByteArray = _sound->getByteArray();
    
    if (_currentSendPosition < 0 ||
        _currentSendPosition >= soundByteArray.size()) {
        _currentSendPosition = 0;
    }
    
    // make sure we actually have samples downloaded to inject
    if (soundByteArray.size()) {
        // give our sample byte array to the local audio interface, if we have it, so it can be handled locally
        if (_options.getLoopbackAudioInterface()) {
            // assume that localAudioInterface could be on a separate thread, use Qt::AutoConnection to handle properly
            QMetaObject::invokeMethod(_options.getLoopbackAudioInterface(), "handleAudioByteArray",
                                      Qt::AutoConnection,
                                      Q_ARG(QByteArray, soundByteArray));
            
        }
        
        // setup the packet for injected audio
        QByteArray injectAudioPacket = byteArrayWithPopulatedHeader(PacketTypeInjectAudio);
        QDataStream packetStream(&injectAudioPacket, QIODevice::Append);
        
        // pack some placeholder sequence number for now
        int numPreSequenceNumberBytes = injectAudioPacket.size();
        packetStream << (quint16)0;
        
        // pack stream identifier (a generated UUID)
        packetStream << QUuid::createUuid();
        
        // pack the stereo/mono type of the stream
        packetStream << _options.isStereo();
        
        // pack the flag for loopback
        uchar loopbackFlag = (uchar) (!_options.getLoopbackAudioInterface());
        packetStream << loopbackFlag;
        
        // pack the position for injected audio
        int positionOptionOffset = injectAudioPacket.size();
        packetStream.writeRawData(reinterpret_cast<const char*>(&_options.getPosition()),
                                  sizeof(_options.getPosition()));
        
        // pack our orientation for injected audio
        int orientationOptionOffset = injectAudioPacket.size();
        packetStream.writeRawData(reinterpret_cast<const char*>(&_options.getOrientation()),
                                  sizeof(_options.getOrientation()));
        
        // pack zero for radius
        float radius = 0;
        packetStream << radius;
        
        // pack 255 for attenuation byte
        int volumeOptionOffset = injectAudioPacket.size();
        quint8 volume = MAX_INJECTOR_VOLUME * _options.getVolume();
        packetStream << volume;
        
        QElapsedTimer timer;
        timer.start();
        int nextFrame = 0;
        
        int numPreAudioDataBytes = injectAudioPacket.size();
        bool shouldLoop = _options.getLoop();
        
        // loop to send off our audio in NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL byte chunks
        quint16 outgoingInjectedAudioSequenceNumber = 0;
        while (_currentSendPosition < soundByteArray.size() && !_shouldStop) {
            
            int bytesToCopy = std::min(((_options.isStereo()) ? 2 : 1) * NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL,
                                       soundByteArray.size() - _currentSendPosition);
            
            //  Measure the loudness of this frame
            _loudness = 0.0f;
            for (int i = 0; i < bytesToCopy; i += sizeof(int16_t)) {
                _loudness += abs(*reinterpret_cast<int16_t*>(soundByteArray.data() + _currentSendPosition + i)) /
                (MAX_SAMPLE_VALUE / 2.0f);
            }
            _loudness /= (float)(bytesToCopy / sizeof(int16_t));

            memcpy(injectAudioPacket.data() + positionOptionOffset,
                   &_options.getPosition(),
                   sizeof(_options.getPosition()));
            memcpy(injectAudioPacket.data() + orientationOptionOffset,
                   &_options.getOrientation(),
                   sizeof(_options.getOrientation()));
            volume = MAX_INJECTOR_VOLUME * _options.getVolume();
            memcpy(injectAudioPacket.data() + volumeOptionOffset, &volume, sizeof(volume));
            
            // resize the QByteArray to the right size
            injectAudioPacket.resize(numPreAudioDataBytes + bytesToCopy);

            // pack the sequence number
            memcpy(injectAudioPacket.data() + numPreSequenceNumberBytes,
                   &outgoingInjectedAudioSequenceNumber, sizeof(quint16));
            
            // copy the next NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL bytes to the packet
            memcpy(injectAudioPacket.data() + numPreAudioDataBytes,
                   soundByteArray.data() + _currentSendPosition, bytesToCopy);
            
            // grab our audio mixer from the NodeList, if it exists
            NodeList* nodeList = NodeList::getInstance();
            SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
            
            // send off this audio packet
            nodeList->writeDatagram(injectAudioPacket, audioMixer);
            outgoingInjectedAudioSequenceNumber++;
            
            _currentSendPosition += bytesToCopy;
            
            // send two packets before the first sleep so the mixer can start playback right away
            
            if (_currentSendPosition != bytesToCopy && _currentSendPosition < soundByteArray.size()) {
                // not the first packet and not done
                // sleep for the appropriate time
                int usecToSleep = (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - timer.nsecsElapsed() / 1000;
                
                if (usecToSleep > 0) {
                    usleep(usecToSleep);
                } 
            }

            if (shouldLoop && _currentSendPosition >= soundByteArray.size()) {
                _currentSendPosition = 0;
            }
        }
    }
    
    emit finished();
}
