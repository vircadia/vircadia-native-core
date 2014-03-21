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

const int SAMPLE_PHASE_DELAY_AT_90 = 20;

/// Handles assignments of type AudioMixer - mixing streams of audio and re-distributing to various clients.
class AudioMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    AudioMixer(const QByteArray& packet);
public slots:
    /// threaded run of assignment
    void run();
    
    void readPendingDatagrams();
private:
    /// adds one buffer to the mix for a listening node
    void addBufferToMixForListeningNodeWithBuffer(PositionalAudioRingBuffer* bufferToAdd,
                                                  AvatarAudioRingBuffer* listeningNodeBuffer);
    
    /// prepares and sends a mix to one Node
    void prepareMixForListeningNode(Node* node);
    
    // client samples capacity is larger than what will be sent to optimize mixing
    // we are MMX adding 4 samples at a time so we need client samples to have an extra 4
    int16_t _clientSamples[NETWORK_BUFFER_LENGTH_SAMPLES_STEREO + (SAMPLE_PHASE_DELAY_AT_90 * 2)];
    
    float _trailingSleepRatio;
    float _minAudabilityThreshold;
};

#endif /* defined(__hifi__AudioMixer__) */
