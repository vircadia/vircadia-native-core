//
//  AudioMixer.h
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioMixer__
#define __hifi__AudioMixer__

#include <AudioRingBuffer.h>

#include <ThreadedAssignment.h>

class PositionalAudioRingBuffer;
class AvatarAudioRingBuffer;

/// Handles assignments of type AudioMixer - mixing streams of audio and re-distributing to various clients.
class AudioMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    AudioMixer(const QByteArray& packet);
public slots:
    /// threaded run of assignment
    void run();
    
    void readPendingDatagrams();
private slots:
    void receivedSessionUUID(const QUuid& sessionUUID);
private:
    /// adds one buffer to the mix for a listening node
    void addBufferToMixForListeningNodeWithBuffer(PositionalAudioRingBuffer* bufferToAdd,
                                                  AvatarAudioRingBuffer* listeningNodeBuffer);
    
    /// prepares and sends a mix to one Node
    void prepareMixForListeningNode(Node* node);
    
    QByteArray _clientMixBuffer;
    
    int16_t _clientSamples[NETWORK_BUFFER_LENGTH_SAMPLES_STEREO];
};

#endif /* defined(__hifi__AudioMixer__) */
