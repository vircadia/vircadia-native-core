//
//  OctreeQuery.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__OctreeQuery__
#define __hifi__OctreeQuery__

#include <string>
#include <inttypes.h>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QVariantMap>

#include <RegisteredMetaTypes.h>

#include <NodeData.h>

// First bitset
const int WANT_LOW_RES_MOVING_BIT = 0;
const int WANT_COLOR_AT_BIT = 1;
const int WANT_DELTA_AT_BIT = 2;
const int WANT_OCCLUSION_CULLING_BIT = 3;
const int WANT_COMPRESSION = 4; // 5th bit

class OctreeQuery : public NodeData {
    Q_OBJECT
    
public:
    OctreeQuery(Node* owningNode = NULL);
    virtual ~OctreeQuery();

    int getBroadcastData(unsigned char* destinationBuffer);
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    QUuid& getUUID() { return _uuid; }
    void setUUID(const QUuid& uuid) { _uuid = uuid; }
    
    // getters for camera details
    const glm::vec3& getCameraPosition() const { return _cameraPosition; }
    const glm::quat& getCameraOrientation() const { return _cameraOrientation; }
    float getCameraFov() const { return _cameraFov; }
    float getCameraAspectRatio() const { return _cameraAspectRatio; }
    float getCameraNearClip() const { return _cameraNearClip; }
    float getCameraFarClip() const { return _cameraFarClip; }
    const glm::vec3& getCameraEyeOffsetPosition() const { return _cameraEyeOffsetPosition; }

    glm::vec3 calculateCameraDirection() const;

    // setters for camera details    
    void setCameraPosition(const glm::vec3& position) { _cameraPosition = position; }
    void setCameraOrientation(const glm::quat& orientation) { _cameraOrientation = orientation; }
    void setCameraFov(float fov) { _cameraFov = fov; }
    void setCameraAspectRatio(float aspectRatio) { _cameraAspectRatio = aspectRatio; }
    void setCameraNearClip(float nearClip) { _cameraNearClip = nearClip; }
    void setCameraFarClip(float farClip) { _cameraFarClip = farClip; }
    void setCameraEyeOffsetPosition(const glm::vec3& eyeOffsetPosition) { _cameraEyeOffsetPosition = eyeOffsetPosition; }
    
    // related to Octree Sending strategies
    bool getWantColor() const { return _wantColor; }
    bool getWantDelta() const { return _wantDelta; }
    bool getWantLowResMoving() const { return _wantLowResMoving; }
    bool getWantOcclusionCulling() const { return _wantOcclusionCulling; }
    bool getWantCompression() const { return _wantCompression; }
    int getMaxOctreePacketsPerSecond() const { return _maxOctreePPS; }
    float getOctreeSizeScale() const { return _octreeElementSizeScale; }
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }
    
public slots:
    void setWantLowResMoving(bool wantLowResMoving) { _wantLowResMoving = wantLowResMoving; }
    void setWantColor(bool wantColor) { _wantColor = wantColor; }
    void setWantDelta(bool wantDelta) { _wantDelta = wantDelta; }
    void setWantOcclusionCulling(bool wantOcclusionCulling) { _wantOcclusionCulling = wantOcclusionCulling; }
    void setWantCompression(bool wantCompression) { _wantCompression = wantCompression; }
    void setMaxOctreePacketsPerSecond(int maxOctreePPS) { _maxOctreePPS = maxOctreePPS; }
    void setOctreeSizeScale(float octreeSizeScale) { _octreeElementSizeScale = octreeSizeScale; }
    void setBoundaryLevelAdjust(int boundaryLevelAdjust) { _boundaryLevelAdjust = boundaryLevelAdjust; }
    
protected:
    QUuid _uuid;
    
    // camera details for the avatar
    glm::vec3 _cameraPosition;
    glm::quat _cameraOrientation;
    float _cameraFov;
    float _cameraAspectRatio;
    float _cameraNearClip;
    float _cameraFarClip;
    glm::vec3 _cameraEyeOffsetPosition;
    
    // octree server sending items
    bool _wantColor;
    bool _wantDelta;
    bool _wantLowResMoving;
    bool _wantOcclusionCulling;
    bool _wantCompression;
    int _maxOctreePPS;
    float _octreeElementSizeScale; /// used for LOD calculations
    int _boundaryLevelAdjust; /// used for LOD calculations
    
private:
    // privatize the copy constructor and assignment operator so they cannot be called
    OctreeQuery(const OctreeQuery&);
    OctreeQuery& operator= (const OctreeQuery&);
};

#endif /* defined(__hifi__OctreeQuery__) */
