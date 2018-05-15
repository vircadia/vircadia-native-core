//
//  PhysicsDebugDraw.h
//  libraries/physics/src
//
//  Created by Anthony Thibault 2018-4-18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtIDebugDraw.html

#ifndef hifi_PhysicsDebugDraw_h
#define hifi_PhysicsDebugDraw_h

#include <stdint.h>
#include <LinearMath/btIDebugDraw.h>

class PhysicsDebugDraw : public btIDebugDraw {
public:
    using btIDebugDraw::drawLine;
    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
    virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance,
                                  int lifeTime, const btVector3& color) override;
    virtual void reportErrorWarning(const char* warningString) override;
    virtual void draw3dText(const btVector3& location, const char* textString) override;
    virtual void setDebugMode(int debugMode) override;
    virtual int getDebugMode() const override;

protected:
    uint32_t _debugDrawMode;
};

#endif // hifi_PhysicsDebugDraw_h
