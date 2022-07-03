//
//  OpusDecoder.h
//  libraries/opus-codec/src
//
//  Copyright 2020 Dale Glass
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_OPUS_CODEC_SRC_OPUSDECODER_H
#define LIBRARIES_OPUS_CODEC_SRC_OPUSDECODER_H


#include <Codec.h>
#include <opus/opus.h>

class OpusDecoder : public Decoder {
public:
    OpusDecoder(int sampleRate, int numChannels);
    ~OpusDecoder() override;


    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) override;
    virtual void lostFrame(QByteArray &decodedBuffer) override;


private:
    int _encodedSize;

    OpusDecoder* _decoder = nullptr;
    int _opusSampleRate = 0;
    int _opusNumChannels = 0;
    int _decodedSize = 0;
};

#endif /* end of include guard */
