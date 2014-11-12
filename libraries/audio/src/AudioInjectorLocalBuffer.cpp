//
//  AudioInjectorLocalBuffer.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 2014-11-11.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AudioInjectorLocalBuffer.h"

AudioInjectorLocalBuffer::AudioInjectorLocalBuffer(const QByteArray& rawAudioArray, QObject* parent) :
    QIODevice(parent),
    _rawAudioArray(rawAudioArray),
    _isLooping(false),
    _isStopped(false)
{
    
}

void AudioInjectorLocalBuffer::stop() {
    _isStopped = true;
    QIODevice::close();
}

qint64 AudioInjectorLocalBuffer::readData(char* data, qint64 maxSize) {
    if (!_isStopped) {
        int bytesToEnd = _rawAudioArray.size() - pos();
        
        int bytesToRead = maxSize;
        
        if (maxSize > bytesToEnd) {
            bytesToRead = bytesToEnd;
        }
        
        memcpy(data, _rawAudioArray.data() + pos(), bytesToRead);
        return bytesToRead;
    } else {
        return 0;
    }
}