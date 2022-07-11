//
//  error.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 27 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_ERROR_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_ERROR_H

#include "common.h"

/// @brief Only one context can exist at a time.
///
/// A limitation in the current library implantation, this error code is
/// returned if user attempts to create the second context.
///
/// @return -1
VIRCADIA_CLIENT_DYN_API int vircadia_error_context_exists();

/// @brief Context ID is invalid.
///
/// The user specified wrong context ID or the context with specified ID
/// was destroyed.
///
/// @return -2
VIRCADIA_CLIENT_DYN_API int vircadia_error_context_invalid();

/// @brief Context was lost.
///
/// The specified context was lost, this can happen due to some
/// unrecoverable internal implantation errors. A lost context still
/// must be destroyed, but cannot be otherwise used.
///
/// @return -3
VIRCADIA_CLIENT_DYN_API int vircadia_error_context_loss();

/// @brief Node index is invalid.
///
/// The Specified node index was out of range.
///
/// @return -4
VIRCADIA_CLIENT_DYN_API int vircadia_error_node_invalid();

/// @brief Message index is invalid.
///
/// Specified message index was out of range.
///
/// @return -5
VIRCADIA_CLIENT_DYN_API int vircadia_error_message_invalid();

/// @brief Message type is invalid.
///
/// @return -6
VIRCADIA_CLIENT_DYN_API int vircadia_error_message_type_invalid();

/// @brief Handling of messages of specified type is disabled.
///
/// Use vircadia_enable_messages() to enable messages of specified
/// type.
///
/// @return -7
VIRCADIA_CLIENT_DYN_API int vircadia_error_message_type_disabled();

/// @brief Writing network packet failed.
///
/// @return -8
VIRCADIA_CLIENT_DYN_API int vircadia_error_packet_write();

/// @brief Invalid function argument.
///
/// One or more arguments passed to the function were not in a required range.
///
/// @return -9
VIRCADIA_CLIENT_DYN_API int vircadia_error_argument_invalid();

/// @brief Audio Functionality is disabled.
///
/// Use vircadia_enable_audio() to enable audio.
///
/// @return Unique negative error code.
VIRCADIA_CLIENT_DYN_API int vircadia_error_audio_disabled();

/// @brief Invalid audio format specified.
///
/// Valid audio format must have non-zero positive frequency and
/// channel count, and one of sample types defined in
/// audio_constants.h. Additionally when passed to
/// vircadia_set_audio_input_format(), the channel count must not be
/// greater than two.
///
/// @return Unique negative error code.
VIRCADIA_CLIENT_DYN_API int vircadia_audio_format_invalid();

/// @brief Invalid audio context specified.
///
/// Audio context must be retrieved by polling
/// vircadia_get_audio_input_context()/vircadia_get_audio_output_context(),
/// after calling
/// vircadia_set_audio_input_format()/vircadia_set_audio_output_format().
///
/// @return Unique negative error code.
VIRCADIA_CLIENT_DYN_API int vircadia_audio_context_invalid();

/// @brief Invalid audio codec name specified.
///
/// Valid codec names are defined in audio_constants.h.
///
/// @return Unique negative error code.
VIRCADIA_CLIENT_DYN_API int vircadia_audio_codec_invalid();

#endif /* end of include guard */
