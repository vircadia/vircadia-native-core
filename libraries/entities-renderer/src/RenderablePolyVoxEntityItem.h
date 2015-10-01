//
//  RenderablePolyVoxEntityItem.h
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/19/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderablePolyVoxEntityItem_h
#define hifi_RenderablePolyVoxEntityItem_h

#include <QSemaphore>
#include <atomic>

#include <PolyVoxCore/SimpleVolume.h>
#include <PolyVoxCore/Raycast.h>

#include <TextureCache.h>

#include "PolyVoxEntityItem.h"
#include "RenderableDebugableEntityItem.h"
#include "RenderableEntityItem.h"
#include "gpu/Context.h"

class PolyVoxPayload {
public:
    PolyVoxPayload(EntityItemPointer owner) : _owner(owner), _bounds(AABox()) { }
    typedef render::Payload<PolyVoxPayload> Payload;
    typedef Payload::DataPointer Pointer;

    EntityItemPointer _owner;
    AABox _bounds;
};

namespace render {
   template <> const ItemKey payloadGetKey(const PolyVoxPayload::Pointer& payload);
   template <> const Item::Bound payloadGetBound(const PolyVoxPayload::Pointer& payload);
   template <> void payloadRender(const PolyVoxPayload::Pointer& payload, RenderArgs* args);
}


class RenderablePolyVoxEntityItem : public PolyVoxEntityItem {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderablePolyVoxEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);

    virtual ~RenderablePolyVoxEntityItem();

    virtual void somethingChangedNotification() {
        // This gets called from EnityItem::readEntityDataFromBuffer every time a packet describing
        // this entity comes from the entity-server.  It gets called even if nothing has actually changed
        // (see the comment in EntityItem.cpp).  If that gets fixed, this could be used to know if we
        // need to redo the voxel data.
        // _needsModelReload = true;
    }

    virtual uint8_t getVoxel(int x, int y, int z);
    virtual bool setVoxel(int x, int y, int z, uint8_t toValue);

    void render(RenderArgs* args);
    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                        bool& keepSearching, OctreeElementPointer& element, float& distance, 
                        BoxFace& face, glm::vec3& surfaceNormal,
                        void** intersectedObject, bool precisionPicking) const;

    virtual void setVoxelData(QByteArray voxelData);
    virtual void setVoxelVolumeSize(glm::vec3 voxelVolumeSize);
    virtual void setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle);

    glm::vec3 getSurfacePositionAdjustment() const;
    glm::mat4 voxelToWorldMatrix() const;
    glm::mat4 worldToVoxelMatrix() const;
    glm::mat4 voxelToLocalMatrix() const;
    glm::mat4 localToVoxelMatrix() const;

    virtual ShapeType getShapeType() const;
    virtual bool isReadyToComputeShape();
    virtual void computeShapeInfo(ShapeInfo& info);

    virtual glm::vec3 voxelCoordsToWorldCoords(glm::vec3& voxelCoords) const;
    virtual glm::vec3 worldCoordsToVoxelCoords(glm::vec3& worldCoords) const;
    virtual glm::vec3 voxelCoordsToLocalCoords(glm::vec3& voxelCoords) const;
    virtual glm::vec3 localCoordsToVoxelCoords(glm::vec3& localCoords) const;

    // coords are in voxel-volume space
    virtual bool setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue);
    virtual bool setVoxelInVolume(glm::vec3 position, uint8_t toValue);

    // coords are in world-space
    virtual bool setSphere(glm::vec3 center, float radius, uint8_t toValue);
    virtual bool setAll(uint8_t toValue);
    virtual bool setCuboid(const glm::vec3& lowPosition, const glm::vec3& cuboidSize, int toValue);

    virtual void setXTextureURL(QString xTextureURL);
    virtual void setYTextureURL(QString yTextureURL);
    virtual void setZTextureURL(QString zTextureURL);

    virtual bool addToScene(EntityItemPointer self,
                            std::shared_ptr<render::Scene> scene,
                            render::PendingChanges& pendingChanges);
    virtual void removeFromScene(EntityItemPointer self,
                                 std::shared_ptr<render::Scene> scene,
                                 render::PendingChanges& pendingChanges);

    virtual void setXNNeighborID(const EntityItemID& xNNeighborID);
    virtual void setYNNeighborID(const EntityItemID& yNNeighborID);
    virtual void setZNNeighborID(const EntityItemID& zNNeighborID);

    virtual void setXPNeighborID(const EntityItemID& xPNeighborID);
    virtual void setYPNeighborID(const EntityItemID& yPNeighborID);
    virtual void setZPNeighborID(const EntityItemID& zPNeighborID);

    virtual void rebakeMesh();

private:
    // The PolyVoxEntityItem class has _voxelData which contains dimensions and compressed voxel data.  The dimensions
    // may not match _voxelVolumeSize.

    model::MeshPointer _mesh;
    bool _meshDirty; // does collision-shape need to be recomputed?
    mutable QReadWriteLock _meshLock{QReadWriteLock::Recursive};

    NetworkTexturePointer _xTexture;
    NetworkTexturePointer _yTexture;
    NetworkTexturePointer _zTexture;

    const int MATERIAL_GPU_SLOT = 3;
    render::ItemID _myItem;
    static gpu::PipelinePointer _pipeline;

    ShapeInfo _shapeInfo;
    mutable QReadWriteLock _shapeInfoLock;

    PolyVox::SimpleVolume<uint8_t>* _volData = nullptr;
    mutable QReadWriteLock _volDataLock{QReadWriteLock::Recursive}; // lock for _volData
    bool _volDataDirty = false; // does getMesh need to be called?
    int _onCount; // how many non-zero voxels are in _volData

    bool inUserBounds(const PolyVox::SimpleVolume<uint8_t>* vol, PolyVoxEntityItem::PolyVoxSurfaceStyle surfaceStyle,
                      int x, int y, int z) const;
    uint8_t getVoxelInternal(int x, int y, int z);
    bool setVoxelInternal(int x, int y, int z, uint8_t toValue);
    bool updateOnCount(int x, int y, int z, uint8_t toValue);
    PolyVox::RaycastResult doRayCast(glm::vec4 originInVoxel, glm::vec4 farInVoxel, glm::vec4& result) const;

    // these are run off the main thread
    void decompressVolumeData();
    void decompressVolumeDataAsync();
    void compressVolumeDataAndSendEditPacket();
    void compressVolumeDataAndSendEditPacketAsync();
    void getMesh();
    void getMeshAsync();
    void computeShapeInfoWorker();
    void computeShapeInfoWorkerAsync();

    QSemaphore _threadRunning{1};

    // these are cached lookups of _xNNeighborID, _yNNeighborID, _zNNeighborID, _xPNeighborID, _yPNeighborID, _zPNeighborID
    EntityItemWeakPointer _xNNeighbor; // neighor found by going along negative X axis
    EntityItemWeakPointer _yNNeighbor;
    EntityItemWeakPointer _zNNeighbor;
    EntityItemWeakPointer _xPNeighbor; // neighor found by going along positive X axis
    EntityItemWeakPointer _yPNeighbor;
    EntityItemWeakPointer _zPNeighbor;
    void clearOutOfDateNeighbors();
    void cacheNeighbors();
    void copyUpperEdgesFromNeighbors();
    void bonkNeighbors();
};


#endif // hifi_RenderablePolyVoxEntityItem_h
