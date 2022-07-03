//
//  Codec.h
//  libraries/shared/src
//
//  Created by Nshan G. 17 June 2022
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef VIRCADIA_LIBRARIES_SHARED_SRC_CODEC_H
#define VIRCADIA_LIBRARIES_SHARED_SRC_CODEC_H

#include <QByteArray>
#include <QString>

class Encoder {
public:
    virtual ~Encoder() { }
    virtual void encode(const QByteArray& decodedBuffer, QByteArray& encodedBuffer) = 0;
};

class Decoder {
public:
    virtual ~Decoder() { }
    virtual void decode(const QByteArray& encodedBuffer, QByteArray& decodedBuffer) = 0;

    virtual void lostFrame(QByteArray& decodedBuffer) = 0;
};

class Codec {
public:
    virtual const QString getName() const = 0;
    virtual Encoder* createEncoder(int sampleRate, int numChannels) = 0;
    virtual Decoder* createDecoder(int sampleRate, int numChannels) = 0;
    virtual void releaseEncoder(Encoder* encoder) = 0;
    virtual void releaseDecoder(Decoder* decoder) = 0;
};

#endif /* end of include guard */
