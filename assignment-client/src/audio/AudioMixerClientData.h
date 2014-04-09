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

#ifndef __hifi__AudioMixerClientData__
#define __hifi__AudioMixerClientData__

#include <vector>

#include <NodeData.h>
#include <PositionalAudioRingBuffer.h>

#include "AvatarAudioRingBuffer.h"

class AudioMixerClientData : public NodeData {
public:
    AudioMixerClientData();
    ~AudioMixerClientData();
    
    const std::vector<PositionalAudioRingBuffer*> getRingBuffers() const { return _ringBuffers; }
    AvatarAudioRingBuffer* getAvatarAudioRingBuffer() const;
    
    int parseData(const QByteArray& packet);
    void checkBuffersBeforeFrameSend(int jitterBufferLengthSamples);
    void pushBuffersAfterFrameSend();
private:
    std::vector<PositionalAudioRingBuffer*> _ringBuffers;
};

#endif /* defined(__hifi__AudioMixerClientData__) */
