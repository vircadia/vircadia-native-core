//
//  PhysicsEntity.h
//  libraries/physics/src
//
//  Created by Andrew Meadows 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PhysicsEntity_h
#define hifi_PhysicsEntity_h

#include <QVector>
#include <QSet>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <CollisionInfo.h>
#include <RayIntersectionInfo.h>

class PhysicsEntity {

public:
    PhysicsEntity();
    virtual ~PhysicsEntity();

    virtual void stepForward(float deltaTime) { }

    void setTranslation(const glm::vec3& translation);
    void setRotation(const glm::quat& rotation);

    const glm::vec3& getTranslation() const { return _translation; }
    const glm::quat& getRotation() const { return _rotation; }
    float getBoundingRadius() const { return _boundingRadius; }

protected:
    glm::vec3 _translation;
    glm::quat _rotation;
    float _boundingRadius;
};

#endif // hifi_PhysicsEntity_h
