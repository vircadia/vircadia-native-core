//
//  error.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 27 March 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "error.h"

#include "internal/Error.h"

using namespace vircadia::client;

VIRCADIA_CLIENT_DYN_API int vircadia_error_context_exists() {
    return toInt(ErrorCode::CONTEXT_EXISTS);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_context_invalid() {
    return toInt(ErrorCode::CONTEXT_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_context_loss() {
    return toInt(ErrorCode::CONTEXT_LOSS);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_node_invalid() {
    return toInt(ErrorCode::NODE_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_message_invalid() {
    return toInt(ErrorCode::MESSAGE_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_message_type_invalid() {
    return toInt(ErrorCode::MESSAGE_TYPE_INVALID);
}

VIRCADIA_CLIENT_DYN_API int vircadia_error_message_type_disabled() {
    return toInt(ErrorCode::MESSAGE_TYPE_DISABLED);
}
