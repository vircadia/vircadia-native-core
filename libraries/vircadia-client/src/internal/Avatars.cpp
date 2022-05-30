//
//  Avatars.cpp
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 14 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Avatars.h"

#include <cassert>

#include "avatars/AvatarManager.h"

namespace vircadia::client
{

    void Avatars::enable() {
        if (!isEnabled()) {
            manager = std::make_unique<AvatarManager>();
        }
    }

    void Avatars::update() {
        assert(isEnabled());
        manager->updateData();
    }

    void Avatars::updateManager() {
        assert(isEnabled());
        manager->update();
    }

    const std::vector<AvatarData>& Avatars::get() const {
        assert(isEnabled());
        return manager->getAvatarDataOut();
    }

    bool Avatars::isEnabled() const {
        return manager != nullptr;
    }

    void Avatars::destroy() {
        manager.reset();
    }

    AvatarData& Avatars::myAvatar() {
        assert(isEnabled());
        return manager->myAvatarDataIn;
    }

    const std::vector<AvatarData>& Avatars::all() const {
        assert(isEnabled());
        return manager->avatarDataOut;
    }

} // namespace vircadia::client
