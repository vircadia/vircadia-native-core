//
//  AudioRingBuffer.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2/1/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioRingBuffer_h
#define hifi_AudioRingBuffer_h

#include "AudioConstants.h"

#include <QtCore/QIODevice>

#include <SharedUtil.h>
#include <NodeData.h>

const int DEFAULT_RING_BUFFER_FRAME_CAPACITY = 10;

class AudioRingBuffer {
public:
    AudioRingBuffer(int numFrameSamples, int numFramesCapacity = DEFAULT_RING_BUFFER_FRAME_CAPACITY);
    ~AudioRingBuffer();

    void reset();
    void resizeForFrameSize(int numFrameSamples);

    void clear();

    int getSampleCapacity() const { return _sampleCapacity; }
    int getFrameCapacity() const { return _frameCapacity; }

    int readSamples(int16_t* destination, int maxSamples);
    int writeSamples(const int16_t* source, int maxSamples);

    int readData(char* data, int maxSize);
    int writeData(const char* data, int maxSize);

    int16_t& operator[](const int index);
    const int16_t& operator[] (const int index) const;

    void shiftReadPosition(unsigned int numSamples);

    float getNextOutputFrameLoudness() const;

    int samplesAvailable() const;
    int framesAvailable() const { return (_numFrameSamples == 0) ? 0 : samplesAvailable() / _numFrameSamples; }

    int getNumFrameSamples() const { return _numFrameSamples; }

    int getOverflowCount() const { return _overflowCount; } /// how many times has the ring buffer has overwritten old data

    int addSilentSamples(int samples);

private:
    float getFrameLoudness(const int16_t* frameStart) const;

protected:
    // disallow copying of AudioRingBuffer objects
    AudioRingBuffer(const AudioRingBuffer&);
    AudioRingBuffer& operator= (const AudioRingBuffer&);

    int16_t* shiftedPositionAccomodatingWrap(int16_t* position, int numSamplesShift) const;

    int _frameCapacity;
    int _sampleCapacity;
    int _bufferLength;      // actual length of _buffer: will be one frame larger than _sampleCapacity
    int _numFrameSamples;
    int16_t* _nextOutput;
    int16_t* _endOfLastWrite;
    int16_t* _buffer;

    int _overflowCount; /// how many times has the ring buffer has overwritten old data

public:
    class ConstIterator { //public std::iterator < std::forward_iterator_tag, int16_t > {
    public:
        ConstIterator()
            : _bufferLength(0),
            _bufferFirst(NULL),
            _bufferLast(NULL),
            _at(NULL) {}
        ConstIterator(int16_t* bufferFirst, int capacity, int16_t* at)
            : _bufferLength(capacity),
            _bufferFirst(bufferFirst),
            _bufferLast(bufferFirst + capacity - 1),
            _at(at) {}
        ConstIterator(const ConstIterator& rhs) = default;

        bool isNull() const { return _at == NULL; }

        bool operator==(const ConstIterator& rhs) { return _at == rhs._at; }
        bool operator!=(const ConstIterator& rhs) { return _at != rhs._at; }
        const int16_t& operator*() { return *_at; }

        ConstIterator& operator=(const ConstIterator& rhs) {
            _bufferLength = rhs._bufferLength;
            _bufferFirst = rhs._bufferFirst;
            _bufferLast = rhs._bufferLast;
            _at = rhs._at;
            return *this;
        }

        ConstIterator& operator++() {
            _at = (_at == _bufferLast) ? _bufferFirst : _at + 1;
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp(*this);
            ++(*this);
            return tmp;
        }

        ConstIterator& operator--() {
            _at = (_at == _bufferFirst) ? _bufferLast : _at - 1;
            return *this;
        }

        ConstIterator operator--(int) {
            ConstIterator tmp(*this);
            --(*this);
            return tmp;
        }

        const int16_t& operator[] (int i) {
            return *atShiftedBy(i);
        }

        ConstIterator operator+(int i) {
            return ConstIterator(_bufferFirst, _bufferLength, atShiftedBy(i));
        }

        ConstIterator operator-(int i) {
            return ConstIterator(_bufferFirst, _bufferLength, atShiftedBy(-i));
        }

        void readSamples(int16_t* dest, int numSamples) {
            auto samplesToEnd = _bufferLast - _at + 1;
            
            if (samplesToEnd >= numSamples) {
                memcpy(dest, _at, numSamples * sizeof(int16_t));
                _at += numSamples;
            } else {
                auto samplesFromStart = numSamples - samplesToEnd;
                memcpy(dest, _at, samplesToEnd * sizeof(int16_t));
                memcpy(dest + samplesToEnd, _bufferFirst, samplesFromStart * sizeof(int16_t));

                _at = _bufferFirst + samplesFromStart;
            }
        }

        void readSamplesWithFade(int16_t* dest, int numSamples, float fade) {
            int16_t* at = _at;
            for (int i = 0; i < numSamples; i++) {
                *dest = (float)*at * fade;
                ++dest;
                at = (at == _bufferLast) ? _bufferFirst : at + 1;
            }
        }

    private:
        int16_t* atShiftedBy(int i) {
            i = (_at - _bufferFirst + i) % _bufferLength;
            if (i < 0) {
                i += _bufferLength;
            }
            return _bufferFirst + i;
        }

    private:
        int _bufferLength;
        int16_t* _bufferFirst;
        int16_t* _bufferLast;
        int16_t* _at;
    };

    ConstIterator nextOutput() const { return ConstIterator(_buffer, _bufferLength, _nextOutput); }
    ConstIterator lastFrameWritten() const { return ConstIterator(_buffer, _bufferLength, _endOfLastWrite) - _numFrameSamples; }

    float getFrameLoudness(ConstIterator frameStart) const;

    int writeSamples(ConstIterator source, int maxSamples);
    int writeSamplesWithFade(ConstIterator source, int maxSamples, float fade);
};

#endif // hifi_AudioRingBuffer_h
