//
//  messages.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 25 March 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "messages.h"

#include "internal/Error.h"
#include "internal/Context.h"

using namespace vircadia::client;

int validMessageType(uint8_t type) {
    if ((type & ~MESSAGE_TYPE_ANY) != 0)
        return toInt(ErrorCode::MESSAGE_TYPE_INVALID);
    return 0;
}

int singularMessageType(uint8_t type) {
    if (std::bitset<8>(type).count() != 1)
        return toInt(ErrorCode::MESSAGE_TYPE_INVALID);
    return 0;
}

int messageTypesEnabled(const Context& context, uint8_t type) {
    if (context.messages().isEnabled(type)) {
        return 0;
    } else {
        return toInt(ErrorCode::MESSAGE_TYPE_DISABLED);
    }
}

template <typename F>
auto validateType(int context_id, uint8_t type, bool singular, F&& f) {
    return chain({
        checkContextReady(context_id),
        validMessageType(type),
        singular ? singularMessageType(type) : 0,
    }, [&](auto) {
        auto context = std::next(std::begin(contexts), context_id);
        return chain(messageTypesEnabled(*context, type), [&](auto) {
            return std::invoke(std::forward<F>(f), *context);
        });
    });
}

template <typename F>
auto validateTypes(int context_id, uint8_t type, F&& f) {
    return validateType(context_id, type, false, std::forward<F>(f));
}

template <typename F>
auto validateSingleType(int context_id, uint8_t type, F&& f) {
    return validateType(context_id, type, true, std::forward<F>(f));
}

VIRCADIA_CLIENT_DYN_API
int vircadia_enable_messages(int context_id, uint8_t type) {
    return chain({
        checkContextReady(context_id),
        validMessageType(type)
    }, [&](auto) {

        std::next(std::begin(contexts), context_id)->messages().enable(type);

        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_messages_subscribe(int context_id, const char* channel) {
    return chain(checkContextReady(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->messages().subscribe(channel);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_messages_unsubscribe(int context_id, const char* channel) {
    return chain(checkContextReady(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->messages().unsubscribe(channel);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_update_messages(int context_id, uint8_t type) {
    return validateTypes(context_id, type, [&](auto& context) {
        context.messages().update(type);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_messages_count(int context_id, uint8_t type) {
    return validateSingleType(context_id, type, [&](auto& context) -> int {
        return context.messages().get(type).size();
    });
}


template <typename F>
auto validateMessageIndex(int context_id, uint8_t type, int index, F&& f) {
    return validateSingleType(context_id, type, [&](auto& context) {
        const auto& messages = context.messages().get(type);
        return chain(checkIndexValid(messages, index, ErrorCode::MESSAGE_INVALID), [&](auto) {
            return std::invoke(std::forward<F>(f), messages[index]);
        });
    });
}

VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_message(int context_id, uint8_t type, int index) {
    return validateMessageIndex(context_id, type, index, [&](auto& message) {
        return message.payload.c_str();
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_get_message_size(int context_id, uint8_t type, int index) {
    return validateMessageIndex(context_id, type, index, [&](auto& message) -> int {
        return message.payload.size();
    });
}


VIRCADIA_CLIENT_DYN_API
const char* vircadia_get_message_channel(int context_id, uint8_t type, int index) {
    return validateMessageIndex(context_id, type, index, [&](auto& message) {
        return message.channel.c_str();
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_is_message_local_only(int context_id, uint8_t type, int index) {
    return validateMessageIndex(context_id, type, index, [&](auto& message) {
        return message.localOnly ? 1 : 0;
    });
}

VIRCADIA_CLIENT_DYN_API
const uint8_t* vircadia_get_message_sender(int context_id, uint8_t type, int index) {
    return validateMessageIndex(context_id, type, index, [&](auto& message) {
        return message.sender.data();
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_clear_messages(int context_id, uint8_t type) {
    return validateTypes(context_id, type, [&](auto& context) {
        context.messages().clear(type);
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_send_message(int context_id, uint8_t type, const char* channel, const char* payload, int size, uint8_t local) {
    return validateTypes(context_id, type, [&](auto& context) {
        if (channel == nullptr || payload == nullptr) {
            return toInt(ErrorCode::ARGUMENT_INVALID);
        }
        // validate the `local` parameter to be 0 or 1?
        // validate size to be >= 0 or -1 for null terminated payload?
        int sent = context.messages().send(type, channel, QByteArray(payload, size), local == 1 ? true : false);
        if (sent < 0) {
            return toInt(ErrorCode::PACKET_WRITE);
        }
        return 0;
    });
}

