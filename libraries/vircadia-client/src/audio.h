//
//  audio.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 9 July 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef VIRCADIA_LIBRARIES_VIRCADIA_CLIENT_SRC_AUDIO_H
#define VIRCADIA_LIBRARIES_VIRCADIA_CLIENT_SRC_AUDIO_H

#include <stdint.h>

#include "common.h"

// FIXME: Avatar API also has these, so will need to move to common.h
// once merged.
struct vircadia_vector_ {
    float x;
    float y;
    float z;
};
struct vircadia_bounds_ {
    vircadia_vector_ dimensions;
    vircadia_vector_ offset;
};
struct vircadia_quaternion_ {
    float x;
    float y;
    float z;
    float w;
};
struct vircadia_vantage_ {
    vircadia_vector_ position;
    vircadia_quaternion_ rotation;
};

struct vircadia_audio_format {
    uint8_t sample_type;
    int sample_rate;
    int channel_count;
};

struct vircadia_audio_codec_params {
    uint8_t allowed;

    // TODO: implement and document once
    // https://github.com/vircadia/vircadia/pull/1582 is merged
    uint8_t encoder_vbr;
    uint8_t encoder_fec;
    int encoder_bitrate;
    int encoder_complexity;
    int encoder_packet_loss;
};

VIRCADIA_CLIENT_DYN_API
int vircadia_enable_audio(int context_id);

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_selected_audio_codec_name(int context_id);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_codec_params(int context_id, const char* codec, vircadia_audio_codec_params params);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_format(int context_id, vircadia_audio_format format);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_output_format(int context_id, vircadia_audio_format format);

VIRCADIA_CLIENT_DYN_API
uint8_t* vircadia_get_audio_input_context(int context_id);

VIRCADIA_CLIENT_DYN_API
uint8_t* vircadia_get_audio_output_context(int context_id);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_data(uint8_t* audio_context, const uint8_t* data, int size);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_audio_output_data(uint8_t* audio_context, uint8_t* data, int size);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_bounds(int context_id, vircadia_bounds_ bounds);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_vantage(int context_id, vircadia_vantage_ vantage);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_echo(int context_id, uint8_t enabled);

VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_muted(int context_id, uint8_t muted);

VIRCADIA_CLIENT_DYN_API
int vircadia_get_audio_input_muted_by_mixer(int context_id);

#endif /* end of include guard */
