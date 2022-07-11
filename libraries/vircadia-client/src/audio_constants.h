//
//  audio_constants.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 12 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_MESSAGE_TYPES_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_MESSAGE_TYPES_H

#include <stdint.h>

#include "common.h"

/// @brief Value indicating floating point sample type of
/// vircadia_audio_format.
///
/// @return Positive integer.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_audio_sample_type_float();

/// @brief Value indicating signed 16 bit sample type of
/// vircadia_audio_format.
///
/// @return Positive integer.
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_audio_sample_type_sint16();

/// @brief Unique identifier/name of the Opus codec.
///
/// @return null terminated string.
VIRCADIA_CLIENT_DYN_API const char* vircadia_audio_codec_opus();

/// @brief Unique identifier/name of the raw PCM codec.
///
/// @return Null terminated string.
VIRCADIA_CLIENT_DYN_API const char* vircadia_audio_codec_pcm();

/// @brief Unique identifier/name of the zlib codec.
///
/// @return Null terminated string.
VIRCADIA_CLIENT_DYN_API const char* vircadia_audio_codec_zlib();

#endif /* end of include guard */
