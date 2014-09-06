//
//  AudioFormat.h
//  hifi
//
//  Created by Craig Hansen-Sturm on 8/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioFormat_h
#define hifi_AudioFormat_h

#ifndef _FLOAT32_T
#define _FLOAT32_T
typedef float float32_t;
#endif

#ifndef _FLOAT64_T
#define _FLOAT64_T
typedef double float64_t;
#endif

//
// Audio format structure (currently for uncompressed streams only)
//

struct AudioFormat {

    struct Flags {
        uint32_t _isFloat : 1;
        uint32_t _isSigned : 1;
        uint32_t _isInterleaved : 1;
        uint32_t _isBigEndian : 1;
        uint32_t _isPacked : 1;
        uint32_t _reserved : 27;
    } _flags;

    uint32_t _bytesPerFrame;
    uint32_t _channelsPerFrame;
    uint32_t _bitsPerChannel;
    float64_t _sampleRate;
    
    AudioFormat() {
        std::memset(this, 0, sizeof(*this));
    }
    ~AudioFormat() { }
    
    AudioFormat& operator=(const AudioFormat& fmt) {
        std::memcpy(this, &fmt, sizeof(*this));
        return *this;
    }
    
    bool operator==(const AudioFormat& fmt) {
        return std::memcmp(this, &fmt, sizeof(*this)) == 0;
    }
    
    bool operator!=(const AudioFormat& fmt) {
        return std::memcmp(this, &fmt, sizeof(*this)) != 0;
    }

    void setCanonicalFloat32(uint32_t channels) {
        assert(channels > 0 && channels <= 2);
        _sampleRate = SAMPLE_RATE; // todo:  create audio constants header
        _bitsPerChannel = sizeof(float32_t) * 8;
        _channelsPerFrame = channels;
        _bytesPerFrame = _channelsPerFrame * _bitsPerChannel / 8;
        _flags._isFloat = true;
        _flags._isInterleaved = _channelsPerFrame > 1;
    }
    
    void setCanonicalInt16(uint32_t channels) {
        assert(channels > 0 && channels <= 2);
        _sampleRate = SAMPLE_RATE; // todo:  create audio constants header
        _bitsPerChannel = sizeof(int16_t) * 8;
        _channelsPerFrame = channels;
        _bytesPerFrame = _channelsPerFrame * _bitsPerChannel / 8;
        _flags._isSigned = true;
        _flags._isInterleaved = _channelsPerFrame > 1;
    }
};

#endif // hifi_AudioFormat_h
