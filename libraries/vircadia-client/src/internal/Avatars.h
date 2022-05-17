//
//  Avatars.h
//  libraries/vircadia-client/src/internal
//
//  Created by Nshan G. on 14 May 2022.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_H

#include <vector>
#include <memory>

#include "Common.h"

namespace vircadia::client {

    struct AvatarData;
    class AvatarManager;

    /// @private
    class Avatars {

    public:

        void enable();
        void update();
        void updateManager();
        const std::vector<AvatarData>& get() const;
        bool isEnabled() const;
        int set(const AvatarData&);

        void destroy();

        AvatarData& myAvatar();

    private:

        std::unique_ptr<AvatarManager> manager;
    };


} // namespace vircadia::client

#endif /* end of include guard */
