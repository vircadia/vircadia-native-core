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

#include <QFuture>
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

class RenderablePolyVoxEntityItem;


class RenderablePolyVoxAsynchronous : public QObject {
    Q_OBJECT

 public:
    RenderablePolyVoxAsynchronous(PolyVoxEntityItem::PolyVoxSurfaceStyle voxelSurfaceStyle, glm::vec3 voxelVolumeSize,
                                  RenderablePolyVoxEntityItem* owner);
    ~RenderablePolyVoxAsynchronous();

    void setVoxelVolumeSize(glm::vec3 voxelVolumeSize);
    void updateVoxelSurfaceStyle(PolyVoxEntityItem::PolyVoxSurfaceStyle voxelSurfaceStyle, quint64 modelVersion);
    uint8_t getVoxel(int x, int y, int z);
    bool setVoxel(int x, int y, int z, uint8_t toValue, quint64 modelVersion);
    bool setAll(uint8_t toValue, quint64 modelVersion);
    bool setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue, quint64 modelVersion);
    PolyVox::RaycastResult doRayCast(glm::vec4 originInVoxel, glm::vec4 farInVoxel, glm::vec4& result) const;
    int getOnCount() const;
    void decompressVolumeData(QByteArray voxelData, quint64 modelVersion);

// signals:
//     void doDecompressVolumeDataAsync(QByteArray voxelData, quint64 modelVersion);
//     void doGetMeshAsync(quint64 modelVersion);



private:
    void decompressVolumeDataAsync(QByteArray voxelData, quint64 modelVersion);
    void getMeshAsync(quint64 modelVersion);

    uint8_t getVoxelInternal(int x, int y, int z);
    bool updateOnCount(int x, int y, int z, uint8_t new_value);
    bool setVoxelInternal(int x, int y, int z, uint8_t toValue);
    void compressVolumeData(quint64 modelVersion);
    static bool inUserBounds(const PolyVox::SimpleVolume<uint8_t>* vol, PolyVoxEntityItem::PolyVoxSurfaceStyle surfaceStyle,
                             int x, int y, int z);

    PolyVoxEntityItem::PolyVoxSurfaceStyle _voxelSurfaceStyle;
    glm::vec3 _voxelVolumeSize;
    PolyVox::SimpleVolume<uint8_t>* _volData = nullptr;
    mutable QReadWriteLock _volDataLock; // lock for _volData
    std::atomic_int _onCount; // how many non-zero voxels are in _volData

    RenderablePolyVoxEntityItem* _owner = nullptr;
};


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
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
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

    virtual void setXTextureURL(QString xTextureURL);
    virtual void setYTextureURL(QString yTextureURL);
    virtual void setZTextureURL(QString zTextureURL);

    virtual bool addToScene(EntityItemPointer self,
                            std::shared_ptr<render::Scene> scene,
                            render::PendingChanges& pendingChanges);
    virtual void removeFromScene(EntityItemPointer self,
                                 std::shared_ptr<render::Scene> scene,
                                 render::PendingChanges& pendingChanges);

    void receiveNewVoxelData(QByteArray newVoxelData, quint64 dataVersion);
    void receiveNewMesh(model::MeshPointer newMeshPtr, quint64 meshVersion);
    void setDataVersion(quint64 dataVersion) { _dataVersion = dataVersion; }

private:
    // The PolyVoxEntityItem class has _voxelData which contains dimensions and compressed voxel data.  The dimensions
    // may not match _voxelVolumeSize.

    virtual bool computeShapeInfoWorker();

    // model::Geometry _modelGeometry;
    mutable QReadWriteLock _modelGeometryLock;
    model::MeshPointer _mesh;

    QFuture<bool> _getMeshWorker;

    NetworkTexturePointer _xTexture;
    NetworkTexturePointer _yTexture;
    NetworkTexturePointer _zTexture;

    const int MATERIAL_GPU_SLOT = 3;
    render::ItemID _myItem;
    static gpu::PipelinePointer _pipeline;

    ShapeInfo _shapeInfo;
    // QFuture<bool> _shapeInfoWorker;
    mutable QReadWriteLock _shapeInfoLock;

    // this does work outside of the main thread.
    RenderablePolyVoxAsynchronous _async;

    quint64 _modelVersion = 1; // local idea of how many changes have happened
    // the following are compared against _modelVersion
    quint64 _meshVersion = 0; // version of most recently computed mesh
    quint64 _dataVersion = 0; // version of most recently compressed voxel data
    quint64 _shapeVersion = 0; // version of most recently computed collision shape
};


#endif // hifi_RenderablePolyVoxEntityItem_h
