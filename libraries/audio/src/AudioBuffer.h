//
//  AudioBuffer.h
//  hifi
//
//  Created by Craig Hansen-Sturm on 8/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioBuffer_h
#define hifi_AudioBuffer_h

#include <typeinfo>

template< typename T >
class AudioFrameBuffer {
    
    uint16_t _channelCount;
    uint16_t _frameCount;
    uint16_t _frameCountMax;
    
    T** _frameBuffer;

    void allocateFrames() {
        _frameBuffer = new T*[_channelCount];
        if (_frameBuffer) {
            for (uint16_t i = 0; i < _channelCount; ++i) {
                _frameBuffer[i] = new T[_frameCountMax];
            }
        }
    }
    
    void deallocateFrames() {
        if (_frameBuffer) {
            for (uint16_t i = 0; i < _channelCount; ++i) {
                delete _frameBuffer[i];
            }
            delete _frameBuffer;
        }
        _frameBuffer = NULL;
    }
    
public:
    
    AudioFrameBuffer() :
        _channelCount(0),
        _frameCount(0),
        _frameCountMax(0),
        _frameBuffer(NULL) {
    }
    
    AudioFrameBuffer(const uint16_t channelCount, const uint16_t frameCount) :
        _channelCount(channelCount),
        _frameCount(frameCount),
        _frameCountMax(frameCount),
        _frameBuffer(NULL) {
        allocateFrames();
    }
    
    ~AudioFrameBuffer() {
        finalize();
    }
    
    void initialize(const uint16_t channelCount, const uint16_t frameCount) {
        if (_frameBuffer) {
            finalize();
        }
        _channelCount = channelCount;
        _frameCount = frameCount;
        _frameCountMax = frameCount;
        allocateFrames();
    }
    
    void finalize() {
        deallocateFrames();
        _channelCount = 0;
        _frameCount = 0;
    }
                
    T**& getFrameData() {
        return _frameBuffer;
    }
    
    uint16_t getChannelCount() {
        return _channelCount;
    }
    
    uint16_t getFrameCount() {
        return _frameCount;
    }
    
    void zeroFrames() {
        if (!_frameBuffer) {
            return;
        }
        for (uint16_t i = 0; i < _channelCount; ++i) {
            memset(_frameBuffer[i], 0, sizeof(T)*_frameCountMax);
        }
    }
     
    template< typename S >
    void copyFrames(uint16_t channelCount, const uint16_t frameCount, S* frames, const bool copyOut = false) {
        if ( !_frameBuffer || !frames) {
            return;
        }
        assert(channelCount == _channelCount);
        assert(frameCount <= _frameCountMax);
        
        _frameCount = frameCount;  // we allow copying fewer frames than we've allocated
        
        if (copyOut) {
            S* dst = frames;
            
            if(typeid(T) == typeid(S)) { // source and destination types are the same
                for (int i = 0; i < _frameCount; ++i) {
                    for (int j = 0; j < _channelCount; ++j) {
                        *dst++ = _frameBuffer[j][i];
                    }
                }
            }
            else {
                if(typeid(T) == typeid(float32_t) &&
                   typeid(S) == typeid(int16_t)) {
                    
                    const int scale = (2 << ((8 * sizeof(S)) - 1));
                    
                    for (int i = 0; i < _frameCount; ++i) {
                        for (int j = 0; j < _channelCount; ++j) {
                            *dst++ = (S)(_frameBuffer[j][i] * scale);
                        }
                    }
                }
                else {
                    assert(0); // currently unsupported conversion
                }
            }
        }
        else { // copyIn
            S* src = frames;
            
            if(typeid(T) == typeid(S)) { // source and destination types are the same
                for (int i = 0; i < _frameCount; ++i) {
                    for (int j = 0; j < _channelCount; ++j) {
                        _frameBuffer[j][i] = *src++;
                    }
                }
            }
            else {
                if(typeid(T) == typeid(float32_t) &&
                   typeid(S) == typeid(int16_t)) {
                    
                    const int scale = (2 << ((8 * sizeof(S)) - 1));
                    
                    for (int i = 0; i < _frameCount; ++i) {
                        for (int j = 0; j < _channelCount; ++j) {
                            _frameBuffer[j][i] = ((T)(*src++)) / scale;
                        }
                    }
                }
                else {
                    assert(0); // currently unsupported conversion
                }
            }
        }
    }
};

typedef AudioFrameBuffer< float32_t > AudioBufferFloat32;
typedef AudioFrameBuffer< int32_t > AudioBufferSInt32;

#endif // hifi_AudioBuffer_h

