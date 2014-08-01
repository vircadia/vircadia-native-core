//
//  ContactPoint.h
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.07.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ContactPoint_h
#define hifi_ContactPoint_h

#include <QtGlobal>
#include <glm/glm.hpp>

#include "CollisionInfo.h"

class Shape;

class ContactPoint {
public:
    ContactPoint();
    ContactPoint(const CollisionInfo& collision, quint32 frame);

    virtual float enforce();
   
    void updateContact(const CollisionInfo& collision, quint32 frame);
    quint32 getLastFrame() const { return _lastFrame; }

    Shape* getShapeA() const { return _shapeA; }
    Shape* getShapeB() const { return _shapeB; }

protected:
    quint32 _lastFrame; // frame count of last update
    Shape* _shapeA;
    Shape* _shapeB;
    glm::vec3 _offsetA; // contact point relative to A's center
    glm::vec3 _offsetB; // contact point relative to B's center
    glm::vec3 _normal; // (points from A toward B)
};

#endif // hifi_ContactPoint_h
