//
//  message_types.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 29 March 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "message_types.h"

#include "internal/Context.h"

using namespace vircadia::client;

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_text_messages() {
    return MESSAGE_TYPE_TEXT;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_data_messages() {
    return MESSAGE_TYPE_DATA;
}

VIRCADIA_CLIENT_DYN_API uint8_t vircadia_any_messages() {
    return MESSAGE_TYPE_ANY;
}
