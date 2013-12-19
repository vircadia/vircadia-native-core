//
//  AudioInjector.cpp
//  hifi
//
//  Created by Stephen Birarda on 12/19/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <sys/time.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AbstractAudioInterface.h"
#include "AudioRingBuffer.h"

#include "AudioInjector.h"

int abstractAudioPointerMeta = qRegisterMetaType<AbstractAudioInterface*>("AbstractAudioInterface*");

AudioInjector::AudioInjector(const QUrl& sampleURL) :
    _currentSendPosition(0),
    _sourceURL(sampleURL),
	_position(0,0,0),
    _orientation()
{
    // we want to live on our own thread
    moveToThread(&_thread);
    connect(&_thread, SIGNAL(started()), this, SLOT(startDownload()));
    _thread.start();
}

void AudioInjector::startDownload() {
    // assume we have a QApplication or QCoreApplication instance and use the
    // QNetworkAccess manager to grab the raw audio file at the given URL
    
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    
    manager->get(QNetworkRequest(_sourceURL));
}

void AudioInjector::replyFinished(QNetworkReply* reply) {
    // replace our samples array with the downloaded data
    _sampleByteArray = reply->readAll();
}

void AudioInjector::injectViaThread(AbstractAudioInterface* localAudioInterface) {
    // use Qt::AutoConnection so that this is called on our thread, if appropriate
    QMetaObject::invokeMethod(this, "injectAudio", Qt::AutoConnection, Q_ARG(AbstractAudioInterface*, localAudioInterface));
}

void AudioInjector::injectAudio(AbstractAudioInterface* localAudioInterface) {
    
    // make sure we actually have samples downloaded to inject
    if (_sampleByteArray.size()) {
        // give our sample byte array to the local audio interface, if we have it, so it can be handled locally
        if (localAudioInterface) {
            // assume that localAudioInterface could be on a separate thread, use Qt::AutoConnection to handle properly
            QMetaObject::invokeMethod(localAudioInterface, "handleAudioByteArray",
                                      Qt::AutoConnection,
                                      Q_ARG(QByteArray, _sampleByteArray));
            
        }
        
        NodeList* nodeList = NodeList::getInstance();
        
        // reset the current send position to the beginning
        _currentSendPosition = 0;
        
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
        
        // pack the position for injected audio
        memcpy(currentPacketPosition, &_position, sizeof(_position));
        currentPacketPosition += sizeof(_position);
        
        // pack a zero orientation for injected audio
        memcpy(currentPacketPosition, &_orientation, sizeof(_orientation));
        currentPacketPosition += sizeof(_orientation);
        
        // pack zero for radius
        float radius = 0;
        memcpy(currentPacketPosition, &radius, sizeof(radius));
        currentPacketPosition += sizeof(radius);
        
        // pack 255 for attenuation byte
        uchar volume = 1;
        memcpy(currentPacketPosition, &volume, sizeof(volume));
        currentPacketPosition += sizeof(volume);
        
        timeval startTime = {};
        gettimeofday(&startTime, NULL);
        int nextFrame = 0;
        
        // loop to send off our audio in NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL byte chunks
        while (_currentSendPosition < _sampleByteArray.size()) {
            
            int bytesToCopy = std::min(NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL,
                                       _sampleByteArray.size() - _currentSendPosition);
            
            // copy the next NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL bytes to the packet
            memcpy(currentPacketPosition, _sampleByteArray.data() + _currentSendPosition,
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
            
            _currentSendPosition += bytesToCopy;
            
            // send two packets before the first sleep so the mixer can start playback right away
            
            if (_currentSendPosition != bytesToCopy && _currentSendPosition < _sampleByteArray.size()) {
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