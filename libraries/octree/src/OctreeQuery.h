//
//  OctreeQuery.h
//  libraries/octree/src
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeQuery_h
#define hifi_OctreeQuery_h

/* VS2010 defines stdint.h, but not inttypes.h */
#if defined(_MSC_VER)
typedef signed char  int8_t;
typedef signed short int16_t;
typedef signed int   int32_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef signed long long   int64_t;
typedef unsigned long long quint64;
#define PRId64 "I64d"
#else
#include <inttypes.h>
#endif


#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <NodeData.h>

// First bitset
const int WANT_LOW_RES_MOVING_BIT = 0;
const int WANT_COLOR_AT_BIT = 1;
const int WANT_DELTA_AT_BIT = 2;
const int UNUSED_BIT_3 = 3; // unused... available for new feature
const int WANT_COMPRESSION = 4; // 5th bit

class OctreeQuery : public NodeData {
    Q_OBJECT

public:
    OctreeQuery();
    virtual ~OctreeQuery() {}

    int getBroadcastData(unsigned char* destinationBuffer);
    int parseData(ReceivedMessage& message) override;

    // getters for camera details
    const glm::vec3& getCameraPosition() const { return _cameraPosition; }
    const glm::quat& getCameraOrientation() const { return _cameraOrientation; }
    float getCameraFov() const { return _cameraFov; }
    float getCameraAspectRatio() const { return _cameraAspectRatio; }
    float getCameraNearClip() const { return _cameraNearClip; }
    float getCameraFarClip() const { return _cameraFarClip; }
    const glm::vec3& getCameraEyeOffsetPosition() const { return _cameraEyeOffsetPosition; }
    float getCameraCenterRadius() const { return _cameraCenterRadius; }

    glm::vec3 calculateCameraDirection() const;

    // setters for camera details
    void setCameraPosition(const glm::vec3& position) { _cameraPosition = position; }
    void setCameraOrientation(const glm::quat& orientation) { _cameraOrientation = orientation; }
    void setCameraFov(float fov) { _cameraFov = fov; }
    void setCameraAspectRatio(float aspectRatio) { _cameraAspectRatio = aspectRatio; }
    void setCameraNearClip(float nearClip) { _cameraNearClip = nearClip; }
    void setCameraFarClip(float farClip) { _cameraFarClip = farClip; }
    void setCameraEyeOffsetPosition(const glm::vec3& eyeOffsetPosition) { _cameraEyeOffsetPosition = eyeOffsetPosition; }
    void setCameraCenterRadius(float radius) { _cameraCenterRadius = radius; }

    // related to Octree Sending strategies
    int getMaxQueryPacketsPerSecond() const { return _maxQueryPPS; }
    float getOctreeSizeScale() const { return _octreeElementSizeScale; }
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }

public slots:
    void setMaxQueryPacketsPerSecond(int maxQueryPPS) { _maxQueryPPS = maxQueryPPS; }
    void setOctreeSizeScale(float octreeSizeScale) { _octreeElementSizeScale = octreeSizeScale; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust) { _boundaryLevelAdjust = boundaryLevelAdjust; }

protected:
    // camera details for the avatar
    glm::vec3 _cameraPosition = glm::vec3(0.0f);
    glm::quat _cameraOrientation = glm::quat();
    float _cameraFov = 0.0f;
    float _cameraAspectRatio = 1.0f;
    float _cameraNearClip = 0.0f;
    float _cameraFarClip = 0.0f;
    float _cameraCenterRadius { 0.0f };
    glm::vec3 _cameraEyeOffsetPosition = glm::vec3(0.0f);

    // octree server sending items
    int _maxQueryPPS = DEFAULT_MAX_OCTREE_PPS;
    float _octreeElementSizeScale = DEFAULT_OCTREE_SIZE_SCALE; /// used for LOD calculations
    int _boundaryLevelAdjust = 0; /// used for LOD calculations

private:
    // privatize the copy constructor and assignment operator so they cannot be called
    OctreeQuery(const OctreeQuery&);
    OctreeQuery& operator= (const OctreeQuery&);
};

#endif // hifi_OctreeQuery_h
