//
//  VoxelQuery.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelQuery__
#define __hifi__VoxelQuery__

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
const int WANT_OCCLUSION_CULLING_BIT = 3; // 4th bit

class VoxelQuery : public NodeData {
    Q_OBJECT
    
public:
    VoxelQuery(Node* owningNode = NULL);
    ~VoxelQuery();

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
    
    // related to Voxel Sending strategies
    bool getWantColor() const { return _wantColor; }
    bool getWantDelta() const { return _wantDelta; }
    bool getWantLowResMoving() const { return _wantLowResMoving; }
    bool getWantOcclusionCulling() const { return _wantOcclusionCulling; }
    int getMaxVoxelPacketsPerSecond() const { return _maxVoxelPPS; }
    float getVoxelSizeScale() const { return _voxelSizeScale; }
    int getBoundaryLevelAdjust() const { return _boundaryLevelAdjust; }
    
public slots:
    void setWantLowResMoving(bool wantLowResMoving) { _wantLowResMoving = wantLowResMoving; }
    void setWantColor(bool wantColor) { _wantColor = wantColor; }
    void setWantDelta(bool wantDelta) { _wantDelta = wantDelta; }
    void setWantOcclusionCulling(bool wantOcclusionCulling) { _wantOcclusionCulling = wantOcclusionCulling; }
    void setMaxVoxelPacketsPerSecond(int maxVoxelPPS) { _maxVoxelPPS = maxVoxelPPS; }
    void setVoxelSizeScale(float voxelSizeScale) { _voxelSizeScale = voxelSizeScale; }
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
    
    // voxel server sending items
    bool _wantColor;
    bool _wantDelta;
    bool _wantLowResMoving;
    bool _wantOcclusionCulling;
    int _maxVoxelPPS;
    float _voxelSizeScale; /// used for LOD calculations
    int _boundaryLevelAdjust; /// used for LOD calculations
    
private:
    // privatize the copy constructor and assignment operator so they cannot be called
    VoxelQuery(const VoxelQuery&);
    VoxelQuery& operator= (const VoxelQuery&);
};

#endif /* defined(__hifi__VoxelQuery__) */
