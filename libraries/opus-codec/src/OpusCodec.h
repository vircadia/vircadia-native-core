//
//  OpusCodec.h
//  libraries/opus-codec/src
//
//  Created by Nshan G. on 3 July 2022.
//  Copyright 2019 Michael Bailey
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_OPUS_CODEC_SRC_OPUSCODEC_H
#define LIBRARIES_OPUS_CODEC_SRC_OPUSCODEC_H

#include <Codec.h>

class OpusCodec : public Codec {

public:
    const QString getName() const override { return NAME; }

    virtual Encoder* createEncoder(int sampleRate, int numChannels) override;
    virtual Decoder* createDecoder(int sampleRate, int numChannels) override;
    virtual void releaseEncoder(Encoder* encoder) override;
    virtual void releaseDecoder(Decoder* decoder) override;

private:
    static const char* NAME;
};

#endif /* end of include guard */
