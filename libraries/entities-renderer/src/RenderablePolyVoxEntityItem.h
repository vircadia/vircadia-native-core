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

#include <atomic>

#include <QSemaphore>

#include <PolyVoxCore/SimpleVolume.h>
#include <PolyVoxCore/Raycast.h>

#include <gpu/Forward.h>
#include <gpu/Context.h>
#include <graphics/Forward.h>
#include <graphics/Geometry.h>
#include <TextureCache.h>
#include <PolyVoxEntityItem.h>

#include "RenderableEntityItem.h"

namespace render { namespace entities {
class PolyVoxEntityRenderer;
} }


enum class PolyVoxState {
    Ready,
    Uncompressing,
    UncompressingFinished,
    BakingMesh,
    BakingMeshFinished,
    BakingMeshNoCompress,
    BakingMeshNoCompressFinished,
    Compressing,
    CompressingFinished,
    BakingShape,
    BakingShapeFinished
};

QDebug operator<<(QDebug debug, PolyVoxState state);


class RenderablePolyVoxEntityItem : public PolyVoxEntityItem, public scriptable::ModelProvider {
    friend class render::entities::PolyVoxEntityRenderer;

public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderablePolyVoxEntityItem(const EntityItemID& entityItemID);

    virtual ~RenderablePolyVoxEntityItem();

    void initializePolyVox();

    using PolyVoxEntityItem::getVoxel;
    virtual uint8_t getVoxel(const ivec3& v) const override;

    using PolyVoxEntityItem::setVoxel;
    virtual bool setVoxel(const ivec3& v, uint8_t toValue) override;

    int getOnCount() const override { return _onCount; }

    virtual bool supportsDetailedIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                             const glm::vec3& viewFrustumPos, OctreeElementPointer& element, float& distance,
                                             BoxFace& face, glm::vec3& surfaceNormal,
                                             QVariantMap& extraInfo, bool precisionPicking) const override;
    virtual bool findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity, const vec3& accleration,
                                                  const glm::vec3& viewFrustumPos, OctreeElementPointer& element,
                                                  float& parabolicDistance, BoxFace& face, glm::vec3& surfaceNormal,
                                                  QVariantMap& extraInfo, bool precisionPicking) const override;

    virtual void setVoxelData(const QByteArray& voxelData) override;
    virtual void setVoxelVolumeSize(const glm::vec3& voxelVolumeSize) override;
    virtual void setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle) override;

    virtual ShapeType getShapeType() const override;
    virtual bool isReadyToComputeShape() const override;
    virtual void computeShapeInfo(ShapeInfo& info) override;

    // coords are in voxel-volume space
    virtual bool setSphereInVolume(const vec3& center, float radius, uint8_t toValue) override;
    virtual bool setVoxelInVolume(const vec3& position, uint8_t toValue) override;

    // coords are in world-space
    virtual bool setSphere(const vec3& center, float radius, uint8_t toValue) override;
    virtual bool setCapsule(const vec3& startWorldCoords, const vec3& endWorldCoords,
                            float radiusWorldCoords, uint8_t toValue) override;
    virtual bool setAll(uint8_t toValue) override;
    virtual bool setCuboid(const vec3& lowPosition, const vec3& cuboidSize, int toValue) override;

    virtual void setXNNeighborID(const EntityItemID& xNNeighborID) override;
    virtual void setYNNeighborID(const EntityItemID& yNNeighborID) override;
    virtual void setZNNeighborID(const EntityItemID& zNNeighborID) override;

    virtual void setXPNeighborID(const EntityItemID& xPNeighborID) override;
    virtual void setYPNeighborID(const EntityItemID& yPNeighborID) override;
    virtual void setZPNeighborID(const EntityItemID& zPNeighborID) override;

    std::shared_ptr<RenderablePolyVoxEntityItem> getXNNeighbor();
    std::shared_ptr<RenderablePolyVoxEntityItem> getYNNeighbor();
    std::shared_ptr<RenderablePolyVoxEntityItem> getZNNeighbor();
    std::shared_ptr<RenderablePolyVoxEntityItem> getXPNeighbor();
    std::shared_ptr<RenderablePolyVoxEntityItem> getYPNeighbor();
    std::shared_ptr<RenderablePolyVoxEntityItem> getZPNeighbor();

    virtual void setRegistrationPoint(const glm::vec3& value) override;

    void setVoxelsFromData(QByteArray uncompressedData, quint16 voxelXSize, quint16 voxelYSize, quint16 voxelZSize);
    void forEachVoxelValue(const ivec3& voxelSize, std::function<void(const ivec3&, uint8_t)> thunk);
    QByteArray volDataToArray(quint16 voxelXSize, quint16 voxelYSize, quint16 voxelZSize) const;

    void setMesh(graphics::MeshPointer mesh);
    void setCollisionPoints(ShapeInfo::PointCollection points, AABox box);
    PolyVox::SimpleVolume<uint8_t>* getVolData() { return _volData.get(); }

    uint8_t getVoxelInternal(const ivec3& v) const;
    bool setVoxelInternal(const ivec3& v, uint8_t toValue);
    void setVoxelMarkNeighbors(int x, int y, int z, uint8_t toValue);

    void compressVolumeDataFinished(const QByteArray& voxelData);
    void neighborXEdgeChanged() { withWriteLock([&] { _updateFromNeighborXEdge = true; }); startUpdates(); }
    void neighborYEdgeChanged() { withWriteLock([&] { _updateFromNeighborYEdge = true; }); startUpdates(); }
    void neighborZEdgeChanged() { withWriteLock([&] { _updateFromNeighborZEdge = true; }); startUpdates(); }

    bool getMeshes(MeshProxyList& result) override; // deprecated
    virtual scriptable::ScriptableModelBase getScriptableModel() override;

    virtual void update(const quint64& now) override;
    bool needsToCallUpdate() const override { return _updateNeeded; }

private:
    bool updateOnCount(const ivec3& v, uint8_t toValue);
    PolyVox::RaycastResult doRayCast(glm::vec4 originInVoxel, glm::vec4 farInVoxel, glm::vec4& result) const;

    void changeUpdates(bool value);
    void startUpdates();
    void stopUpdates();

    void recomputeMesh();
    void cacheNeighbors();
    void copyUpperEdgesFromNeighbors();
    void tellNeighborsToRecopyEdges(bool force);
    bool updateDependents();

    // these are run off the main thread
    void uncompressVolumeData();
    void compressVolumeDataAndSendEditPacket();
    void computeShapeInfoWorker();

    // The PolyVoxEntityItem class has _voxelData which contains dimensions and compressed voxel data.  The dimensions
    // may not match _voxelVolumeSize.
    bool _meshReady { false }; // do we have something to give scripts that ask for the mesh?
    bool _voxelDataDirty { false }; // do we need to uncompress data and expand it into _volData?
    bool _volDataDirty { false }; // does recomputeMesh need to be called?
    bool _shapeReady { false }; // are we ready to tell bullet our shape?
    PolyVoxState _state { PolyVoxState::Ready };
    bool _updateNeeded { true };

    graphics::MeshPointer _mesh;

    ShapeInfo _shapeInfo;

    std::shared_ptr<PolyVox::SimpleVolume<uint8_t>> _volData;
    int _onCount; // how many non-zero voxels are in _volData

    bool _neighborXNeedsUpdate { false };
    bool _neighborYNeedsUpdate { false };
    bool _neighborZNeedsUpdate { false };

    bool _updateFromNeighborXEdge { false };
    bool _updateFromNeighborYEdge { false };
    bool _updateFromNeighborZEdge { false };

    // these are cached lookups of _xNNeighborID, _yNNeighborID, _zNNeighborID, _xPNeighborID, _yPNeighborID, _zPNeighborID
    EntityItemWeakPointer _xNNeighbor; // neighbor found by going along negative X axis
    EntityItemWeakPointer _yNNeighbor;
    EntityItemWeakPointer _zNNeighbor;
    EntityItemWeakPointer _xPNeighbor; // neighbor found by going along positive X axis
    EntityItemWeakPointer _yPNeighbor;
    EntityItemWeakPointer _zPNeighbor;
};

namespace render { namespace entities {

class PolyVoxEntityRenderer : public TypedEntityRenderer<RenderablePolyVoxEntityItem> {
    using Parent = TypedEntityRenderer<RenderablePolyVoxEntityItem>;
    friend class EntityRenderer;

public:
    PolyVoxEntityRenderer(const EntityItemPointer& entity);
    virtual scriptable::ScriptableModelBase getScriptableModel() override {
        return asTypedEntity<RenderablePolyVoxEntityItem>()->getScriptableModel();
    }

protected:
    virtual ItemKey getKey() override;
    virtual ShapeKey getShapeKey() override;
    virtual bool needsRenderUpdateFromTypedEntity(const TypedEntityPointer& entity) const override;
    virtual void doRenderUpdateSynchronousTyped(const ScenePointer& scene, Transaction& transaction, const TypedEntityPointer& entity) override;
    virtual void doRenderUpdateAsynchronousTyped(const TypedEntityPointer& entity) override;
    virtual void doRender(RenderArgs* args) override;
    virtual bool isTransparent() const override { return false; }

private:
#ifdef POLYVOX_ENTITY_USE_FADE_EFFECT
    bool _hasTransitioned{ false };
#endif

    graphics::MeshPointer _mesh;
    gpu::BufferPointer _params;
    std::array<NetworkTexturePointer, 3> _xyzTextures;
    glm::vec3 _lastVoxelVolumeSize;
    glm::mat4 _lastVoxelToLocalMatrix;
    glm::vec3 _position;
    glm::quat _orientation;
    PolyVoxEntityItem::PolyVoxSurfaceStyle _lastSurfaceStyle { PolyVoxEntityItem::SURFACE_MARCHING_CUBES };
    std::array<QString, 3> _xyzTextureUrls;
};

} }


#endif // hifi_RenderablePolyVoxEntityItem_h
