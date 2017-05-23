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

AudioInjectorLocalBuffer::AudioInjectorLocalBuffer(const QByteArray& rawAudioArray) :
    _rawAudioArray(rawAudioArray),
    _shouldLoop(false),
    _isStopped(false),
    _currentOffset(0)
{
    
}

void AudioInjectorLocalBuffer::stop() {
    _isStopped = true;
    
    QIODevice::close();
}

bool AudioInjectorLocalBuffer::seek(qint64 pos) {
    if (_isStopped) {
        return false;
    } else {
        return QIODevice::seek(pos);
    }
}


qint64 AudioInjectorLocalBuffer::readData(char* data, qint64 maxSize) {
    if (!_isStopped) {
        
        // first copy to the end of the raw audio
        int bytesToEnd = _rawAudioArray.size() - _currentOffset;
        
        int bytesRead = maxSize;
        
        if (maxSize > bytesToEnd) {
            bytesRead = bytesToEnd;
        }
        
        memcpy(data, _rawAudioArray.data() + _currentOffset, bytesRead);
        
        // now check if we are supposed to loop and if we can copy more from the beginning
        if (_shouldLoop && maxSize != bytesRead) {
            bytesRead += recursiveReadFromFront(data + bytesRead, maxSize - bytesRead);
        } else {
            _currentOffset += bytesRead;
        }
        
        if (_shouldLoop && _currentOffset == _rawAudioArray.size()) {
            _currentOffset = 0;
        }
        
        return bytesRead;
    } else {
        return 0;
    }
}

qint64 AudioInjectorLocalBuffer::recursiveReadFromFront(char* data, qint64 maxSize) {
    // see how much we can get in this pass
    int bytesRead = maxSize;
    
    if (bytesRead > _rawAudioArray.size()) {
        bytesRead = _rawAudioArray.size();
    }
    
    // copy that amount
    memcpy(data, _rawAudioArray.data(), bytesRead);
    
    // check if we need to call ourselves again and pull from the front again
    if (bytesRead < maxSize) {
        return bytesRead + recursiveReadFromFront(data + bytesRead, maxSize - bytesRead);
    } else {
        _currentOffset = bytesRead;
        return bytesRead;
    }
}
