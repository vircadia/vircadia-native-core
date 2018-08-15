//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OtherAvatar_h
#define hifi_OtherAvatar_h

#include <memory>

#include <avatars-renderer/Avatar.h>
#include "ui/overlays/Overlays.h"
#include "ui/overlays/Sphere3DOverlay.h"
#include "InterfaceLogging.h"

#include <workload/Space.h>

class OtherAvatar : public Avatar {
public:
    explicit OtherAvatar(QThread* thread);
    virtual ~OtherAvatar();

    virtual void instantiableAvatar() override { };
    virtual void createOrb() override;
    void updateOrbPosition();
    void removeOrb();

    void setSpaceIndex(int32_t index);
    int32_t getSpaceIndex() const { return _spaceIndex; }
    void updateSpaceProxy(workload::Transaction& transaction) const;

protected:
    std::shared_ptr<Sphere3DOverlay> _otherAvatarOrbMeshPlaceholder { nullptr };
    OverlayID _otherAvatarOrbMeshPlaceholderID { UNKNOWN_OVERLAY_ID };
    int32_t _spaceIndex { -1 };
};

using OtherAvatarPointer = std::shared_ptr<OtherAvatar>;

#endif  // hifi_OtherAvatar_h
