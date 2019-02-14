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
#include <vector>

#include <avatars-renderer/Avatar.h>
#include <workload/Space.h>

#include "InterfaceLogging.h"

class AvatarManager;
class AvatarMotionState;
class DetailedMotionState;

class OtherAvatar : public Avatar {
public:
    explicit OtherAvatar(QThread* thread);
    virtual ~OtherAvatar();

    enum BodyLOD {
        Sphere = 0,
        MultiSphereLow, // No finger joints
        MultiSphereHigh // All joints
    };

    virtual void instantiableAvatar() override { };
    virtual void createOrb() override;
    virtual void indicateLoadingStatus(LoadingStatus loadingStatus) override;
    void updateOrbPosition();
    void removeOrb();

    void setSpaceIndex(int32_t index);
    int32_t getSpaceIndex() const { return _spaceIndex; }
    void updateSpaceProxy(workload::Transaction& transaction) const;

    int parseDataFromBuffer(const QByteArray& buffer) override;

    bool isInPhysicsSimulation() const;
    void rebuildCollisionShape() override;

    void setWorkloadRegion(uint8_t region);
    bool shouldBeInPhysicsSimulation() const;
    bool needsPhysicsUpdate() const;

    btCollisionShape* createCollisionShape(int jointIndex, bool& isBound, std::vector<int>& boundJoints);
    DetailedMotionState* createMotionState(std::shared_ptr<OtherAvatar> avatar, int jointIndex);
    void createDetailedMotionStates(const std::shared_ptr<OtherAvatar>& avatar);
    std::vector<DetailedMotionState*>& getDetailedMotionStates() { return _detailedMotionStates; }
    void resetDetailedMotionStates();
    BodyLOD getBodyLOD() { return _bodyLOD; }
    void computeShapeLOD();

    void updateCollisionGroup(bool myAvatarCollide);
    bool getCollideWithOtherAvatars() const { return _collideWithOtherAvatars; } 

    void setCollisionWithOtherAvatarsFlags() override;

    void simulate(float deltaTime, bool inView) override;

    friend AvatarManager;

protected:
    void handleChangedAvatarEntityData();
    void updateAttachedAvatarEntities();
    void onAddAttachedAvatarEntity(const QUuid& id);
    void onRemoveAttachedAvatarEntity(const QUuid& id);

    std::vector<QUuid> _attachedAvatarEntities;
    QUuid _otherAvatarOrbMeshPlaceholderID;
    AvatarMotionState* _motionState { nullptr };
    std::vector<DetailedMotionState*> _detailedMotionStates;
    int32_t _spaceIndex { -1 };
    uint8_t _workloadRegion { workload::Region::INVALID };
    BodyLOD _bodyLOD { BodyLOD::Sphere };
    bool _needsReinsertion { false };
};

using OtherAvatarPointer = std::shared_ptr<OtherAvatar>;
using AvatarPointer = std::shared_ptr<Avatar>;

#endif  // hifi_OtherAvatar_h
