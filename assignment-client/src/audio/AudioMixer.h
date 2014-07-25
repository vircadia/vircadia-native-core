//
//  AudioMixer.h
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioMixer_h
#define hifi_AudioMixer_h

#include <AABox.h>
#include <AudioRingBuffer.h>
#include <ThreadedAssignment.h>

class PositionalAudioRingBuffer;
class AvatarAudioRingBuffer;

const int SAMPLE_PHASE_DELAY_AT_90 = 20;

const quint64 TOO_LONG_SINCE_LAST_SEND_AUDIO_STREAM_STATS = 1 * USECS_PER_SECOND;

/// Handles assignments of type AudioMixer - mixing streams of audio and re-distributing to various clients.
class AudioMixer : public ThreadedAssignment {
    Q_OBJECT
public:
    AudioMixer(const QByteArray& packet);
    ~AudioMixer();
public slots:
    /// threaded run of assignment
    void run();
    
    void readPendingDatagrams();
    
    void sendStatsPacket();

    static bool getUseDynamicJitterBuffers() { return _useDynamicJitterBuffers; }

private:
    /// adds one buffer to the mix for a listening node
    void addBufferToMixForListeningNodeWithBuffer(PositionalAudioRingBuffer* bufferToAdd,
                                                  AvatarAudioRingBuffer* listeningNodeBuffer,
                                                  bool bufferToAddBelongsToListener);
    
    /// prepares and sends a mix to one Node
    void prepareMixForListeningNode(Node* node);
    
    // client samples capacity is larger than what will be sent to optimize mixing
    // we are MMX adding 4 samples at a time so we need client samples to have an extra 4
    int16_t _clientSamples[NETWORK_BUFFER_LENGTH_SAMPLES_STEREO + (SAMPLE_PHASE_DELAY_AT_90 * 2)];
    
    float _trailingSleepRatio;
    float _minAudibilityThreshold;
    float _performanceThrottlingRatio;
    int _numStatFrames;
    int _sumListeners;
    int _sumMixes;
    AABox* _sourceUnattenuatedZone;
    AABox* _listenerUnattenuatedZone;
    static bool _useDynamicJitterBuffers;

    quint64 _lastSendAudioStreamStatsTime;
};

#endif // hifi_AudioMixer_h
