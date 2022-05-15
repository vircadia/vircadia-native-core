//
//  avatars.cpp
//  libraries/vircadia-client/src
//
//  Created by Nshan G. on 16 May 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "avatars.h"


#include "internal/Error.h"
#include "internal/Context.h"
#include "internal/Avatars.h"
#include "internal/avatars/AvatarData.h"

using namespace vircadia::client;

VIRCADIA_CLIENT_DYN_API
int vircadia_enable_avatars(int context_id) {
    return chain(checkContextReady(context_id), [&](auto) {
        std::next(std::begin(contexts), context_id)->avatars().enable();
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_update_avatars(int context_id) {
    return chain(checkContextReady(context_id), [&](auto) {
        // TODO: check is enabled
        std::next(std::begin(contexts), context_id)->avatars().update();
        return 0;
    });
}

VIRCADIA_CLIENT_DYN_API
int vircadia_set_my_avatar_display_name(int context_id, const char* display_name) {
    return chain(checkContextReady(context_id), [&](auto) {
        // TODO: check is enabled
        // TODO: only change display name
        AvatarDataPacket::Identity identity{};
        identity.displayName = display_name;
        std::next(std::begin(contexts), context_id)->avatars().myAvatar().setProperty<AvatarData::IdentityIndex>(identity);
        return 0;
    });
}

