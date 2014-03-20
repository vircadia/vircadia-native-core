//
//  AudioMixerClientData.h
//  hifi
//
//  Created by Stephen Birarda on 10/18/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
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
    
    float getNextOutputLoudness() const { return _nextOutputLoudness; }
    
    int parseData(const QByteArray& packet);
    void checkBuffersBeforeFrameSend(int jitterBufferLengthSamples, float& currentMinLoudness, float& currentMaxLoudness);
    void pushBuffersAfterFrameSend();
private:
    std::vector<PositionalAudioRingBuffer*> _ringBuffers;
    float _nextOutputLoudness;
};

#endif /* defined(__hifi__AudioMixerClientData__) */
