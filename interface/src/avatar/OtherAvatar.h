//
//  Created by amantly 2018.06.26
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OtherAvatar_h
#define hifi_OtherAvatar_h

#include <memory>

#include <avatars-renderer/Avatar.h>
#include <workload/Space.h>

#include "InterfaceLogging.h"
#include "ui/overlays/Overlays.h"
#include "ui/overlays/Sphere3DOverlay.h"

class AvatarManager;
class AvatarMotionState;

class OtherAvatar : public Avatar {
public:
    explicit OtherAvatar(QThread* thread);
    virtual ~OtherAvatar();

    virtual void instantiableAvatar() override { };
    virtual void createOrb() override;
    virtual void indicateLoadingStatus(LoadingStatus loadingStatus) override;
    void updateOrbPosition();
    void removeOrb();

    void setSpaceIndex(int32_t index);
    int32_t getSpaceIndex() const { return _spaceIndex; }
    void updateSpaceProxy(workload::Transaction& transaction) const;

    int parseDataFromBuffer(const QByteArray& buffer) override;

    bool isInPhysicsSimulation() const { return _motionState != nullptr; }
    void rebuildCollisionShape() override;

    void setWorkloadRegion(uint8_t region);
    bool shouldBeInPhysicsSimulation() const;

    friend AvatarManager;

protected:
    std::shared_ptr<Sphere3DOverlay> _otherAvatarOrbMeshPlaceholder { nullptr };
    OverlayID _otherAvatarOrbMeshPlaceholderID { UNKNOWN_OVERLAY_ID };
    AvatarMotionState* _motionState { nullptr };
    int32_t _spaceIndex { -1 };
    uint8_t _workloadRegion { workload::Region::INVALID };
};

using OtherAvatarPointer = std::shared_ptr<OtherAvatar>;

#endif  // hifi_OtherAvatar_h
