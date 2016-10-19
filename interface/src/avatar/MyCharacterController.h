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

class btCollisionShape;
class MyAvatar;

class MyCharacterController : public CharacterController {
public:
    explicit MyCharacterController(MyAvatar* avatar);
    ~MyCharacterController ();

    void setDynamicsWorld(btDynamicsWorld* world) override;
    void updateShapeIfNecessary() override;

    // Sweeping a convex shape through the physics simulation can expensive when the obstacles are too complex
    // (e.g. small 20k triangle static mesh) so instead as a fallback we cast several rays forward and if they
    // don't hit anything we consider it a clean sweep.  Hence the "Shotgun" code.
    class RayShotgunResult {
    public:
        void reset();

        //glm::vec3 hitNormal { 0.0f, 0.0f, 0.0f };
        float hitFraction { 1.0f };
        //int32_t numHits { 0 };
        bool walkable { true };
    };

    /// return true if RayShotgun hits anything
    bool testRayShotgun(const glm::vec3& position, const glm::vec3& step, RayShotgunResult& result);

protected:
    void initRayShotgun(const btCollisionWorld* world);

private:
    btConvexHullShape* computeShape() const;

protected:
    MyAvatar* _avatar { nullptr };

    // shotgun scan data
    btAlignedObjectArray<btVector3> _topPoints;
    btAlignedObjectArray<btVector3> _bottomPoints;
};

#endif // hifi_MyCharacterController_h
