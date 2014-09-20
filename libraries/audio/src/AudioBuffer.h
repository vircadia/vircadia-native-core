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
    
protected:
    
    uint32_t _channelCount;
    uint32_t _channelCountMax;
    uint32_t _frameCount;
    uint32_t _frameCountMax;
    
    T** _frameBuffer;

    void allocateFrames();
    void deallocateFrames();
    
public:
    
    AudioFrameBuffer();
    AudioFrameBuffer(const uint32_t channelCount, const uint32_t frameCount);
    virtual ~AudioFrameBuffer();
    
    void initialize(const uint32_t channelCount, const uint32_t frameCount);
    void finalize();
                
    T**& getFrameData();
    uint32_t getChannelCount();
    uint32_t getFrameCount();

    template< typename S >
    void copyFrames(uint32_t channelCount, const uint32_t frameCount, S* frames, const bool copyOut = false);
    void zeroFrames();
};

template< typename T >
AudioFrameBuffer< T >::AudioFrameBuffer() :
    _channelCount(0),
    _frameCount(0),
    _frameCountMax(0),
    _frameBuffer(NULL) {
}

template< typename T >
AudioFrameBuffer< T >::AudioFrameBuffer(const uint32_t channelCount, const uint32_t frameCount) :
    _channelCount(channelCount),
    _channelCountMax(channelCount),
    _frameCount(frameCount),
    _frameCountMax(frameCount),
    _frameBuffer(NULL) {
    allocateFrames();
}

template< typename T >
AudioFrameBuffer< T >::~AudioFrameBuffer() {
    finalize();
}

template< typename T >
void AudioFrameBuffer< T >::allocateFrames() {
    _frameBuffer = new T*[_channelCountMax];
    if (_frameBuffer) {
        for (uint32_t i = 0; i < _channelCountMax; ++i) {
            _frameBuffer[i] = new T[_frameCountMax];
        }
    }
}

template< typename T >
void AudioFrameBuffer< T >::deallocateFrames() {
    if (_frameBuffer) {
        for (uint32_t i = 0; i < _channelCountMax; ++i) {
            delete _frameBuffer[i];
        }
        delete _frameBuffer;
    }
    _frameBuffer = NULL;
}

template< typename T >
void AudioFrameBuffer< T >::initialize(const uint32_t channelCount, const uint32_t frameCount) {
    if (_frameBuffer) {
        finalize();
    }
    _channelCount = channelCount;
    _channelCountMax = channelCount;
    _frameCount = frameCount;
    _frameCountMax = frameCount;
    allocateFrames();
}

template< typename T >
void AudioFrameBuffer< T >::finalize() {
    deallocateFrames();
    _channelCount = 0;
    _channelCountMax = 0;
    _frameCount = 0;
    _frameCountMax = 0;
}

template< typename T >
inline T**& AudioFrameBuffer< T >::getFrameData() {
    return _frameBuffer;
}

template< typename T >
inline uint32_t AudioFrameBuffer< T >::getChannelCount() {
    return _channelCount;
}

template< typename T >
inline uint32_t AudioFrameBuffer< T >::getFrameCount() {
    return _frameCount;
}

template< typename T >
inline void AudioFrameBuffer< T >::zeroFrames() {
    if (!_frameBuffer) {
        return;
    }
    for (uint32_t i = 0; i < _channelCountMax; ++i) {
        memset(_frameBuffer[i], 0, sizeof(T)*_frameCountMax);
    }
}

template< typename T >
template< typename S >
inline void AudioFrameBuffer< T >::copyFrames(uint32_t channelCount, const uint32_t frameCount, S* frames, const bool copyOut) {
    if ( !_frameBuffer || !frames) {
        return;
    }
    
    if (channelCount <=_channelCountMax && frameCount <=_frameCountMax) {
        // We always allow copying fewer frames than we have allocated
        _frameCount = frameCount;
        _channelCount = channelCount; 
    }
    else {
        //
        // However we do not attempt to copy more frames than we've allocated ;-) This is a framing error caused by either
        // a/ the platform audio driver not correctly queuing and regularly smoothing device IO capture frames -or-
        // b/ our IO processing thread (currently running on a Qt GUI thread) has been delayed/scheduled too late.
        // 
        // The fix is not to make the problem worse by allocating additional frames on this thread, rather, it is to handle
        // dynamic re-sizing off the IO processing thread.   While a/ is not in our control, we will address the off thread
        // re-sizing,, as well as b/, in later releases.
        //
        // For now, we log this condition, and do our best to recover by copying as many frames as we have allocated.  
        // Unfortunately, this will result (temporarily), in an audible discontinuity.
        // 
        // If you repeatedly receive this error, contact craig@highfidelity.io and send me what audio device you are using,
        // what audio-stack you are using (pulse/alsa, core audio, ...),  what OS, and what the reported frame/channel 
        // counts are.  In addition, any information about what you were doing at the time of the discontinuity, would be
        // useful (e.g., accessing any client features/menus)
        //
        qDebug() << "Audio framing error:  _channelCount=" 
        << _channelCount 
        << "channelCountMax=" 
        << _channelCountMax
        << "_frameCount=" 
        << _frameCount 
        << "frameCountMax=" 
        << _frameCountMax;
        
        
        _channelCount = std::min(_channelCount,_channelCountMax);
        _frameCount = std::min(_frameCount,_frameCountMax);
    }
    
    if (copyOut) {
        S* dst = frames;
        
        if(typeid(T) == typeid(S)) { // source and destination types are the same
            for (uint32_t i = 0; i < _frameCount; ++i) {
                for (uint32_t j = 0; j < _channelCount; ++j) {
                    *dst++ = _frameBuffer[j][i];
                }
            }
        }
        else {
            if(typeid(T) == typeid(float32_t) &&
               typeid(S) == typeid(int16_t)) {
                
                const int scale = (2 << ((8 * sizeof(S)) - 1));
                
                for (uint32_t i = 0; i < _frameCount; ++i) {
                    for (uint32_t j = 0; j < _channelCount; ++j) {
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
            for (uint32_t i = 0; i < _frameCount; ++i) {
                for (uint32_t j = 0; j < _channelCount; ++j) {
                    _frameBuffer[j][i] = *src++;
                }
            }
        }
        else {
            if(typeid(T) == typeid(float32_t) &&
               typeid(S) == typeid(int16_t)) {
                
                const int scale = (2 << ((8 * sizeof(S)) - 1));
                
                for (uint32_t i = 0; i < _frameCount; ++i) {
                    for (uint32_t j = 0; j < _channelCount; ++j) {
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

typedef AudioFrameBuffer< float32_t > AudioBufferFloat32;
typedef AudioFrameBuffer< int32_t > AudioBufferSInt32;

#endif // hifi_AudioBuffer_h

