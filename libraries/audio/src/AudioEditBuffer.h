//
//  AudioEditBuffer.h
//  hifi
//
//  Created by Craig Hansen-Sturm on 8/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioEditBuffer_h
#define hifi_AudioEditBuffer_h

template< typename T >
class AudioEditBuffer : public AudioFrameBuffer<T> {
    
public:
    
    AudioEditBuffer();    
    AudioEditBuffer(const uint32_t channelCount, const uint32_t frameCount);    
    ~AudioEditBuffer();
    
    bool getZeroCrossing(uint32_t start, bool direction, float32_t epsilon, uint32_t& zero);
        
    void linearFade(uint32_t start, uint32_t stop, bool slope);
    void exponentialFade(uint32_t start, uint32_t stop, bool slope);
};

template< typename T >
AudioEditBuffer<T>::AudioEditBuffer() : 
    AudioFrameBuffer<T>() {
}

template< typename T >
AudioEditBuffer<T>::AudioEditBuffer(const uint32_t channelCount, const uint32_t frameCount) : 
    AudioFrameBuffer<T>(channelCount, frameCount) {
}

template< typename T >
    AudioEditBuffer<T>::~AudioEditBuffer() {
}

template< typename T >
inline bool AudioEditBuffer<T>::getZeroCrossing(uint32_t start, bool direction, float32_t epsilon, uint32_t& zero) {
    
    zero = this->_frameCount;
    
    if (direction) { // scan from the left
        if (start < this->_frameCount) {
            for (uint32_t i = start; i < this->_frameCount; ++i) {
                for (uint32_t j = 0; j < this->_channelCount; ++j) {
                    if (this->_frameBuffer[j][i] >= -epsilon && this->_frameBuffer[j][i] <= epsilon) {
                        zero = i;
                        return true;
                    }
                }
            }
        }
    } else { // scan from the right
        if (start != 0 && start < this->_frameCount) {
            for (uint32_t i = start; i != 0; --i) {
                for (uint32_t j = 0; j < this->_channelCount; ++j) {
                    if (this->_frameBuffer[j][i] >= -epsilon && this->_frameBuffer[j][i] <= epsilon) {
                        zero = i;  
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

template< typename T >
inline void AudioEditBuffer<T>::linearFade(uint32_t start, uint32_t stop, bool slope) {
    
    if (start >= stop || start > this->_frameCount || stop > this->_frameCount ) {
        return;
    }

    uint32_t count = stop - start;
    float32_t delta;
    float32_t gain;
    
    if (slope) { // 0.0 to 1.0f in delta increments
        delta = 1.0f / (float32_t)count;
        gain = 0.0f;
    } else { // 1.0f to 0.0f in delta increments
        delta = -1.0f / (float32_t)count;
        gain = 1.0f;
    }
    
    for (uint32_t i = start; i < stop; ++i) {
        for (uint32_t j = 0; j < this->_channelCount; ++j) {
            this->_frameBuffer[j][i] *= gain;
            gain += delta;
        }
    }
}

template< typename T >
inline void AudioEditBuffer<T>::exponentialFade(uint32_t start, uint32_t stop, bool slope) {
    // TBD
}

typedef AudioEditBuffer< float32_t > AudioEditBufferFloat32;
typedef AudioEditBuffer< int32_t > AudioEditBufferSInt32;

#endif // hifi_AudioEditBuffer_h

