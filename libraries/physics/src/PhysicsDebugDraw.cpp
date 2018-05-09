//
//  PhysicsDebugDraw.cpp
//  libraries/physics/src
//
//  Created by Anthony Thibault 2018-4-18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicsDebugDraw.h"
#include "BulletUtil.h"
#include "PhysicsLogging.h"

#include <DebugDraw.h>
#include <GLMHelpers.h>

void PhysicsDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color) {
    DebugDraw::getInstance().drawRay(bulletToGLM(from), bulletToGLM(to), glm::vec4(color.getX(), color.getY(), color.getZ(), 1.0f));
}

void PhysicsDebugDraw::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance,
                                        int lifeTime, const btVector3& color) {

    glm::vec3 normal, tangent, biNormal;
    generateBasisVectors(bulletToGLM(normalOnB), Vectors::UNIT_X, normal, tangent, biNormal);
    btVector3 u = glmToBullet(normal);
    btVector3 v = glmToBullet(tangent);
    btVector3 w = glmToBullet(biNormal);

    // x marks the spot, green is along the normal.
    const float CONTACT_POINT_RADIUS = 0.1f;
    const btVector3 GREEN(0.0f, 1.0f, 0.0f);
    const btVector3 WHITE(1.0f, 1.0f, 1.0f);
    drawLine(PointOnB - u * CONTACT_POINT_RADIUS, PointOnB + u * CONTACT_POINT_RADIUS, GREEN);
    drawLine(PointOnB - v * CONTACT_POINT_RADIUS, PointOnB + v * CONTACT_POINT_RADIUS, WHITE);
    drawLine(PointOnB - w * CONTACT_POINT_RADIUS, PointOnB + w * CONTACT_POINT_RADIUS, WHITE);
}

void PhysicsDebugDraw::reportErrorWarning(const char* warningString) {
    qCWarning(physics) << "BULLET:" << warningString;
}

void PhysicsDebugDraw::draw3dText(const btVector3& location, const char* textString) {
}

void PhysicsDebugDraw::setDebugMode(int debugMode) {
    _debugDrawMode = debugMode;
}

int PhysicsDebugDraw::getDebugMode() const {
    return _debugDrawMode;
}
