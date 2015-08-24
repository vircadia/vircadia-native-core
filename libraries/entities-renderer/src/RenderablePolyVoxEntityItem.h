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

#include <PolyVoxCore/SimpleVolume.h>
#include <TextureCache.h>

#include "PolyVoxEntityItem.h"
#include "RenderableDebugableEntityItem.h"
#include "RenderableEntityItem.h"


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

    bool updateOnCount(int x, int y, int z, uint8_t new_value);

    void render(RenderArgs* args);
    virtual bool supportsDetailedRayIntersection() const { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                         bool& keepSearching, OctreeElement*& element, float& distance, BoxFace& face,
                         void** intersectedObject, bool precisionPicking) const;

    void getModel();

    virtual void setVoxelData(QByteArray voxelData);

    virtual void setVoxelVolumeSize(glm::vec3 voxelVolumeSize);
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

protected:
    virtual void updateVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle);

private:
    // The PolyVoxEntityItem class has _voxelData which contains dimensions and compressed voxel data.  The dimensions
    // may not match _voxelVolumeSize.

    bool setVoxelInternal(int x, int y, int z, uint8_t toValue);
    void compressVolumeData();
    void decompressVolumeData();
    void clearEdges();

    PolyVox::SimpleVolume<uint8_t>* _volData = nullptr;
    model::Geometry _modelGeometry;
    bool _needsModelReload = true;

    QVector<QVector<glm::vec3>> _points; // XXX

    NetworkTexturePointer _xTexture;
    NetworkTexturePointer _yTexture;
    NetworkTexturePointer _zTexture;

    int _onCount = 0; // how many non-zero voxels are in _volData

    const int MATERIAL_GPU_SLOT = 3;
    render::ItemID _myItem;
    static gpu::PipelinePointer _pipeline;
};


#endif // hifi_RenderablePolyVoxEntityItem_h
