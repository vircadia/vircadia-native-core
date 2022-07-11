//
//  PCMCodec.h
//  libraries/pcm-codec/src
//
//  Created by Nshan G. on 3 July 2022.
//  Copyright 2016 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_PCM_CODEC_SRC_PCMCODEC_H
#define LIBRARIES_PCM_CODEC_SRC_PCMCODEC_H

#include <Codec.h>
#include <AudioConstants.h>

#include <QObject>

class PCMCodec : public Codec, public Encoder, public Decoder {

public:
    const QString getName() const override { return NAME; }

    virtual Encoder* createEncoder(int sampleRate, int numChannels) override;
    virtual Decoder* createDecoder(int sampleRate, int numChannels) override;
    virtual void releaseEncoder(Encoder* encoder) override;
    virtual void releaseDecoder(Decoder* decoder) override;

    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) override {
        encodedBuffer = decodedBuffer;
    }

    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) override {
        decodedBuffer = encodedBuffer;
    }

    virtual void lostFrame(QByteArray& decodedBuffer) override {
        decodedBuffer.resize(AudioConstants::NETWORK_FRAME_BYTES_STEREO);
        memset(decodedBuffer.data(), 0, decodedBuffer.size());
    }

    static const char* getNameCString() { return NAME; }

private:
    static const char* NAME;
};

class zLibCodec : public Codec, public Encoder, public Decoder {

public:
    const QString getName() const override { return NAME; }

    virtual Encoder* createEncoder(int sampleRate, int numChannels) override;
    virtual Decoder* createDecoder(int sampleRate, int numChannels) override;
    virtual void releaseEncoder(Encoder* encoder) override;
    virtual void releaseDecoder(Decoder* decoder) override;

    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) override {
        encodedBuffer = qCompress(decodedBuffer);
    }

    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) override {
        decodedBuffer = qUncompress(encodedBuffer);
    }

    virtual void lostFrame(QByteArray& decodedBuffer) override {
        decodedBuffer.resize(AudioConstants::NETWORK_FRAME_BYTES_STEREO);
        memset(decodedBuffer.data(), 0, decodedBuffer.size());
    }

    static const char* getNameCString() { return NAME; }

private:
    static const char* NAME;
};

#endif /* end of include guard */
