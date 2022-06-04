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
            DependencyManager::set<AvatarManager>();
        }
    }

    void Avatars::update() {
        assert(isEnabled());
        DependencyManager::get<AvatarManager>()->updateData();
    }

    void Avatars::updateManager() {
        assert(isEnabled());
        DependencyManager::get<AvatarManager>()->update();
    }

    bool Avatars::isEnabled() const {
        return DependencyManager::isSet<AvatarManager>();
    }

    void Avatars::destroy() {
        DependencyManager::destroy<AvatarManager>();
    }

    AvatarData& Avatars::myAvatar() {
        assert(isEnabled());
        return DependencyManager::get<AvatarManager>()->myAvatarDataIn;
    }

    const std::vector<AvatarData>& Avatars::all() const {
        assert(isEnabled());
        return DependencyManager::get<AvatarManager>()->avatarDataOut;
    }

    std::vector<vircadia_conical_view_frustum>& Avatars::views() {
        assert(isEnabled());
        return DependencyManager::get<AvatarManager>()->viewsIn;
    }

} // namespace vircadia::client
