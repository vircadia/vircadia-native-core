//
//  AudioInjector.cpp
//  hifi
//
//  Created by Stephen Birarda on 4/23/13.
//
//

#include <sys/time.h>
#include <fstream>
#include <cstring>

#include "SharedUtil.h"
#include "PacketHeaders.h"

#include "AudioInjector.h"

const int BUFFER_LENGTH_BYTES = 512;
const int BUFFER_LENGTH_SAMPLES = BUFFER_LENGTH_BYTES / sizeof(int16_t);
const float SAMPLE_RATE = 22050.0f;
const float BUFFER_SEND_INTERVAL_USECS = (BUFFER_LENGTH_SAMPLES / SAMPLE_RATE) * 1000000;

AudioInjector::AudioInjector(const char* filename) :
    _position(),
    _bearing(0),
    _attenuationModifier(255),
    _indexOfNextSlot(0),
    _isInjectingAudio(false)
{
    std::fstream sourceFile;
    
    sourceFile.open(filename, std::ios::in | std::ios::binary);
    sourceFile.seekg(0, std::ios::end);
    
    int totalBytes = sourceFile.tellg();
    if (totalBytes == -1) {
        printf("Error reading audio data from file %s\n", filename);
        _audioSampleArray = NULL;
    } else {
        printf("Read %d bytes from audio file\n", totalBytes);
        sourceFile.seekg(0, std::ios::beg);
        _numTotalSamples = totalBytes / 2;
        _audioSampleArray = new int16_t[_numTotalSamples];
        
        sourceFile.read((char *)_audioSampleArray, totalBytes);
    }
}

AudioInjector::AudioInjector(int maxNumSamples) :
    _numTotalSamples(maxNumSamples),
    _position(),
    _bearing(0),
    _attenuationModifier(255),
    _indexOfNextSlot(0),
    _isInjectingAudio(false)
{
    _audioSampleArray = new int16_t[maxNumSamples];
    memset(_audioSampleArray, 0, _numTotalSamples * sizeof(int16_t));
}

AudioInjector::~AudioInjector() {
    delete[] _audioSampleArray;
}

void AudioInjector::setPosition(float* position) {
    _position[0] = position[0];
    _position[1] = position[1];
    _position[2] = position[2];
}

void AudioInjector::addSample(const int16_t sample) {
    if (_indexOfNextSlot != _numTotalSamples) {
        // only add this sample if we actually have space for it
        _audioSampleArray[_indexOfNextSlot++] = sample;
    }
}

void AudioInjector::addSamples(int16_t* sampleBuffer, int numSamples) {
    if (_audioSampleArray + _indexOfNextSlot + numSamples <= _audioSampleArray + (_numTotalSamples / sizeof(int16_t))) {
        // only copy the audio from the sample buffer if there's space
        memcpy(_audioSampleArray + _indexOfNextSlot, sampleBuffer, numSamples * sizeof(int16_t));
        _indexOfNextSlot += numSamples;
    }
}

void AudioInjector::injectAudio() {
    if (_audioSampleArray) {
        _isInjectingAudio = true;
        
        timeval startTime;
        
        // one byte for header, 3 positional floats, 1 bearing float, 1 attenuation modifier byte
        int leadingBytes = 1 + (sizeof(float) * 4) + 1;
        unsigned char dataPacket[BUFFER_LENGTH_BYTES + leadingBytes];
        
        dataPacket[0] = PACKET_HEADER_INJECT_AUDIO;
        unsigned char *currentPacketPtr = dataPacket + 1;
        
        for (int i = 0; i < 3; i++) {
            memcpy(currentPacketPtr, &_position[i], sizeof(float));
            currentPacketPtr += sizeof(float);
        }
        
        *currentPacketPtr = _attenuationModifier;
        currentPacketPtr++;
        
        memcpy(currentPacketPtr, &_bearing, sizeof(float));
        currentPacketPtr += sizeof(float);
        
        for (int i = 0; i < _numTotalSamples; i += BUFFER_LENGTH_SAMPLES) {
            gettimeofday(&startTime, NULL);
            
            int numSamplesToCopy = BUFFER_LENGTH_SAMPLES;
            
            if (_numTotalSamples - i < BUFFER_LENGTH_SAMPLES) {
                numSamplesToCopy = _numTotalSamples - i;
                memset(currentPacketPtr + numSamplesToCopy, 0, BUFFER_LENGTH_BYTES - (numSamplesToCopy * sizeof(int16_t)));
            }
            
            memcpy(currentPacketPtr, _audioSampleArray + i, numSamplesToCopy * sizeof(int16_t));
            
            _injectorSocket->send(&_destinationSocket, dataPacket, sizeof(dataPacket));
            
            double usecToSleep = BUFFER_SEND_INTERVAL_USECS - (usecTimestampNow() - usecTimestamp(&startTime));
            if (usecToSleep > 0) {
                usleep(usecToSleep);
            }
        }
        
        _isInjectingAudio = false;
    }
}

void* injectAudioViaThread(void* args) {
    AudioInjector* parentInjector = (AudioInjector*) args;
    parentInjector->injectAudio();
    
    pthread_exit(0);
}

void AudioInjector::threadInjectionOfAudio() {
    pthread_t audioInjectThread;
    pthread_create(&audioInjectThread, NULL, injectAudioViaThread, (void*) this);
}
