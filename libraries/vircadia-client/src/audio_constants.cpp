//
//  audio_constants.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 12 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "audio_constants.h"

#include <AudioPacketHandler.h>
#include <PCMCodec.h>
#include <OpusCodec.h>


VIRCADIA_CLIENT_DYN_API uint8_t vircadia_audio_sample_type_float() {
    return AudioFormat::Float;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_audio_sample_type_sint16() {
    return AudioFormat::Signed16;
}

VIRCADIA_CLIENT_DYN_API const char* vircadia_audio_codec_opus() {
    return OpusCodec::getNameCString();
}

VIRCADIA_CLIENT_DYN_API const char* vircadia_audio_codec_pcm() {
    return PCMCodec().getNameCString();
}

VIRCADIA_CLIENT_DYN_API const char* vircadia_audio_codec_zlib() {
    return zLibCodec().getNameCString();
}

