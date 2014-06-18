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
#include <PositionalAudioRingBuffer.h>

#include "AvatarAudioRingBuffer.h"

class AudioMixerClientData : public NodeData {
public:
    AudioMixerClientData();
    ~AudioMixerClientData();
    
    const QList<PositionalAudioRingBuffer*> getRingBuffers() const { return _ringBuffers; }
    AvatarAudioRingBuffer* getAvatarAudioRingBuffer() const;
    
    int parseData(const QByteArray& packet);
    void checkBuffersBeforeFrameSend(int jitterBufferLengthSamples,
                                     AABox* checkSourceZone = NULL, AABox* listenerZone = NULL);
    void pushBuffersAfterFrameSend();
    
    AABox* getListenerUnattenuatedZone() const { return _listenerUnattenuatedZone; }
    void setListenerUnattenuatedZone(AABox* listenerUnattenuatedZone) { _listenerUnattenuatedZone = listenerUnattenuatedZone; }
private:
    QList<PositionalAudioRingBuffer*> _ringBuffers;
    AABox* _listenerUnattenuatedZone;
};

#endif // hifi_AudioMixerClientData_h
