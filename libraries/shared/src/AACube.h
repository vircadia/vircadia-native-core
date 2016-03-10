//
//  AACube.h
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Originally from lighthouse3d. Modified to utilize glm::vec3 and clean up to our coding standards
//  Simple axis aligned box class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AACube_h
#define hifi_AACube_h

#include <glm/glm.hpp>

#include <QDebug>

#include "BoxBase.h"

class AABox;
class Extents;

class AACube {

public:
    AACube(const AABox& other);
    AACube(const Extents& other);
    AACube(const glm::vec3& corner, float size);
    AACube();
    ~AACube() {};

    void setBox(const glm::vec3& corner, float scale);
    glm::vec3 getFarthestVertex(const glm::vec3& normal) const; // return vertex most parallel to normal
    glm::vec3 getNearestVertex(const glm::vec3& normal) const; // return vertex most anti-parallel to normal
    void scale(float scale);
    const glm::vec3& getCorner() const { return _corner; }
    float getScale() const { return _scale; }
    glm::vec3 getDimensions() const { return glm::vec3(_scale,_scale,_scale); }
    float getLargestDimension() const { return _scale; }

    glm::vec3 calcCenter() const;
    glm::vec3 calcTopFarLeft() const;
    glm::vec3 getVertex(BoxVertex vertex) const;

    const glm::vec3& getMinimumPoint() const { return _corner; }
    glm::vec3 getMaximumPoint() const { return calcTopFarLeft(); }

    bool contains(const glm::vec3& point) const;
    bool contains(const AACube& otherCube) const;
    bool touches(const AACube& otherCube) const;
    bool contains(const AABox& otherBox) const;
    bool touches(const AABox& otherBox) const;
    bool expandedContains(const glm::vec3& point, float expansion) const;
    bool expandedIntersectsSegment(const glm::vec3& start, const glm::vec3& end, float expansion) const;
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                BoxFace& face, glm::vec3& surfaceNormal) const;
    bool touchesSphere(const glm::vec3& center, float radius) const;
    bool findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration) const;
    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration) const;

    AABox clamp(const glm::vec3& min, const glm::vec3& max) const;
    AABox clamp(float min, float max) const;

    AACube& operator += (const glm::vec3& point);

    bool containsNaN() const;

private:
    glm::vec3 getClosestPointOnFace(const glm::vec3& point, BoxFace face) const;
    glm::vec3 getClosestPointOnFace(const glm::vec4& origin, const glm::vec4& direction, BoxFace face) const;
    glm::vec4 getPlane(BoxFace face) const;

    static BoxFace getOppositeFace(BoxFace face);

    glm::vec3 _corner;
    float _scale;
};

inline bool operator==(const AACube& a, const AACube& b) {
    return a.getCorner() == b.getCorner() && a.getScale() == b.getScale();
}

inline bool operator!=(const AACube& a, const AACube& b) {
    return a.getCorner() != b.getCorner() || a.getScale() != b.getScale();
}

inline QDebug operator<<(QDebug debug, const AACube& cube) {
    debug << "AACube[ ("
            << cube.getCorner().x << "," << cube.getCorner().y << "," << cube.getCorner().z << " ) to ("
            << cube.calcTopFarLeft().x << "," << cube.calcTopFarLeft().y << "," << cube.calcTopFarLeft().z << ") size: ("
            << cube.getDimensions().x << "," << cube.getDimensions().y << "," << cube.getDimensions().z << ")"
            << "]";
    return debug;
}


#endif // hifi_AACube_h
