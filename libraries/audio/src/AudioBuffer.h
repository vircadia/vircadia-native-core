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
    } else {
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
    
    bool frameAlignment16 = (_frameCount & 0x0F) == 0;
    
    if (copyOut) {
        S* dst = frames;
        
        if(typeid(T) == typeid(S)) { // source and destination types are the same, just copy out
            
            if (frameAlignment16 && (_channelCount == 1 || _channelCount == 2)) {
                
                if (_channelCount == 1) {
                    for (uint32_t i = 0; i < _frameCount; i += 16) {
                        *dst++ = _frameBuffer[0][i + 0];
                        *dst++ = _frameBuffer[0][i + 1];
                        *dst++ = _frameBuffer[0][i + 2];
                        *dst++ = _frameBuffer[0][i + 3];
                        *dst++ = _frameBuffer[0][i + 4];
                        *dst++ = _frameBuffer[0][i + 5];
                        *dst++ = _frameBuffer[0][i + 6];
                        *dst++ = _frameBuffer[0][i + 7];
                        *dst++ = _frameBuffer[0][i + 8];
                        *dst++ = _frameBuffer[0][i + 9];
                        *dst++ = _frameBuffer[0][i + 10];
                        *dst++ = _frameBuffer[0][i + 11];
                        *dst++ = _frameBuffer[0][i + 12];
                        *dst++ = _frameBuffer[0][i + 13];
                        *dst++ = _frameBuffer[0][i + 14];
                        *dst++ = _frameBuffer[0][i + 15];
                    }
                } else if (_channelCount == 2) {
                    for (uint32_t i = 0; i < _frameCount; i += 16) {
                        *dst++ = _frameBuffer[0][i + 0];
                        *dst++ = _frameBuffer[1][i + 0];
                        *dst++ = _frameBuffer[0][i + 1];
                        *dst++ = _frameBuffer[1][i + 1];
                        *dst++ = _frameBuffer[0][i + 2];
                        *dst++ = _frameBuffer[1][i + 2];
                        *dst++ = _frameBuffer[0][i + 3];
                        *dst++ = _frameBuffer[1][i + 3];
                        *dst++ = _frameBuffer[0][i + 4];
                        *dst++ = _frameBuffer[1][i + 4];
                        *dst++ = _frameBuffer[0][i + 5];
                        *dst++ = _frameBuffer[1][i + 5];
                        *dst++ = _frameBuffer[0][i + 6];
                        *dst++ = _frameBuffer[1][i + 6];
                        *dst++ = _frameBuffer[0][i + 7];
                        *dst++ = _frameBuffer[1][i + 7];
                        *dst++ = _frameBuffer[0][i + 8];
                        *dst++ = _frameBuffer[1][i + 8];
                        *dst++ = _frameBuffer[0][i + 9];
                        *dst++ = _frameBuffer[1][i + 9];
                        *dst++ = _frameBuffer[0][i + 10];
                        *dst++ = _frameBuffer[1][i + 10];
                        *dst++ = _frameBuffer[0][i + 11];
                        *dst++ = _frameBuffer[1][i + 11];
                        *dst++ = _frameBuffer[0][i + 12];
                        *dst++ = _frameBuffer[1][i + 12];
                        *dst++ = _frameBuffer[0][i + 13];
                        *dst++ = _frameBuffer[1][i + 13];
                        *dst++ = _frameBuffer[0][i + 14];
                        *dst++ = _frameBuffer[1][i + 14];
                        *dst++ = _frameBuffer[0][i + 15];
                        *dst++ = _frameBuffer[1][i + 15];
                    }
                }
            } else {
                for (uint32_t i = 0; i < _frameCount; ++i) {
                    for (uint32_t j = 0; j < _channelCount; ++j) {
                        *dst++ = _frameBuffer[j][i];
                    }
                }
            }
        } else {
            if(typeid(T) == typeid(float32_t) &&
               typeid(S) == typeid(int16_t)) { // source and destination aare not the same, convert from float32_t to int16_t and copy out
                
                const int scale = (2 << ((8 * sizeof(S)) - 1));

                if (frameAlignment16 && (_channelCount == 1 || _channelCount == 2)) {
                    
                    if (_channelCount == 1) {
                        for (uint32_t i = 0; i < _frameCount; i += 16) {
                            *dst++ = (S)(_frameBuffer[0][i + 0] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 1] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 2] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 3] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 4] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 5] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 6] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 7] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 8] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 9] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 10] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 11] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 12] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 13] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 14] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 15] * scale);
                        }
                    } else if (_channelCount == 2) {
                        for (uint32_t i = 0; i < _frameCount; i += 16) {
                            *dst++ = (S)(_frameBuffer[0][i + 0] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 0] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 1] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 1] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 2] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 2] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 3] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 3] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 4] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 4] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 5] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 5] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 6] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 6] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 7] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 7] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 8] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 8] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 9] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 9] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 10] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 10] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 11] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 11] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 12] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 12] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 13] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 13] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 14] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 14] * scale);
                            *dst++ = (S)(_frameBuffer[0][i + 15] * scale);
                            *dst++ = (S)(_frameBuffer[1][i + 15] * scale);
                        }
                    }
                } else {
                    for (uint32_t i = 0; i < _frameCount; ++i) {
                        for (uint32_t j = 0; j < _channelCount; ++j) {
                            *dst++ = (S)(_frameBuffer[j][i] * scale);
                        }
                    }
                }
            } else {
                assert(0); // currently unsupported conversion
            }
        }
    } else { // copyIn 
        S* src = frames;
        
        if(typeid(T) == typeid(S)) { // source and destination types are the same, copy in
            
            if (frameAlignment16 && (_channelCount == 1 || _channelCount == 2)) {
                
                if (_channelCount == 1) {
                    for (uint32_t i = 0; i < _frameCount; i += 16) {
                        _frameBuffer[0][i + 0] = *src++;
                        _frameBuffer[0][i + 1] = *src++;
                        _frameBuffer[0][i + 2] = *src++;
                        _frameBuffer[0][i + 3] = *src++;
                        _frameBuffer[0][i + 4] = *src++;
                        _frameBuffer[0][i + 5] = *src++;
                        _frameBuffer[0][i + 6] = *src++;
                        _frameBuffer[0][i + 7] = *src++;
                        _frameBuffer[0][i + 8] = *src++;
                        _frameBuffer[0][i + 9] = *src++;
                        _frameBuffer[0][i + 10] = *src++;
                        _frameBuffer[0][i + 11] = *src++;
                        _frameBuffer[0][i + 12] = *src++;
                        _frameBuffer[0][i + 13] = *src++;
                        _frameBuffer[0][i + 14] = *src++;
                        _frameBuffer[0][i + 15] = *src++;
                    }
                } else if (_channelCount == 2) {
                    for (uint32_t i = 0; i < _frameCount; i += 16) {
                        _frameBuffer[0][i + 0] = *src++;
                        _frameBuffer[1][i + 0] = *src++;
                        _frameBuffer[0][i + 1] = *src++;
                        _frameBuffer[1][i + 1] = *src++;
                        _frameBuffer[0][i + 2] = *src++;
                        _frameBuffer[1][i + 2] = *src++;
                        _frameBuffer[0][i + 3] = *src++;
                        _frameBuffer[1][i + 3] = *src++;
                        _frameBuffer[0][i + 4] = *src++;
                        _frameBuffer[1][i + 4] = *src++;
                        _frameBuffer[0][i + 5] = *src++;
                        _frameBuffer[1][i + 5] = *src++;
                        _frameBuffer[0][i + 6] = *src++;
                        _frameBuffer[1][i + 6] = *src++;
                        _frameBuffer[0][i + 7] = *src++;
                        _frameBuffer[1][i + 7] = *src++;
                        _frameBuffer[0][i + 8] = *src++;
                        _frameBuffer[1][i + 8] = *src++;
                        _frameBuffer[0][i + 9] = *src++;
                        _frameBuffer[1][i + 9] = *src++;
                        _frameBuffer[0][i + 10] = *src++;
                        _frameBuffer[1][i + 10] = *src++;
                        _frameBuffer[0][i + 11] = *src++;
                        _frameBuffer[1][i + 11] = *src++;
                        _frameBuffer[0][i + 12] = *src++;
                        _frameBuffer[1][i + 12] = *src++;
                        _frameBuffer[0][i + 13] = *src++;
                        _frameBuffer[1][i + 13] = *src++;
                        _frameBuffer[0][i + 14] = *src++;
                        _frameBuffer[1][i + 14] = *src++;
                        _frameBuffer[0][i + 15] = *src++;
                        _frameBuffer[1][i + 15] = *src++;
                    }
                }
            } else { 
                for (uint32_t i = 0; i < _frameCount; ++i) {
                    for (uint32_t j = 0; j < _channelCount; ++j) {
                        _frameBuffer[j][i] = *src++;
                    }
                }
            }
        } else { 
            if(typeid(T) == typeid(float32_t) &&
               typeid(S) == typeid(int16_t)) { // source and destination aare not the same, convert from int16_t to float32_t and copy in
                
                const int scale = (2 << ((8 * sizeof(S)) - 1));
                
                if (frameAlignment16 && (_channelCount == 1 || _channelCount == 2)) {
                    
                    if (_channelCount == 1) {
                        for (uint32_t i = 0; i < _frameCount; i += 16) {
                            _frameBuffer[0][i + 0] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 1] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 2] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 3] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 4] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 5] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 6] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 7] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 8] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 9] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 10] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 11] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 12] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 13] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 14] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 15] = ((T)(*src++)) / scale;
                        }
                    } else if (_channelCount == 2) {
                        for (uint32_t i = 0; i < _frameCount; i += 16) {
                            _frameBuffer[0][i + 0] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 0] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 1] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 1] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 2] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 2] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 3] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 3] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 4] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 4] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 5] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 5] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 6] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 6] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 7] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 7] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 8] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 8] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 9] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 9] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 10] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 10] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 11] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 11] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 12] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 12] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 13] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 13] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 14] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 14] = ((T)(*src++)) / scale;
                            _frameBuffer[0][i + 15] = ((T)(*src++)) / scale;
                            _frameBuffer[1][i + 15] = ((T)(*src++)) / scale;
                        }
                    }
                } else {
                    for (uint32_t i = 0; i < _frameCount; ++i) {
                        for (uint32_t j = 0; j < _channelCount; ++j) {
                            _frameBuffer[j][i] = ((T)(*src++)) / scale;
                        }
                    }
                }
            } else {
                assert(0); // currently unsupported conversion
            }
        }
    }
}

typedef AudioFrameBuffer< float32_t > AudioBufferFloat32;
typedef AudioFrameBuffer< int32_t > AudioBufferSInt32;

#endif // hifi_AudioBuffer_h

