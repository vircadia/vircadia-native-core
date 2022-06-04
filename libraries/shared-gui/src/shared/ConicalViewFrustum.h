//
//  ConicalViewFrustum.h
//  libraries/shared-gui/src/shared
//
//  Created by Clement Brisset 4/26/18
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ConicalViewFrustum_h
#define hifi_ConicalViewFrustum_h

#include <vector>

#include <glm/glm.hpp>

#include <shared/ConicalViewFrustumData.h>

class AACube;
class AABox;
class ViewFrustum;
using ViewFrustums = std::vector<ViewFrustum>;

const float SQRT_TWO_OVER_TWO = 0.7071067811865f;

// ConicalViewFrustum is an approximation of a ViewFrustum for fast calculation of sort priority.
class ConicalViewFrustum {
public:
    ConicalViewFrustum() = default;
    ConicalViewFrustum(const ViewFrustum& viewFrustum) { set(viewFrustum); }

    void set(const ViewFrustum& viewFrustum);
    void calculate();

    const glm::vec3& getPosition() const { return _position; }
    const glm::vec3& getDirection() const { return _direction; }
    float getAngle() const { return _angle; }
    float getRadius() const { return _radius; }
    float getFarClip() const { return _farClip; }

    bool isVerySimilar(const ConicalViewFrustum& other) const;

    bool intersects(const AACube& cube) const;
    bool intersects(const AABox& box) const;
    float getAngularSize(const AACube& cube) const;
    float getAngularSize(const AABox& box) const;

    bool intersects(const glm::vec3& relativePosition, float distance, float radius) const;
    float getAngularSize(float distance, float radius) const;

    int serialize(unsigned char* destinationBuffer) const;
    int deserialize(const unsigned char* sourceBuffer);

    // Just test for within radius.
    void setPositionAndSimpleRadius(const glm::vec3& position, float radius);

private:
    ConicalViewFrustumData _data {};

    float _sinAngle { SQRT_TWO_OVER_TWO };
    float _cosAngle { SQRT_TWO_OVER_TWO };
};
using ConicalViewFrustums = std::vector<ConicalViewFrustum>;


#endif /* hifi_ConicalViewFrustum_h */
