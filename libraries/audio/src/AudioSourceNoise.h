//
//  AudioSourceNoise.h
//  hifi
//
//  Created by Craig Hansen-Sturm on 9/1/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Adapted from code by Phil Burk http://www.firstpr.com.au/dsp/pink-noise/
//

#ifndef hifi_AudioSourceNoise_h
#define hifi_AudioSourceNoise_h

template< const uint16_t N = 30>
class AudioSourceNoise
{
    static const uint16_t _randomRows = N;
    static const uint16_t _randomBits = 24;
    static const uint16_t _randomShift = (sizeof(int32_t) * 8) - _randomBits;
    
    static uint32_t _randomSeed;
    
	int32_t _rows[_randomRows];
	int32_t _runningSum; // used to optimize summing of generators.
	uint16_t _index; // incremented each sample.
	uint16_t _indexMask; // index wrapped by ANDing with this mask.
	float32_t _scale; // used to scale within range of -1.0 to +1.0
    
    static uint32_t generateRandomNumber() {
        _randomSeed = (_randomSeed * 196314165) + 907633515;
        return _randomSeed >> _randomShift;
    }
    
public:
    AudioSourceNoise() {
        initialize();
    }
    
    ~AudioSourceNoise() {
        finalize();
    }
    
    void initialize() {
        memset(_rows, 0, _randomRows * sizeof(int32_t));
        
        _runningSum = 0;
        _index = 0;
        _indexMask = (uint16_t)((1 << _randomRows) - 1);
        _scale = 1.0f / ((_randomRows + 1) * (1 << (_randomBits - 1)));
    }
    
    void finalize() {
    }
    
    void reset() {
        initialize();
    }
    
    void setParameters(void) {
    }
    
    void getParameters(void) {
    }
    
    void render(AudioBufferFloat32& frameBuffer) {
        
        uint32_t randomNumber;
        
        float32_t** samples = frameBuffer.getFrameData();
        for (uint32_t i = 0; i < frameBuffer.getFrameCount(); ++i) {
            for (uint32_t j = 0; j < frameBuffer.getChannelCount(); ++j) {
                
                _index = (_index + 1) & _indexMask; // increment and mask index.
                if (_index != 0) { // if index is zero, don't update any random values.
                    
                    uint32_t numZeros = 0; // determine how many trailing zeros in _index
                    uint32_t tmp = _index;
                    while ((tmp & 1) == 0) {
                        tmp >>= 1;
                        numZeros++;
                    }
                    // replace the indexed _rows random value. subtract and add back to _runningSum instead
                    // of adding all the random values together. only one value changes each time.
                    _runningSum -= _rows[numZeros];
                    randomNumber = generateRandomNumber();
                    _runningSum += randomNumber;
                    _rows[numZeros] = randomNumber;
                }
                
                // add extra white noise value and scale between -1.0 and +1.0
                samples[j][i] = (_runningSum + generateRandomNumber()) * _scale;
            }
        }
    }
};

typedef AudioSourceNoise<> AudioSourcePinkNoise;

#endif // AudioSourceNoise_h

