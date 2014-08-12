//
//  AACube.h
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

#ifndef hifi_AACube_h
#define hifi_AACube_h

#include <glm/glm.hpp>

#include <QDebug>

#include "BoxBase.h"

class AABox;

class AACube {

public:
    AACube(const glm::vec3& corner, float size);
    AACube();
    ~AACube() {};

    void setBox(const glm::vec3& corner, float scale);
    glm::vec3 getVertexP(const glm::vec3& normal) const;
    glm::vec3 getVertexN(const glm::vec3& normal) const;
    void scale(float scale);
    const glm::vec3& getCorner() const { return _corner; }
    float getScale() const { return _scale; }
    glm::vec3 getDimensions() const { return glm::vec3(_scale,_scale,_scale); }

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
    bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction, float& distance, BoxFace& face) const;
    bool findSpherePenetration(const glm::vec3& center, float radius, glm::vec3& penetration) const;
    bool findCapsulePenetration(const glm::vec3& start, const glm::vec3& end, float radius, glm::vec3& penetration) const;

    AABox clamp(const glm::vec3& min, const glm::vec3& max) const;
    AABox clamp(float min, float max) const;

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

inline QDebug operator<<(QDebug debug, const AACube& cube) {
    const int TREE_SCALE = 16384; // ~10 miles.. This is the number of meters of the 0.0 to 1.0 voxel universe
    debug << "AACube[ (" 
            << cube.getCorner().x * (float)TREE_SCALE << "," << cube.getCorner().y * (float)TREE_SCALE << "," << cube.getCorner().z * (float)TREE_SCALE << " ) to ("
            << cube.calcTopFarLeft().x * (float)TREE_SCALE << "," << cube.calcTopFarLeft().y * (float)TREE_SCALE << "," << cube.calcTopFarLeft().z * (float)TREE_SCALE << ") size: ("
            << cube.getDimensions().x * (float)TREE_SCALE << "," << cube.getDimensions().y * (float)TREE_SCALE << "," << cube.getDimensions().z * (float)TREE_SCALE << ")"
            << " in meters]";
    return debug;
}


#endif // hifi_AACube_h
