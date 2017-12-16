//
//  ViewFrustum.h
//  libraries/shared/src
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
#include "CubeProjectedPolygon.h"
#include "Plane.h"
#include "RegisteredMetaTypes.h"
#include "Transform.h"

const int NUM_FRUSTUM_CORNERS = 8;
const int NUM_FRUSTUM_PLANES = 6;

const float DEFAULT_CENTER_SPHERE_RADIUS = 3.0f;
const float DEFAULT_FIELD_OF_VIEW_DEGREES = 45.0f;
const float DEFAULT_ASPECT_RATIO = 16.0f/9.0f;
const float DEFAULT_NEAR_CLIP = 0.08f;
const float DEFAULT_FAR_CLIP = 16384.0f;

class ViewFrustum {
public:
    // setters for camera attributes
    void setPosition(const glm::vec3& position);
    void setOrientation(const glm::quat& orientation);

    // getters for camera attributes
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getOrientation() const { return _orientation; }
    const glm::vec3& getDirection() const { return _direction; }
    const glm::vec3& getUp() const { return _up; }
    const glm::vec3& getRight() const { return _right; }

    // setters for lens attributes
    void setProjection(const glm::mat4 & projection);
    void setFocalLength(float focalLength) { _focalLength = focalLength; }

    // getters for lens attributes
    const glm::mat4& getProjection() const { return _projection; }
    const glm::mat4& getView() const { return _view; }
    float getWidth() const { return _width; }
    float getHeight() const { return _height; }
    float getFieldOfView() const { return _fieldOfView; }
    float getAspectRatio() const { return _aspectRatio; }
    float getNearClip() const { return _nearClip; }
    float getFarClip() const { return _farClip; }
    float getFocalLength() const { return _focalLength; }

    class Corners {
    public:
        Corners(glm::vec3&& topLeft, glm::vec3&& topRight, glm::vec3&& bottomLeft, glm::vec3&& bottomRight)
            : topLeft{ topLeft }, topRight{ topRight }, bottomLeft{ bottomLeft }, bottomRight{ bottomRight } {}
        glm::vec3 topLeft;
        glm::vec3 topRight;
        glm::vec3 bottomLeft;
        glm::vec3 bottomRight;
    // Get the corners depth units from frustum position, along frustum orientation
    };
    const Corners getCorners(const float depth) const;

    // getters for corners
    const glm::vec3& getFarTopLeft() const { return _cornersWorld[TOP_LEFT_FAR]; }
    const glm::vec3& getFarTopRight() const { return _cornersWorld[TOP_RIGHT_FAR]; }
    const glm::vec3& getFarBottomLeft() const { return _cornersWorld[BOTTOM_LEFT_FAR]; }
    const glm::vec3& getFarBottomRight() const { return _cornersWorld[BOTTOM_RIGHT_FAR]; }
    const glm::vec3& getNearTopLeft() const { return _cornersWorld[TOP_LEFT_NEAR]; }
    const glm::vec3& getNearTopRight() const { return _cornersWorld[TOP_RIGHT_NEAR]; }
    const glm::vec3& getNearBottomLeft() const { return _cornersWorld[BOTTOM_LEFT_NEAR]; }
    const glm::vec3& getNearBottomRight() const { return _cornersWorld[BOTTOM_RIGHT_NEAR]; }

    // get/set for central spherek attribute
    void  setCenterRadius(float radius) { _centerSphereRadius = radius; }
    float getCenterRadius() const { return _centerSphereRadius; }

    void tesselateSides(Triangle triangles[8]) const;
    void tesselateSides(const Transform& transform, Triangle triangles[8]) const;
    void tesselateSidesAndFar(const Transform& transform, Triangle triangles[10], float farDistance) const;

    void calculate();

    typedef enum { OUTSIDE = 0, INTERSECT, INSIDE } intersection;

    /// @return INSIDE, INTERSECT, or OUTSIDE depending on how cube intersects the keyhole shape
    ViewFrustum::intersection calculateCubeFrustumIntersection(const AACube& cube) const;
    ViewFrustum::intersection calculateCubeKeyholeIntersection(const AACube& cube) const;

    bool pointIntersectsFrustum(const glm::vec3& point) const;
    bool sphereIntersectsFrustum(const glm::vec3& center, float radius) const;
    bool cubeIntersectsFrustum(const AACube& box) const;
    bool boxIntersectsFrustum(const AABox& box) const;

    bool sphereIntersectsKeyhole(const glm::vec3& center, float radius) const;
    bool cubeIntersectsKeyhole(const AACube& cube) const;
    bool boxIntersectsKeyhole(const AABox& box) const;

    bool isVerySimilar(const ViewFrustum& compareTo) const;

    PickRay computePickRay(float x, float y);
    void computePickRay(float x, float y, glm::vec3& origin, glm::vec3& direction) const;

    void computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearValue, float& farValue,
                               glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const;

    void printDebugDetails() const;

    glm::vec2 projectPoint(glm::vec3 point, bool& pointInView) const;
    CubeProjectedPolygon getProjectedPolygon(const AACube& box) const;
    CubeProjectedPolygon getProjectedPolygon(const AABox& box) const;
    bool getProjectedRect(const AABox& box, glm::vec2& bottomLeft, glm::vec2& topRight) const;
    void getFurthestPointFromCamera(const AACube& box, glm::vec3& furthestPoint) const;

    float distanceToCamera(const glm::vec3& point) const;

    void evalProjectionMatrix(glm::mat4& proj) const;

    glm::mat4 evalProjectionMatrixRange(float rangeNear, float rangeFar) const;

    void evalViewTransform(Transform& view) const;

    enum PlaneIndex { TOP_PLANE = 0, BOTTOM_PLANE, LEFT_PLANE, RIGHT_PLANE, NEAR_PLANE, FAR_PLANE, NUM_PLANES };

    const ::Plane* getPlanes() const { return _planes; }
    void getSidePlanes(::Plane planes[4]) const;
    // Transform can have a different scale value in X,Y,Z components
    void getTransformedSidePlanes(const Transform& transform, ::Plane planes[4]) const;
    // Transform is assumed to have the same scale value in all three X,Y,Z components, which
    // allows for a faster computation.
    void getUniformlyTransformedSidePlanes(const Transform& transform, ::Plane planes[4]) const;

    void invalidate(); // causes all reasonable intersection tests to fail

    QByteArray toByteArray();
    void fromByteArray(const QByteArray& input);

private:
    glm::mat4 _view;
    glm::mat4 _projection;

    ::Plane _planes[NUM_FRUSTUM_PLANES]; // plane normals point inside frustum

    glm::vec3 _position; // position in world-frame
    glm::quat _orientation; // orientation in world-frame

    // calculated from orientation
    glm::vec3 _direction = IDENTITY_FORWARD;
    glm::vec3 _up = IDENTITY_UP;
    glm::vec3 _right = IDENTITY_RIGHT;

    // calculated from projection
    glm::vec4 _corners[NUM_FRUSTUM_CORNERS];
    glm::vec3 _cornersWorld[NUM_FRUSTUM_CORNERS];
    float _centerSphereRadius = DEFAULT_CENTER_SPHERE_RADIUS;
    float _width { 1.0f };
    float _height { 1.0f };
    float _aspectRatio { 1.0f };
    float _focalLength { 0.25f };
    float _fieldOfView { DEFAULT_FIELD_OF_VIEW_DEGREES };

    float _nearClip { DEFAULT_NEAR_CLIP };
    float _farClip { DEFAULT_FAR_CLIP };

    const char* debugPlaneName (int plane) const;

    // Used to project points
    glm::mat4 _ourModelViewProjectionMatrix;
    
    template <typename TBOX>
    CubeProjectedPolygon computeProjectedPolygon(const TBOX& box) const;

    static void tesselateSides(const glm::vec3 points[8], Triangle triangles[8]);

};
using ViewFrustumPointer = std::shared_ptr<ViewFrustum>;

#endif // hifi_ViewFrustum_h
