//
//  AudioMixerClientData.h
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioMixerClientData_h
#define hifi_AudioMixerClientData_h

#include <AABox.h>
#include <NodeData.h>

#include "PositionalAudioRingBuffer.h"
#include "AvatarAudioRingBuffer.h"

class AudioMixerJitterBuffersStats {
public:
    AudioMixerJitterBuffersStats()
        : avatarJitterBufferFrames(0), maxJitterBufferFrames(0), avgJitterBufferFrames(0)
    {}

    int avatarJitterBufferFrames;
    int maxJitterBufferFrames;
    float avgJitterBufferFrames;
};

class AudioMixerClientData : public NodeData {
public:
    AudioMixerClientData();
    ~AudioMixerClientData();
    
    const QList<PositionalAudioRingBuffer*> getRingBuffers() const { return _ringBuffers; }
    AvatarAudioRingBuffer* getAvatarAudioRingBuffer() const;
    
    int parseData(const QByteArray& packet);
    void checkBuffersBeforeFrameSend(AABox* checkSourceZone = NULL, AABox* listenerZone = NULL);
    void pushBuffersAfterFrameSend();

    void calculateJitterBuffersStats(AudioMixerJitterBuffersStats& stats) const;
    QString getJitterBufferStats() const;

private:
    QList<PositionalAudioRingBuffer*> _ringBuffers;
};

#endif // hifi_AudioMixerClientData_h
