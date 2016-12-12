//
//  AABox.h
//  libraries/octree/src
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

#ifndef hifi_AABox_h
#define hifi_AABox_h

#include <glm/glm.hpp>

#include <QDebug>

#include "BoxBase.h"
#include "StreamUtils.h"

class AACube;
class Extents;
class Transform;

class AABox {

public:
    AABox(const AACube& other);
    AABox(const Extents& other);
    AABox(const glm::vec3& corner, float size);
    AABox(const glm::vec3& corner, const glm::vec3& dimensions);
    AABox();
    ~AABox() {};

    void setBox(const glm::vec3& corner, const glm::vec3& scale);

    void setBox(const glm::vec3& corner, float scale);
    glm::vec3 getFarthestVertex(const glm::vec3& normal) const; // return vertex most parallel to normal
    glm::vec3 getNearestVertex(const glm::vec3& normal) const; // return vertex most anti-parallel to normal

    const glm::vec3& getCorner() const { return _corner; }
    const glm::vec3& getScale() const { return _scale; }
    const glm::vec3& getDimensions() const { return _scale; }
    float getLargestDimension() const { return glm::max(_scale.x, glm::max(_scale.y, _scale.z)); }

    glm::vec3 calcCenter() const;
    glm::vec3 calcTopFarLeft() const { return _corner + _scale; }

    const glm::vec3& getMinimum() const { return _corner; }
    glm::vec3 getMaximum() const { return _corner + _scale; }

    glm::vec3 getVertex(BoxVertex vertex) const;

    const glm::vec3& getMinimumPoint() const { return _corner; }
    glm::vec3 getMaximumPoint() const { return calcTopFarLeft(); }

    bool contains(const glm::vec3& point) const;
    bool contains(const AABox& otherBox) const;
    bool touches(const AABox& otherBox) const;

    bool contains(const AACube& otherCube) const;
    bool touches(const AACube& otherCube) const;

    bool expandedContains(const glm::vec3& point, float expansion) const;
    bool expandedIntersectsSegment(const glm::vec3& start, const glm::vec3& end, float expansion) const;
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance,
                                BoxFace& face, glm::vec3& surfaceNormal) const;
    bool touchesSphere(const glm::vec3& center, float radius) const; // fast but may generate false positives
    bool findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration) const;
    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration) const;

    bool isNull() const { return _scale == glm::vec3(0.0f, 0.0f, 0.0f); }

    AABox clamp(const glm::vec3& min, const glm::vec3& max) const;
    AABox clamp(float min, float max) const;

    AABox& operator += (const glm::vec3& point);
    AABox& operator += (const AABox& box);

    // Translate the AABox just moving the corner
    void translate(const glm::vec3& translation) { _corner += translation; }

    // Rotate the AABox around its frame origin
    // meaning rotating the corners of the AABox around the point {0,0,0} and reevaluating the min max
    void rotate(const glm::quat& rotation);

    /// Scale the AABox
    void scale(float scale);
    void scale(const glm::vec3& scale);

    /// make the AABox bigger (scale about it's center)
    void embiggen(float scale);
    void embiggen(const glm::vec3& scale);

    // Set a new scale for the Box, but keep it centered at its current location
    void setScaleStayCentered(const glm::vec3& scale);

    // Transform the extents with transform
    void transform(const Transform& transform);

    // Transform the extents with matrix
    void transform(const glm::mat4& matrix);

    static const glm::vec3 INFINITY_VECTOR;

    bool isInvalid() const { return _corner == INFINITY_VECTOR; }

private:
    glm::vec3 getClosestPointOnFace(const glm::vec3& point, BoxFace face) const;
    glm::vec3 getClosestPointOnFace(const glm::vec4& origin, const glm::vec4& direction, BoxFace face) const;
    glm::vec4 getPlane(BoxFace face) const;

    static BoxFace getOppositeFace(BoxFace face);

    glm::vec3 _corner;
    glm::vec3 _scale;
};

inline bool operator==(const AABox& a, const AABox& b) {
    return a.getCorner() == b.getCorner() && a.getDimensions() == b.getDimensions();
}

inline QDebug operator<<(QDebug debug, const AABox& box) {
    debug << "AABox[ ("
            << box.getCorner().x << "," << box.getCorner().y << "," << box.getCorner().z << " ) to ("
            << box.calcTopFarLeft().x << "," << box.calcTopFarLeft().y << "," << box.calcTopFarLeft().z << ") size: ("
            << box.getDimensions().x << "," << box.getDimensions().y << "," << box.getDimensions().z << ")"
            << "]";

    return debug;
}

#endif // hifi_AABox_h
