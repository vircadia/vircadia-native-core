//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OtherAvatar_h
#define hifi_OtherAvatar_h

#include <avatars-renderer/Avatar.h>
#include "ui/overlays/Overlays.h"
#include "ui/overlays/Sphere3DOverlay.h"
#include "InterfaceLogging.h"

class OtherAvatar : public Avatar {
public:
    explicit OtherAvatar(QThread* thread);
    virtual ~OtherAvatar();

    virtual void instantiableAvatar() override { };
    virtual void createOrb() override;
    void updateOrbPosition();
    void removeOrb();

protected:
    std::shared_ptr<Sphere3DOverlay> _otherAvatarOrbMeshPlaceholder { nullptr };
    OverlayID _otherAvatarOrbMeshPlaceholderID { UNKNOWN_OVERLAY_ID };
};

#endif  // hifi_OtherAvatar_h
