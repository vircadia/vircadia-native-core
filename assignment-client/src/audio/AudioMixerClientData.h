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
    ~AudioMixerClientData();
    
    int parseData(unsigned char* packetData, int numBytes);
    
    AvatarAudioRingBuffer* getAvatarAudioRingBuffer() const;
private:
    std::vector<PositionalAudioRingBuffer*> _ringBuffers;
};

#endif /* defined(__hifi__AudioMixerClientData__) */
