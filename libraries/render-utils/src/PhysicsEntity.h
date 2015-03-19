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

class Shape;

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

    void setShapeBackPointers();

    void setEnableShapes(bool enable);

    virtual void buildShapes() = 0;
    virtual void clearShapes();
    const QVector<Shape*> getShapes() const { return _shapes; }

    bool findRayIntersection(RayIntersectionInfo& intersection) const;
    bool findCollisions(const QVector<const Shape*> shapes, CollisionList& collisions);
    bool findSphereCollisions(const glm::vec3& sphereCenter, float sphereRadius, CollisionList& collisions);
    bool findPlaneCollisions(const glm::vec4& plane, CollisionList& collisions);

    void disableCollisions(int shapeIndexA, int shapeIndexB);
    bool collisionsAreEnabled(int shapeIndexA, int shapeIndexB) const;

protected:
    glm::vec3 _translation;
    glm::quat _rotation;
    float _boundingRadius;
    bool _shapesAreDirty;
    bool _enableShapes;
    QVector<Shape*> _shapes;
    QSet<int> _disabledCollisions;
};

#endif // hifi_PhysicsEntity_h
