//
//  AudioMixer.h
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioMixer__
#define __hifi__AudioMixer__


#include <Assignment.h>
#include <AudioRingBuffer.h>

class PositionalAudioRingBuffer;
class AvatarAudioRingBuffer;

/// Handles assignments of type AudioMixer - mixing streams of audio and re-distributing to various clients.
class AudioMixer : public Assignment {
    Q_OBJECT
public:
    AudioMixer(const unsigned char* dataBuffer, int numBytes);
public slots:
    /// runs the audio mixer
    void run();
    
    void processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr);
signals:
    void finished();
private:
    /// adds one buffer to the mix for a listening node
    void addBufferToMixForListeningNodeWithBuffer(PositionalAudioRingBuffer* bufferToAdd,
                                                  AvatarAudioRingBuffer* listeningNodeBuffer);
    
    /// prepares and sends a mix to one Node
    void prepareMixForListeningNode(Node* node);
    
    
    int16_t _clientSamples[BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 2];
private slots:
    void checkInWithDomainServerOrExit();
    void sendClientMixes();
};

#endif /* defined(__hifi__AudioMixer__) */
