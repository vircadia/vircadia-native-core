//
//  MyCharacterController.h
//  interface/src/avatar
//
//  Created by AndrewMeadows 2015.10.21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_MyCharacterController_h
#define hifi_MyCharacterController_h

#include <CharacterController.h>
//#include <SharedUtil.h>
#include <PhysicsEngine.h>

class btCollisionShape;
class MyAvatar;
class DetailedMotionState;

class MyCharacterController : public CharacterController {
public:
    explicit MyCharacterController(std::shared_ptr<MyAvatar> avatar, const FollowTimePerType& followTimeRemainingPerType);
    ~MyCharacterController ();

    void addToWorld() override;
    void updateShapeIfNecessary() override;

    // Sweeping a convex shape through the physics simulation can be expensive when the obstacles are too
    // complex (e.g. small 20k triangle static mesh) so instead we cast several rays forward and if they
    // don't hit anything we consider it a clean sweep.  Hence this "Shotgun" code.
    class RayShotgunResult {
    public:
        void reset();
        float hitFraction { 1.0f };
        bool walkable { true };
    };

    /// return true if RayShotgun hits anything
    bool testRayShotgun(const glm::vec3& position, const glm::vec3& step, RayShotgunResult& result);

    void setDensity(btScalar density) { _density = density; }

    const btCollisionShape* createDetailedCollisionShapeForJoint(int32_t jointIndex);
    DetailedMotionState* createDetailedMotionStateForJoint(int32_t jointIndex);
    std::vector<DetailedMotionState*>& getDetailedMotionStates() { return _detailedMotionStates; }
    void clearDetailedMotionStates();

    void buildPhysicsTransaction(PhysicsEngine::Transaction& transaction);

    struct RayAvatarResult {
        bool _intersect { false };
        bool _isBound { false };
        QUuid _intersectWithAvatar;
        int32_t _intersectWithJoint { -1 };
        float _distance { 0.0f };
        float _maxDistance { 0.0f };
        QVariantMap _extraInfo;
        glm::vec3 _intersectionPoint;
        glm::vec3 _intersectionNormal;
        std::vector<int32_t> _boundJoints;
    };
    std::vector<RayAvatarResult> rayTest(const btVector3& origin, const btVector3& direction, const btScalar& length,
                                         const QVector<uint>& jointsToExclude) const;

    int32_t computeCollisionMask() const override;
    void handleChangedCollisionMask() override;

    void setCollideWithOtherAvatars(bool collideWithOtherAvatars) { _collideWithOtherAvatars = collideWithOtherAvatars; }

    bool needsSafeLandingSupport() const;

protected:
    void initRayShotgun(const btCollisionWorld* world);
    void updateMassProperties() override;

private:
    btConvexHullShape* computeShape() const;

protected:
    std::shared_ptr<MyAvatar> _avatar { nullptr };

    // shotgun scan data
    btAlignedObjectArray<btVector3> _topPoints;
    btAlignedObjectArray<btVector3> _bottomPoints;
    btScalar _density { 1.0f };

    std::vector<DetailedMotionState*> _detailedMotionStates;
    bool _collideWithOtherAvatars { true };
};

#endif // hifi_MyCharacterController_h
