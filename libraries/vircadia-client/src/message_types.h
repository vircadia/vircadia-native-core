//
//  message_types.h
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 29 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @file

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_MESSAGE_TYPES_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_MESSAGE_TYPES_H

#include <stdint.h>

#include "common.h"

/// @brief Flag value indicating text message type.
///
/// @return 0b1
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_text_messages();

/// @brief Flag value indicating data message type.
///
/// @return 0b10
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_data_messages();

/// @brief Flag value indicating any message type.
///
/// @return vircadia_text_messages() | vircadia_data_messages()
VIRCADIA_CLIENT_DYN_API uint8_t vircadia_any_messages();

#endif /* end of include guard */
