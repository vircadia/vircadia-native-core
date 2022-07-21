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

/// @brief Basic information of PCM audio data.
///
/// Used to specify the format of audio input/output
/// (vircadia_set_audio_input_format()/vircadia_set_audio_output_format()).
/// For multiple channels the audio samples are interleaved in both cases.
struct vircadia_audio_format {

    /// @brief The type of a single sample.
    ///
    /// Valid values are defined in audio_constants.h
    uint8_t sample_type;

    /// @brief The rate at which samples are processed.
    ///
    /// Specified as number of samples per second (Hz). This can be set to any
    /// reasonable value (usually 8000-96000), typically 48000. The
    /// input/output data will be re-sampled to/from desired network sample rate.
    int sample_rate;

    /// @brief The number of channels.
    ///
    /// Input supports only mono(1 channel) and stereo(2 channels), and these
    /// are sent to the mixer as is. \n
    /// Output supports any value above zero (usually 1-8), but the incoming
    /// mixer data is always stereo, so for other channel counts the data is
    /// converted in a straight forward fashion (linear downmix, or upmix with
    /// extra channels set to 0).
    int channel_count;
};

/// @brief Audio codec parameters.
///
/// Used with vircadia_set_audio_codec_params().
struct vircadia_audio_codec_params {

    /// @brief Dis/allow the use of the codec for commination with the mixer.
    ///
    /// 0 - disallow \n
    /// 1 - allow \n
    ///
    /// By default all codecs are allowed, and the mixer decides which to use
    /// with the client (vircadia_get_selected_audio_codec_name()). This
    /// parameter can be used to limit the choices for the mixer.
    uint8_t allowed;

    // TODO: implement and document once
    // https://github.com/vircadia/vircadia/pull/1582 is merged
    uint8_t encoder_vbr;
    uint8_t encoder_fec;
    int encoder_bitrate;
    int encoder_complexity;
    int encoder_packet_loss;
};

/// @brief Enable handling of audio input and output.
///
/// To start sending audio, the input the format must be specified with
/// vircadia_set_audio_input_format(), after which the data can be sent with
/// vircadia_set_audio_input_data(), with a input context retrieved from
/// vircadia_get_audio_input_context(). vircadia_set_audio_input_data() can be
/// called from a different thread with a valid input context. The input
/// context is invalidated if a new format is set (then a new context must be
/// retrieved before sending more data) or when entire client context is
/// destroyed. The audio output works in the same way:
/// vircadia_set_audio_output_format() -> vircadia_get_audio_output_context()
/// -> vircadia_get_audio_output_data().
///
/// @param context_id - The id of the client context (context.h).
///
/// @return non-negative value on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
VIRCADIA_CLIENT_DYN_API
int vircadia_enable_audio(int context_id);

/// @brief Reads the audio codec selected for communication with the mixer.
///
/// This is determined by the mixer, but the client can control allowed codecs
/// with vircadia_set_audio_codec_params(). \n
/// Possible values are defined in audio_constants.h.
///
/// @param context_id - The id of the client context (context.h).
///
/// @return A codec identifier, or an empty string if a codec has not been
/// selected yet, or null if an error occurred.
VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_selected_audio_codec_name(int context_id);

/// @brief Sets parameters of specified audio codec.
///
/// @param context_id - The id of the client context (context.h).
/// @param codec - The id of the codec (audio_constants.h).
/// @param params - The codec parameters to set.
///
/// @return non-negative value on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_audio_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_codec_params(int context_id, const char* codec, vircadia_audio_codec_params params);

/// @brief Sets the format of audio input data.
///
/// Specifically it's the format of the data to be passed to
/// vircadia_set_audio_input_data(). \n
/// This function initiates a creation of new input context and invalidates any
/// previous input context. The new context can be retrieved using
/// vircadia_get_audio_input_context().
///
/// @param context_id - The id of the client context (context.h).
/// @param format - The format of audio input data to be sent.
///
/// @return non-negative value on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_audio_disabled() \n
/// vircadia_audio_format_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_format(int context_id, vircadia_audio_format format);

/// @brief Sets the format of audio output data.
///
/// Specifically it's the format of the data retrieved from
/// vircadia_get_audio_output_data(). \n
/// This function initiates a creation of new output context and invalidates
/// any previous output context. The new context can be retrieved using
/// vircadia_get_audio_output_context().
///
/// @param context_id - The id of the client context (context.h).
/// @param format - The format of audio output data to be received.
///
/// @return non-negative value on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_audio_disabled() \n
/// vircadia_audio_format_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_output_format(int context_id, vircadia_audio_format format);

/// @brief Retrieves audio input context.
///
/// This function must be polled to retrieve a new valid input context for
/// vircadia_set_audio_input_data(), after every call to
/// vircadia_set_audio_input_format().
///
/// @param context_id - The id of the client context (context.h).
///
/// @return A pointer to the input context, or null if context is not yet
/// created/ready.
VIRCADIA_CLIENT_DYN_API
uint8_t* vircadia_get_audio_input_context(int context_id);

/// @brief Retrieves audio output context.
///
/// This function must be polled to retrieve a new valid output context for
/// vircadia_get_audio_output_data(), after every call to
/// vircadia_set_audio_output_format().
///
/// @param context_id - The id of the client context (context.h).
///
/// @return A pointer to the output context, or null if context is not yet
/// created/ready.
VIRCADIA_CLIENT_DYN_API
uint8_t* vircadia_get_audio_output_context(int context_id);

/// @brief Set the audio input data.
///
/// Sets the audio input data to be sent to the mixer. The data is buffered
/// internally and sent out periodically on a different thread. This function
/// must be called with a valid audio input context
/// (vircadia_get_audio_input_context()), and data of specified audio format
/// (vircadia_set_audio_input_format()). This function can be called from a
/// different thread, to be used as a realtime audio callback.
///
/// @param audio_context - The audio context. Must not be null.
/// @param data - The audio data to set. Must not be null.
/// @param size - The size of data to be set in bytes. Must not be negative.
///
/// @return non-negative value on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_argument_invalid() \n
/// vircadia_error_audio_context_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_data(uint8_t* audio_context, const uint8_t* data, int size);

/// @brief Get the audio output data.
///
/// Retrieves the audio output data receives from the mixer. The data is
/// buffered internally as it arrives and must be periodically retrieved to not
/// exhaust the buffer. This function must be called with a valid audio output
/// context (vircadia_get_audio_output_context()), and data of specified audio
/// format (vircadia_set_audio_output_format()). This function can be called
/// from a different thread, to be used as a realtime audio callback.
///
/// @param audio_context - The audio context. Must not be null.
/// @param data - The audio data buffer to read into. Must not be null.
/// @param size - The maximum size of data to read. Must not be negative.
///
/// @return The number of bytes read, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_argument_invalid() \n
/// vircadia_error_audio_context_invalid()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_audio_output_data(uint8_t* audio_context, uint8_t* data, int size);

/// @brief Set the bounding box of the audio source.
///
/// @param context_id - The id of the client context (context.h).
/// @param bounds - The bounds to set.
///
/// @return non-negative value on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_audio_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_bounds(int context_id, vircadia_bounds_ bounds);

/// @brief Set the position and rotation of the audio source.
///
/// @param context_id - The id of the client context (context.h).
/// @param vantage - The position and rotation (the vantage point) to set.
///
/// @return non-negative value on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_audio_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_vantage(int context_id, vircadia_vantage_ vantage);

/// @brief Specify whether the audio input should be sent back to the client.
///
/// By default the input is not sent back.
///
/// @param context_id - The id of the client context (context.h).
/// @param enabled - 0 - not sent back, 1 - sent back.
///
/// @return non-negative value on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_audio_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_echo(int context_id, uint8_t enabled);

/// @brief Un/mutes the audio input.
///
/// By default the input not muted.
///
/// @param context_id - The id of the client context (context.h).
/// @param muted - 0 - not muted, 1 - muted.
///
/// @return non-negative value on success, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_audio_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_set_audio_input_muted(int context_id, uint8_t muted);

/// @brief Determines whether this client has been muted by the mixer.
///
/// @param context_id - The id of the client context (context.h).
///
/// @return 1 - muted, 0 - not muted, or a negative error code. \n
/// Possible error codes: \n
/// vircadia_error_context_invalid() \n
/// vircadia_error_context_loss() \n
/// vircadia_error_audio_disabled()
VIRCADIA_CLIENT_DYN_API
int vircadia_get_audio_input_muted_by_mixer(int context_id);

// TODO: API for injectors (AudioInjectorManager)
// TODO: API for soling, noise gate, noise reduction, gain, output buffer size (AudioPacketHandler)

#endif /* end of include guard */
