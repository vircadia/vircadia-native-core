//
//  AvatarAudioStream.cpp
//  assignment-client/src/audio
//
//  Created by Stephen Birarda on 6/5/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarAudioStream.h"

#include <udt/PacketHeaders.h>

#include "AudioLogging.h"

AvatarAudioStream::AvatarAudioStream(bool isStereo, int numStaticJitterFrames) :
    PositionalAudioStream(PositionalAudioStream::Microphone, isStereo, numStaticJitterFrames) {}

int AvatarAudioStream::parseStreamProperties(PacketType type, const QByteArray& packetAfterSeqNum, int& numAudioSamples) {
    int readBytes = 0;

    if (type == PacketType::SilentAudioFrame) {
        const char* dataAt = packetAfterSeqNum.constData();
        SilentSamplesBytes numSilentSamples = *(reinterpret_cast<const quint16*>(dataAt));
        readBytes += sizeof(SilentSamplesBytes);
        numAudioSamples = (int) numSilentSamples;

        // read the positional data
        readBytes += parsePositionalData(packetAfterSeqNum.mid(readBytes));

    } else {
        _shouldLoopbackForNode = (type == PacketType::MicrophoneAudioWithEcho);

        // read the channel flag
        ChannelFlag channelFlag = packetAfterSeqNum.at(readBytes);
        bool isStereo = channelFlag == 1;
        readBytes += sizeof(ChannelFlag);

        // if isStereo value has changed, restart the ring buffer with new frame size
        if (isStereo != _isStereo) {
            _ringBuffer.resizeForFrameSize(isStereo
                                           ? AudioConstants::NETWORK_FRAME_SAMPLES_STEREO
                                           : AudioConstants::NETWORK_FRAME_SAMPLES_PER_CHANNEL);
            // restart the codec
            if (_codec) {
                if (_decoder) {
                    _codec->releaseDecoder(_decoder);
                }
                _decoder = _codec->createDecoder(AudioConstants::SAMPLE_RATE, isStereo ? AudioConstants::STEREO : AudioConstants::MONO);
            }
            qCDebug(audio) << "resetting AvatarAudioStream... codec:" << _selectedCodecName << "isStereo:" << isStereo;

            _isStereo = isStereo;
        }

        // read the positional data
        readBytes += parsePositionalData(packetAfterSeqNum.mid(readBytes));
        
        // calculate how many samples are in this packet
        int numAudioBytes = packetAfterSeqNum.size() - readBytes;
        numAudioSamples = numAudioBytes / sizeof(int16_t);
    }

    return readBytes;
}
