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

