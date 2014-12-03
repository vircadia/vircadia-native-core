//
//  Shape.h
//  libraries/physics/src
//
//  Created by Andrew Meadows on 2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Shape_h
#define hifi_Shape_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QDebug>
#include <QtGlobal>
#include <QVector>

#include "RayIntersectionInfo.h"

class PhysicsEntity;
class VerletPoint;

const float MAX_SHAPE_MASS = 1.0e18f; // something less than sqrt(FLT_MAX)

// DANGER: until ShapeCollider goes away the order of these values matter.  Specifically, 
// UNKNOWN_SHAPE must be equal to the number of shapes that ShapeCollider actually supports.
const quint8 SPHERE_SHAPE = 0;
const quint8 CAPSULE_SHAPE = 1;
const quint8 PLANE_SHAPE = 2;
const quint8 AACUBE_SHAPE = 3;
const quint8 LIST_SHAPE = 4;
const quint8 UNKNOWN_SHAPE = 5;
const quint8 INVALID_SHAPE = 5;

// new shapes to be supported by Bullet
const quint8 BOX_SHAPE = 7;
const quint8 CYLINDER_SHAPE = 8;

class Shape {
public:

    typedef quint8 Type;

    static quint32 getNextID() { static quint32 nextID = 0; return ++nextID; }

    Shape() : _type(UNKNOWN_SHAPE), _owningEntity(NULL), _boundingRadius(0.0f), 
            _translation(0.0f), _rotation(), _mass(MAX_SHAPE_MASS) {
        _id = getNextID();
    }
    virtual ~Shape() { }

    Type getType() const { return _type; }
    quint32 getID() const { return _id; }

    void setEntity(PhysicsEntity* entity) { _owningEntity = entity; }
    PhysicsEntity* getEntity() const { return _owningEntity; }

    float getBoundingRadius() const { return _boundingRadius; }

    virtual const glm::quat& getRotation() const { return _rotation; }
    virtual void setRotation(const glm::quat& rotation) { _rotation = rotation; }

    virtual void setTranslation(const glm::vec3& translation) { _translation = translation; }
    virtual const glm::vec3& getTranslation() const { return _translation; }

    virtual void setMass(float mass) { _mass = mass; }
    virtual float getMass() const { return _mass; }

    virtual bool findRayIntersection(RayIntersectionInfo& intersection) const = 0;

    /// \param penetration of collision
    /// \param contactPoint of collision
    /// \return the effective mass for the collision
    /// For most shapes has side effects: computes and caches the partial Lagrangian coefficients which will be
    /// used in the next accumulateDelta() call.
    virtual float computeEffectiveMass(const glm::vec3& penetration, const glm::vec3& contactPoint) { return _mass; }

    /// \param relativeMassFactor the final ingredient for partial Lagrangian coefficients from computeEffectiveMass()
    /// \param penetration the delta movement
    virtual void accumulateDelta(float relativeMassFactor, const glm::vec3& penetration) {}

    virtual void applyAccumulatedDelta() {}

    /// \return volume of shape in cubic meters
    virtual float getVolume() const { return 1.0; }

    virtual void getVerletPoints(QVector<VerletPoint*>& points) {}
    
    virtual QDebug& dumpToDebug(QDebug& debugConext) const;

protected:
    // these ctors are protected (used by derived classes only)
    Shape(Type type) : _type(type), _owningEntity(NULL), 
            _boundingRadius(0.0f), _translation(0.0f), 
            _rotation(), _mass(MAX_SHAPE_MASS) {
        _id = getNextID();
    }

    Shape(Type type, const glm::vec3& position) : 
            _type(type), _owningEntity(NULL), 
            _boundingRadius(0.0f), _translation(position), 
            _rotation(), _mass(MAX_SHAPE_MASS) {
        _id = getNextID();
    }

    Shape(Type type, const glm::vec3& position, const glm::quat& rotation) : _type(type), _owningEntity(NULL), 
            _boundingRadius(0.0f), _translation(position), 
            _rotation(rotation), _mass(MAX_SHAPE_MASS) {
        _id = getNextID();
    }

    void setBoundingRadius(float radius) { _boundingRadius = radius; }

    Type _type;
    quint32 _id;
    PhysicsEntity* _owningEntity;
    float _boundingRadius;
    glm::vec3 _translation;
    glm::quat _rotation;
    float _mass;
};

inline QDebug& Shape::dumpToDebug(QDebug& debugConext) const {
    debugConext << "Shape[ (" 
            << "type: " << getType()
            << "position: "
            << getTranslation().x << ", " << getTranslation().y << ", " << getTranslation().z
            << "radius: "
            << getBoundingRadius()
            << "]";

    return debugConext;
}

inline QDebug operator<<(QDebug debug, const Shape& shape) {
    return shape.dumpToDebug(debug);
}

inline QDebug operator<<(QDebug debug, const Shape* shape) {
    return shape->dumpToDebug(debug);
}


#endif // hifi_Shape_h
