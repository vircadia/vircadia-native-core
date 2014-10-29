//
//  ViewFrustum.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 04/11/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Simple view frustum class.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ViewFrustum_h
#define hifi_ViewFrustum_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "AABox.h"
#include "AACube.h"
#include "Plane.h"
#include "OctreeConstants.h"
#include "OctreeProjectedPolygon.h"

const float DEFAULT_KEYHOLE_RADIUS = 3.0f;
const float DEFAULT_FIELD_OF_VIEW_DEGREES = 45.0f;
const float DEFAULT_REAL_WORLD_FIELD_OF_VIEW_DEGREES = 30.f;
const float DEFAULT_ASPECT_RATIO = 16.f/9.f;
const float DEFAULT_NEAR_CLIP = 0.08f;
const float DEFAULT_FAR_CLIP = TREE_SCALE;

class ViewFrustum {
public:
    // setters for camera attributes
    void setPosition(const glm::vec3& p) { _position = p; _positionVoxelScale = (p / (float)TREE_SCALE); }
    void setOrientation(const glm::quat& orientationAsQuaternion);

    // getters for camera attributes
    const glm::vec3& getPosition() const { return _position; }
    const glm::vec3& getPositionVoxelScale() const { return _positionVoxelScale; }
    const glm::quat& getOrientation() const { return _orientation; }
    const glm::vec3& getDirection() const { return _direction; }
    const glm::vec3& getUp() const { return _up; }
    const glm::vec3& getRight() const { return _right; }

    // setters for lens attributes
    void setOrthographic(bool orthographic) { _orthographic = orthographic; }
    void setWidth(float width) { _width = width; }
    void setHeight(float height) { _height = height; }
    void setFieldOfView(float f) { _fieldOfView = f; }
    void setAspectRatio(float a) { _aspectRatio = a; }
    void setNearClip(float n) { _nearClip = n; }
    void setFarClip(float f) { _farClip = f; }
    void setFocalLength(float length) { _focalLength = length; }
    void setEyeOffsetPosition(const glm::vec3& p) { _eyeOffsetPosition    = p; }
    void setEyeOffsetOrientation(const glm::quat& o) { _eyeOffsetOrientation = o; }

    // getters for lens attributes
    bool isOrthographic() const { return _orthographic; }
    float getWidth() const { return _width; }
    float getHeight() const { return _height; }
    float getFieldOfView() const { return _fieldOfView; }
    float getAspectRatio() const { return _aspectRatio; }
    float getNearClip() const { return _nearClip; }
    float getFarClip() const { return _farClip; }
    float getFocalLength() const { return _focalLength; }
    const glm::vec3& getEyeOffsetPosition() const { return _eyeOffsetPosition; }
    const glm::quat& getEyeOffsetOrientation() const { return _eyeOffsetOrientation; }

    const glm::vec3& getOffsetPosition() const { return _offsetPosition; }
    const glm::vec3& getOffsetDirection() const { return _offsetDirection; }
    const glm::vec3& getOffsetUp() const { return _offsetUp; }
    const glm::vec3& getOffsetRight() const { return _offsetRight; }

    const glm::vec3& getFarTopLeft() const { return _farTopLeft; }
    const glm::vec3& getFarTopRight() const { return _farTopRight; }
    const glm::vec3& getFarBottomLeft() const { return _farBottomLeft; }
    const glm::vec3& getFarBottomRight() const { return _farBottomRight; }

    const glm::vec3& getNearTopLeft() const { return _nearTopLeft; }
    const glm::vec3& getNearTopRight() const { return _nearTopRight; }
    const glm::vec3& getNearBottomLeft() const { return _nearBottomLeft; }
    const glm::vec3& getNearBottomRight() const { return _nearBottomRight; }

    // get/set for keyhole attribute
    void  setKeyholeRadius(float keyholdRadius) { _keyholeRadius = keyholdRadius; }
    float getKeyholeRadius() const { return _keyholeRadius; }

    void calculate();

    ViewFrustum();

    typedef enum {OUTSIDE, INTERSECT, INSIDE} location;

    ViewFrustum::location pointInFrustum(const glm::vec3& point) const;
    ViewFrustum::location sphereInFrustum(const glm::vec3& center, float radius) const;
    ViewFrustum::location cubeInFrustum(const AACube& cube) const;
    ViewFrustum::location boxInFrustum(const AABox& box) const;

    // some frustum comparisons
    bool matches(const ViewFrustum& compareTo, bool debug = false) const;
    bool matches(const ViewFrustum* compareTo, bool debug = false) const { return matches(*compareTo, debug); }

    bool isVerySimilar(const ViewFrustum& compareTo, bool debug = false) const;
    bool isVerySimilar(const ViewFrustum* compareTo, bool debug = false) const { return isVerySimilar(*compareTo, debug); }

    void computePickRay(float x, float y, glm::vec3& origin, glm::vec3& direction) const;

    void computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearValue, float& farValue,
                               glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const;

    void printDebugDetails() const;

    glm::vec2 projectPoint(glm::vec3 point, bool& pointInView) const;
    OctreeProjectedPolygon getProjectedPolygon(const AACube& box) const;
    void getFurthestPointFromCamera(const AACube& box, glm::vec3& furthestPoint) const;
    
    // assumes box is in voxel scale, not TREE_SCALE, will scale view frustum's position accordingly
    void getFurthestPointFromCameraVoxelScale(const AACube& box, glm::vec3& furthestPoint) const;

    float distanceToCamera(const glm::vec3& point) const;
    
private:
    // Used for keyhole calculations
    ViewFrustum::location pointInKeyhole(const glm::vec3& point) const;
    ViewFrustum::location sphereInKeyhole(const glm::vec3& center, float radius) const;
    ViewFrustum::location cubeInKeyhole(const AACube& cube) const;
    ViewFrustum::location boxInKeyhole(const AABox& box) const;

    void calculateOrthographic();

    // camera location/orientation attributes
    glm::vec3 _position; // the position in TREE_SCALE
    glm::vec3 _positionVoxelScale; // the position in voxel scale
    glm::quat _orientation;

    // calculated for orientation
    glm::vec3 _direction;
    glm::vec3 _up;
    glm::vec3 _right;

    // Lens attributes
    bool _orthographic;
    float _width;
    float _height;
    float _fieldOfView; // degrees
    float _aspectRatio;
    float _nearClip;
    float _farClip;
    float _focalLength;
    glm::vec3 _eyeOffsetPosition;
    glm::quat _eyeOffsetOrientation;

    // keyhole attributes
    float _keyholeRadius;
    AACube _keyholeBoundingCube;


    // Calculated values
    glm::vec3 _offsetPosition;
    glm::vec3 _offsetDirection;
    glm::vec3 _offsetUp;
    glm::vec3 _offsetRight;
    glm::vec3 _farTopLeft;
    glm::vec3 _farTopRight;
    glm::vec3 _farBottomLeft;
    glm::vec3 _farBottomRight;
    glm::vec3 _nearTopLeft;
    glm::vec3 _nearTopRight;
    glm::vec3 _nearBottomLeft;
    glm::vec3 _nearBottomRight;
    enum { TOP_PLANE = 0, BOTTOM_PLANE, LEFT_PLANE, RIGHT_PLANE, NEAR_PLANE, FAR_PLANE };
    ::Plane _planes[6]; // How will this be used?

    const char* debugPlaneName (int plane) const;

    // Used to project points
    glm::mat4 _ourModelViewProjectionMatrix;
};


#endif // hifi_ViewFrustum_h
