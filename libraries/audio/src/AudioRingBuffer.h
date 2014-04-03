//
//  AudioRingBuffer.h
//  interface
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __interface__AudioRingBuffer__
#define __interface__AudioRingBuffer__

#include <limits>
#include <stdint.h>

#include <glm/glm.hpp>

#include <QtCore/QIODevice>

#include "NodeData.h"

const int SAMPLE_RATE = 24000;

const int NETWORK_BUFFER_LENGTH_BYTES_STEREO = 1024;
const int NETWORK_BUFFER_LENGTH_SAMPLES_STEREO = NETWORK_BUFFER_LENGTH_BYTES_STEREO / sizeof(int16_t);
const int NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL = 512;
const int NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL = NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL / sizeof(int16_t);

const unsigned int BUFFER_SEND_INTERVAL_USECS = floorf((NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL
                                                        / (float) SAMPLE_RATE) * 1000 * 1000);

const short RING_BUFFER_LENGTH_FRAMES = 10;

const int MAX_SAMPLE_VALUE = std::numeric_limits<int16_t>::max();
const int MIN_SAMPLE_VALUE = std::numeric_limits<int16_t>::min();

class AudioRingBuffer : public NodeData {
    Q_OBJECT
public:
    AudioRingBuffer(int numFrameSamples);
    ~AudioRingBuffer();

    void reset();
    void resizeForFrameSize(qint64 numFrameSamples);
    
    int getSampleCapacity() const { return _sampleCapacity; }
    
    int parseData(const QByteArray& packet);
    
    // assume callers using this will never wrap around the end
    const int16_t* getNextOutput() const { return _nextOutput; }
    const int16_t* getBuffer() const { return _buffer; }

    qint64 readSamples(int16_t* destination, qint64 maxSamples);
    qint64 writeSamples(const int16_t* source, qint64 maxSamples);
    
    qint64 readData(char* data, qint64 maxSize);
    qint64 writeData(const char* data, qint64 maxSize);
    
    int16_t& operator[](const int index);
    const int16_t& operator[] (const int index) const;
    
    void shiftReadPosition(unsigned int numSamples);
    
    unsigned int samplesAvailable() const;
    
    bool isNotStarvedOrHasMinimumSamples(unsigned int numRequiredSamples) const;
    
    bool isStarved() const { return _isStarved; }
    void setIsStarved(bool isStarved) { _isStarved = isStarved; }
    
    bool hasStarted() const { return _hasStarted; }
    
    void addSilentFrame(int numSilentSamples);
protected:
    // disallow copying of AudioRingBuffer objects
    AudioRingBuffer(const AudioRingBuffer&);
    AudioRingBuffer& operator= (const AudioRingBuffer&);
    
    int16_t* shiftedPositionAccomodatingWrap(int16_t* position, int numSamplesShift) const;
    
    int _sampleCapacity;
    int _numFrameSamples;
    int16_t* _nextOutput;
    int16_t* _endOfLastWrite;
    int16_t* _buffer;
    bool _isStarved;
    bool _hasStarted;
};

#endif /* defined(__interface__AudioRingBuffer__) */
