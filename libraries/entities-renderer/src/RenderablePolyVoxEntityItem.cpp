//
//  RenderablePolyVoxEntityItem.cpp
//  libraries/entities-renderer/src/
//
//  Created by Seth Alves on 5/19/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <math.h>
#include <QObject>
#include <QByteArray>
#include <QtConcurrent/QtConcurrentRun>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include <DeferredLightingEffect.h>
#include <Model.h>
#include <PerfStat.h>
#include <render/Scene.h>

#include <PolyVoxCore/CubicSurfaceExtractorWithNormals.h>
#include <PolyVoxCore/MarchingCubesSurfaceExtractor.h>
#include <PolyVoxCore/SurfaceMesh.h>
#include <PolyVoxCore/SimpleVolume.h>
#include <PolyVoxCore/Material.h>

#include "model/Geometry.h"
#include "EntityTreeRenderer.h"
#include "polyvox_vert.h"
#include "polyvox_frag.h"
#include "RenderablePolyVoxEntityItem.h"
#include "EntityEditPacketSender.h"
#include "PhysicalEntitySimulation.h"

gpu::PipelinePointer RenderablePolyVoxEntityItem::_pipeline = nullptr;
const float MARCHING_CUBE_COLLISION_HULL_OFFSET = 0.5;

EntityItemPointer RenderablePolyVoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<RenderablePolyVoxEntityItem>(entityID, properties);
}

RenderablePolyVoxEntityItem::RenderablePolyVoxEntityItem(const EntityItemID& entityItemID,
                                                         const EntityItemProperties& properties) :
    PolyVoxEntityItem(entityItemID, properties),
    _mesh(new model::Mesh()),
    _meshDirty(true),
    _xTexture(nullptr),
    _yTexture(nullptr),
    _zTexture(nullptr) {
    setVoxelVolumeSize(_voxelVolumeSize);
    getMeshAsync();
}

RenderablePolyVoxEntityItem::~RenderablePolyVoxEntityItem() {
}


void RenderablePolyVoxEntityItem::setVoxelData(QByteArray voxelData) {
    _voxelDataLock.lockForWrite();
    if (_voxelData == voxelData) {
        _voxelDataLock.unlock();
        return;
    }

    _voxelData = voxelData;
    _voxelDataDirty = true;
    _voxelDataLock.unlock();
    decompressVolumeData();
}


void RenderablePolyVoxEntityItem::setVoxelSurfaceStyle(PolyVoxSurfaceStyle voxelSurfaceStyle) {
    if (_voxelSurfaceStyle == voxelSurfaceStyle) {
        return;
    }

    // if we are switching to or from "edged" we need to force a resize of _volData.
    bool wasEdged = (_voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_CUBIC ||
                     _voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES);
    bool willBeEdged = (voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_CUBIC ||
                        voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES);

    if (wasEdged != willBeEdged) {
        _volDataLock.lockForWrite();
        _volDataDirty = true;
        if (_volData) {
            delete _volData;
        }
        _volData = nullptr;
        _voxelSurfaceStyle = voxelSurfaceStyle;
        _volDataLock.unlock();
        setVoxelVolumeSize(_voxelVolumeSize);
        decompressVolumeData();
    } else {
        _voxelSurfaceStyle = voxelSurfaceStyle;
        getMesh();
    }
}


glm::vec3 RenderablePolyVoxEntityItem::getSurfacePositionAdjustment() const {
    glm::vec3 scale = getDimensions() / _voxelVolumeSize; // meters / voxel-units
    switch (_voxelSurfaceStyle) {
        case PolyVoxEntityItem::SURFACE_MARCHING_CUBES:
        case PolyVoxEntityItem::SURFACE_CUBIC:
            return scale / 2.0f;
        case PolyVoxEntityItem::SURFACE_EDGED_CUBIC:
        case PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES:
            return scale / -2.0f;
    }
    return glm::vec3(0.0f, 0.0f, 0.0f);
}


glm::mat4 RenderablePolyVoxEntityItem::voxelToLocalMatrix() const {
    glm::vec3 scale = getDimensions() / _voxelVolumeSize; // meters / voxel-units
    glm::vec3 center = getCenterPosition();
    glm::vec3 position = getPosition();
    glm::vec3 positionToCenter = center - position;
    positionToCenter -= getDimensions() * glm::vec3(0.5f, 0.5f, 0.5f) - getSurfacePositionAdjustment();
    glm::mat4 centerToCorner = glm::translate(glm::mat4(), positionToCenter);
    glm::mat4 scaled = glm::scale(centerToCorner, scale);
    return scaled;
}

glm::mat4 RenderablePolyVoxEntityItem::localToVoxelMatrix() const {
    glm::mat4 localToModelMatrix = glm::inverse(voxelToLocalMatrix());
    return localToModelMatrix;
}

glm::mat4 RenderablePolyVoxEntityItem::voxelToWorldMatrix() const {
    glm::mat4 rotation = glm::mat4_cast(getRotation());
    glm::mat4 translation = glm::translate(getPosition());
    return translation * rotation * voxelToLocalMatrix();
}

glm::mat4 RenderablePolyVoxEntityItem::worldToVoxelMatrix() const {
    glm::mat4 worldToModelMatrix = glm::inverse(voxelToWorldMatrix());
    return worldToModelMatrix;
}


bool RenderablePolyVoxEntityItem::setVoxel(int x, int y, int z, uint8_t toValue) {
    if (_locked) {
        return false;
    }

    _volDataLock.lockForWrite();
    bool result = setVoxelInternal(x, y, z, toValue);
    if (result) {
        _volDataDirty = true;
    }
    _volDataLock.unlock();
    if (result) {
        compressVolumeDataAndSendEditPacket();
    }

    return result;
}


bool RenderablePolyVoxEntityItem::setAll(uint8_t toValue) {
    bool result = false;
    if (_locked) {
        return result;
    }

    _volDataLock.lockForWrite();
    _volDataDirty = true;
    for (int z = 0; z < _voxelVolumeSize.z; z++) {
        for (int y = 0; y < _voxelVolumeSize.y; y++) {
            for (int x = 0; x < _voxelVolumeSize.x; x++) {
                result |= setVoxelInternal(x, y, z, toValue);
            }
        }
    }
    _volDataLock.unlock();
    if (result) {
        compressVolumeDataAndSendEditPacket();
    }
    return result;
}



bool RenderablePolyVoxEntityItem::setVoxelInVolume(glm::vec3 position, uint8_t toValue) {
    if (_locked) {
        return false;
    }

    // same as setVoxel but takes a vector rather than 3 floats.
    return setVoxel(roundf(position.x), roundf(position.y), roundf(position.z), toValue);
}

bool RenderablePolyVoxEntityItem::setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue) {
    bool result = false;
    if (_locked) {
        return result;
    }

    // This three-level for loop iterates over every voxel in the volume
    _volDataLock.lockForWrite();
    _volDataDirty = true;
    for (int z = 0; z < _voxelVolumeSize.z; z++) {
        for (int y = 0; y < _voxelVolumeSize.y; y++) {
            for (int x = 0; x < _voxelVolumeSize.x; x++) {
                // Store our current position as a vector...
                glm::vec3 pos(x + 0.5f, y + 0.5f, z + 0.5f); // consider voxels cenetered on their coordinates
                // And compute how far the current position is from the center of the volume
                float fDistToCenter = glm::distance(pos, center);
                // If the current voxel is less than 'radius' units from the center then we set its value
                if (fDistToCenter <= radius) {
                    result |= setVoxelInternal(x, y, z, toValue);
                }
            }
        }
    }
    _volDataLock.unlock();
    if (result) {
        compressVolumeDataAndSendEditPacket();
    }
    return result;
}

bool RenderablePolyVoxEntityItem::setSphere(glm::vec3 centerWorldCoords, float radiusWorldCoords, uint8_t toValue) {
    // glm::vec3 centerVoxelCoords = worldToVoxelCoordinates(centerWorldCoords);
    glm::vec4 centerVoxelCoords = worldToVoxelMatrix() * glm::vec4(centerWorldCoords, 1.0f);
    glm::vec3 scale = getDimensions() / _voxelVolumeSize; // meters / voxel-units
    float scaleY = scale.y;
    float radiusVoxelCoords = radiusWorldCoords / scaleY;
    return setSphereInVolume(glm::vec3(centerVoxelCoords), radiusVoxelCoords, toValue);
}

class RaycastFunctor
{
public:
    RaycastFunctor(PolyVox::SimpleVolume<uint8_t>* vol) :
        _result(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)),
        _vol(vol) {
    }

    static bool inBounds(const PolyVox::SimpleVolume<uint8_t>* vol, int x, int y, int z) {
        // x, y, z are in polyvox volume coords
        return !(x < 0 || y < 0 || z < 0 || x >= vol->getWidth() || y >= vol->getHeight() || z >= vol->getDepth());
    }


    bool operator()(PolyVox::SimpleVolume<unsigned char>::Sampler& sampler)
    {
        PolyVox::Vector3DInt32 positionIndex = sampler.getPosition();
        int x = positionIndex.getX();
        int y = positionIndex.getY();
        int z = positionIndex.getZ();

        if (!inBounds(_vol, x, y, z)) {
            return true;
        }

        if (sampler.getVoxel() == 0) {
            return true; // keep raycasting
        }

        _result = glm::vec4((float)x, (float)y, (float)z, 1.0f);
        return false;
    }
    glm::vec4 _result;
    const PolyVox::SimpleVolume<uint8_t>* _vol = nullptr;
};

bool RenderablePolyVoxEntityItem::findDetailedRayIntersection(const glm::vec3& origin,
                                                              const glm::vec3& direction,
                                                              bool& keepSearching,
                                                              OctreeElement*& element,
                                                              float& distance, BoxFace& face,
                                                              void** intersectedObject,
                                                              bool precisionPicking) const
{
    // TODO -- correctly pick against marching-cube generated meshes
    if (!precisionPicking) {
        // just intersect with bounding box
        return true;
    }

    glm::mat4 wtvMatrix = worldToVoxelMatrix();
    glm::mat4 vtwMatrix = voxelToWorldMatrix();
    glm::mat4 vtlMatrix = voxelToLocalMatrix();
    glm::vec3 normDirection = glm::normalize(direction);

    // the PolyVox ray intersection code requires a near and far point.
    // set ray cast length to long enough to cover all of the voxel space
    float distanceToEntity = glm::distance(origin, getPosition());
    float largestDimension = glm::max(getDimensions().x, getDimensions().y, getDimensions().z) * 2.0f;
    glm::vec3 farPoint = origin + normDirection * (distanceToEntity + largestDimension);
    glm::vec4 originInVoxel = wtvMatrix * glm::vec4(origin, 1.0f);
    glm::vec4 farInVoxel = wtvMatrix * glm::vec4(farPoint, 1.0f);

    glm::vec4 result;
    PolyVox::RaycastResult raycastResult = doRayCast(originInVoxel, farInVoxel, result);
    if (raycastResult == PolyVox::RaycastResults::Completed) {
        // the ray completed its path -- nothing was hit.
        return false;
    }

    // set up ray tests against each face of the voxel.
    glm::vec3 minXPosition = glm::vec3(vtwMatrix * (result + glm::vec4(0.0f, 0.5f, 0.5f, 0.0f)));
    glm::vec3 maxXPosition = glm::vec3(vtwMatrix * (result + glm::vec4(1.0f, 0.5f, 0.5f, 0.0f)));
    glm::vec3 minYPosition = glm::vec3(vtwMatrix * (result + glm::vec4(0.5f, 0.0f, 0.5f, 0.0f)));
    glm::vec3 maxYPosition = glm::vec3(vtwMatrix * (result + glm::vec4(0.5f, 1.0f, 0.5f, 0.0f)));
    glm::vec3 minZPosition = glm::vec3(vtwMatrix * (result + glm::vec4(0.5f, 0.5f, 0.0f, 0.0f)));
    glm::vec3 maxZPosition = glm::vec3(vtwMatrix * (result + glm::vec4(0.5f, 0.5f, 1.0f, 0.0f)));

    glm::vec4 baseDimensions = glm::vec4(1.0, 1.0, 1.0, 0.0);
    glm::vec3 worldDimensions = glm::vec3(vtlMatrix * baseDimensions);
    glm::vec2 xDimensions = glm::vec2(worldDimensions.z, worldDimensions.y);
    glm::vec2 yDimensions = glm::vec2(worldDimensions.x, worldDimensions.z);
    glm::vec2 zDimensions = glm::vec2(worldDimensions.x, worldDimensions.y);

    glm::quat vtwRotation = extractRotation(vtwMatrix);
    glm::quat minXRotation = vtwRotation * glm::quat(glm::vec3(0.0f, PI_OVER_TWO, 0.0f));
    glm::quat maxXRotation = vtwRotation * glm::quat(glm::vec3(0.0f, PI_OVER_TWO, 0.0f));
    glm::quat minYRotation = vtwRotation * glm::quat(glm::vec3(PI_OVER_TWO, 0.0f, 0.0f));
    glm::quat maxYRotation = vtwRotation * glm::quat(glm::vec3(PI_OVER_TWO, 0.0f, 0.0f));
    glm::quat minZRotation = vtwRotation * glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
    glm::quat maxZRotation = vtwRotation * glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));

    float bestDx = FLT_MAX;
    bool hit[ 6 ];
    float dx[ 6 ] = {FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX};

    hit[0] = findRayRectangleIntersection(origin, direction, minXRotation, minXPosition, xDimensions, dx[0]);
    hit[1] = findRayRectangleIntersection(origin, direction, maxXRotation, maxXPosition, xDimensions, dx[1]);
    hit[2] = findRayRectangleIntersection(origin, direction, minYRotation, minYPosition, yDimensions, dx[2]);
    hit[3] = findRayRectangleIntersection(origin, direction, maxYRotation, maxYPosition, yDimensions, dx[3]);
    hit[4] = findRayRectangleIntersection(origin, direction, minZRotation, minZPosition, zDimensions, dx[4]);
    hit[5] = findRayRectangleIntersection(origin, direction, maxZRotation, maxZPosition, zDimensions, dx[5]);

    bool ok = false;
    for (int i = 0; i < 6; i ++) {
        if (hit[ i ] && dx[ i ] < bestDx) {
            face = (BoxFace)i;
            distance = dx[ i ];
            ok = true;
            bestDx = dx[ i ];
        }
    }

    if (!ok) {
        // if the attempt to put the ray against one of the voxel-faces fails, just return the center
        glm::vec4 intersectedWorldPosition = vtwMatrix * (result + vec4(0.5f, 0.5f, 0.5f, 0.0f));
        distance = glm::distance(glm::vec3(intersectedWorldPosition), origin);
        face = BoxFace::MIN_X_FACE;
    }

    return true;
}


PolyVox::RaycastResult RenderablePolyVoxEntityItem::doRayCast(glm::vec4 originInVoxel,
                                                              glm::vec4 farInVoxel,
                                                              glm::vec4& result) const {
    PolyVox::Vector3DFloat startPoint(originInVoxel.x, originInVoxel.y, originInVoxel.z);
    PolyVox::Vector3DFloat endPoint(farInVoxel.x, farInVoxel.y, farInVoxel.z);

    _volDataLock.lockForRead();
    RaycastFunctor callback(_volData);
    PolyVox::RaycastResult raycastResult = PolyVox::raycastWithEndpoints(_volData, startPoint, endPoint, callback);
    _volDataLock.unlock();

    // result is in voxel-space coordinates.
    result = callback._result - glm::vec4(0.5f, 0.5f, 0.5f, 0.0f);
    return raycastResult;
}


// virtual
ShapeType RenderablePolyVoxEntityItem::getShapeType() const {
    return SHAPE_TYPE_COMPOUND;
}

bool RenderablePolyVoxEntityItem::isReadyToComputeShape() {
    _meshLock.lockForRead();
    if (_meshDirty) {
        _meshLock.unlock();
        computeShapeInfoWorker();
        return false;
    }
    _meshLock.unlock();
    return true;
}

void RenderablePolyVoxEntityItem::computeShapeInfo(ShapeInfo& info) {
    _shapeInfoLock.lockForRead();
    info = _shapeInfo;
    _shapeInfoLock.unlock();
}

void RenderablePolyVoxEntityItem::setXTextureURL(QString xTextureURL) {
    _xTexture.clear();
    PolyVoxEntityItem::setXTextureURL(xTextureURL);
}

void RenderablePolyVoxEntityItem::setYTextureURL(QString yTextureURL) {
    _yTexture.clear();
    PolyVoxEntityItem::setYTextureURL(yTextureURL);
}

void RenderablePolyVoxEntityItem::setZTextureURL(QString zTextureURL) {
    _zTexture.clear();
    PolyVoxEntityItem::setZTextureURL(zTextureURL);
}

void RenderablePolyVoxEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderablePolyVoxEntityItem::render");
    assert(getType() == EntityTypes::PolyVox);
    Q_ASSERT(args->_batch);

    _volDataLock.lockForRead();
    if (_volDataDirty) {
        getMesh();
    }
    _volDataLock.unlock();

    _meshLock.lockForRead();
    model::MeshPointer mesh = _mesh;
    _meshLock.unlock();

    if (!_pipeline) {
        gpu::ShaderPointer vertexShader = gpu::ShaderPointer(gpu::Shader::createVertex(std::string(polyvox_vert)));
        gpu::ShaderPointer pixelShader = gpu::ShaderPointer(gpu::Shader::createPixel(std::string(polyvox_frag)));

        gpu::Shader::BindingSet slotBindings;
        slotBindings.insert(gpu::Shader::Binding(std::string("materialBuffer"), MATERIAL_GPU_SLOT));
        slotBindings.insert(gpu::Shader::Binding(std::string("xMap"), 0));
        slotBindings.insert(gpu::Shader::Binding(std::string("yMap"), 1));
        slotBindings.insert(gpu::Shader::Binding(std::string("zMap"), 2));

        gpu::ShaderPointer program = gpu::ShaderPointer(gpu::Shader::createProgram(vertexShader, pixelShader));
        gpu::Shader::makeProgram(*program, slotBindings);

        auto state = std::make_shared<gpu::State>();
        state->setCullMode(gpu::State::CULL_BACK);
        state->setDepthTest(true, true, gpu::LESS_EQUAL);

        _pipeline = gpu::PipelinePointer(gpu::Pipeline::create(program, state));
    }

    gpu::Batch& batch = *args->_batch;
    batch.setPipeline(_pipeline);

    Transform transform(voxelToWorldMatrix());
    batch.setModelTransform(transform);
    batch.setInputFormat(mesh->getVertexFormat());
    batch.setInputBuffer(gpu::Stream::POSITION, mesh->getVertexBuffer());
    batch.setInputBuffer(gpu::Stream::NORMAL,
                         mesh->getVertexBuffer()._buffer,
                         sizeof(float) * 3,
                         mesh->getVertexBuffer()._stride);
    batch.setIndexBuffer(gpu::UINT32, mesh->getIndexBuffer()._buffer, 0);

    if (!_xTextureURL.isEmpty() && !_xTexture) {
        _xTexture = DependencyManager::get<TextureCache>()->getTexture(_xTextureURL);
    }
    if (!_yTextureURL.isEmpty() && !_yTexture) {
        _yTexture = DependencyManager::get<TextureCache>()->getTexture(_yTextureURL);
    }
    if (!_zTextureURL.isEmpty() && !_zTexture) {
        _zTexture = DependencyManager::get<TextureCache>()->getTexture(_zTextureURL);
    }

    if (_xTexture) {
        batch.setResourceTexture(0, _xTexture->getGPUTexture());
    } else {
        batch.setResourceTexture(0, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }
    if (_yTexture) {
        batch.setResourceTexture(1, _yTexture->getGPUTexture());
    } else {
        batch.setResourceTexture(1, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }
    if (_zTexture) {
        batch.setResourceTexture(2, _zTexture->getGPUTexture());
    } else {
        batch.setResourceTexture(2, DependencyManager::get<TextureCache>()->getWhiteTexture());
    }

    int voxelVolumeSizeLocation = _pipeline->getProgram()->getUniforms().findLocation("voxelVolumeSize");
    batch._glUniform3f(voxelVolumeSizeLocation, _voxelVolumeSize.x, _voxelVolumeSize.y, _voxelVolumeSize.z);

    batch.drawIndexed(gpu::TRIANGLES, mesh->getNumIndices(), 0);

    RenderableDebugableEntityItem::render(this, args);
}

bool RenderablePolyVoxEntityItem::addToScene(EntityItemPointer self,
                                             std::shared_ptr<render::Scene> scene,
                                             render::PendingChanges& pendingChanges) {
    _myItem = scene->allocateID();

    auto renderItem = std::make_shared<PolyVoxPayload>(shared_from_this());
    auto renderData = PolyVoxPayload::Pointer(renderItem);
    auto renderPayload = std::make_shared<PolyVoxPayload::Payload>(renderData);

    pendingChanges.resetItem(_myItem, renderPayload);

    return true;
}

void RenderablePolyVoxEntityItem::removeFromScene(EntityItemPointer self,
                                                  std::shared_ptr<render::Scene> scene,
                                                  render::PendingChanges& pendingChanges) {
    pendingChanges.removeItem(_myItem);
}

namespace render {
    template <> const ItemKey payloadGetKey(const PolyVoxPayload::Pointer& payload) {
        return ItemKey::Builder::opaqueShape();
    }

    template <> const Item::Bound payloadGetBound(const PolyVoxPayload::Pointer& payload) {
        if (payload && payload->_owner) {
            auto polyVoxEntity = std::dynamic_pointer_cast<RenderablePolyVoxEntityItem>(payload->_owner);
            return polyVoxEntity->getAABox();
        }
        return render::Item::Bound();
    }

    template <> void payloadRender(const PolyVoxPayload::Pointer& payload, RenderArgs* args) {
        if (args && payload && payload->_owner) {
            payload->_owner->render(args);
        }
    }
}


glm::vec3 RenderablePolyVoxEntityItem::voxelCoordsToWorldCoords(glm::vec3& voxelCoords) const {
    return glm::vec3(voxelToWorldMatrix() * glm::vec4(voxelCoords, 1.0f));
}

glm::vec3 RenderablePolyVoxEntityItem::worldCoordsToVoxelCoords(glm::vec3& worldCoords) const {
    glm::vec3 result = glm::vec3(worldToVoxelMatrix() * glm::vec4(worldCoords, 1.0f));
    switch (_voxelSurfaceStyle) {
        case PolyVoxEntityItem::SURFACE_MARCHING_CUBES:
        case PolyVoxEntityItem::SURFACE_CUBIC:
            result += glm::vec3(0.5f, 0.5f, 0.5f);
            break;
        case PolyVoxEntityItem::SURFACE_EDGED_CUBIC:
        case PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES:
            result -= glm::vec3(0.5f, 0.5f, 0.5f);
            break;
    }

    return result;
}

glm::vec3 RenderablePolyVoxEntityItem::voxelCoordsToLocalCoords(glm::vec3& voxelCoords) const {
    return glm::vec3(voxelToLocalMatrix() * glm::vec4(voxelCoords, 0.0f));
}

glm::vec3 RenderablePolyVoxEntityItem::localCoordsToVoxelCoords(glm::vec3& localCoords) const {
    return glm::vec3(localToVoxelMatrix() * glm::vec4(localCoords, 0.0f));
}


void RenderablePolyVoxEntityItem::setVoxelVolumeSize(glm::vec3 voxelVolumeSize) {
    if (_volData && _voxelVolumeSize == voxelVolumeSize) {
        return;
    }

    _volDataLock.lockForWrite();
    _volDataDirty = true;
    _voxelVolumeSize = voxelVolumeSize;

    if (_volData) {
        delete _volData;
    }
    _onCount = 0;

    if (_voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_CUBIC ||
        _voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES) {
        // with _EDGED_ we maintain an extra box of voxels around those that the user asked for.  This
        // changes how the surface extractor acts -- mainly it becomes impossible to have holes in the
        // generated mesh.  The non _EDGED_ modes will leave holes in the mesh at the edges of the
        // voxel space.
        PolyVox::Vector3DInt32 lowCorner(0, 0, 0);
        PolyVox::Vector3DInt32 highCorner(_voxelVolumeSize.x + 1, // corners are inclusive
                                          _voxelVolumeSize.y + 1,
                                          _voxelVolumeSize.z + 1);
        _volData = new PolyVox::SimpleVolume<uint8_t>(PolyVox::Region(lowCorner, highCorner));
    } else {
        PolyVox::Vector3DInt32 lowCorner(0, 0, 0);
        PolyVox::Vector3DInt32 highCorner(_voxelVolumeSize.x, // -1 because these corners are inclusive
                                          _voxelVolumeSize.y,
                                          _voxelVolumeSize.z);
        _volData = new PolyVox::SimpleVolume<uint8_t>(PolyVox::Region(lowCorner, highCorner));


        // // XXX
        // {
        //     for (int x = 0; x <= _voxelVolumeSize.x; x++) {
        //         for (int y = 0; y <= _voxelVolumeSize.y; y++) {
        //             for (int z = 0; z <= _voxelVolumeSize.z; z++) {
        //                 if (x == _voxelVolumeSize.x ||
        //                     y == _voxelVolumeSize.y ||
        //                     z == _voxelVolumeSize.z) {
        //                     setVoxelInternal(x, y, z, 255);
        //                 }
        //             }
        //         }
        //     }
        // }

    }

    // having the "outside of voxel-space" value be 255 has helped me notice some problems.
    _volData->setBorderValue(255);
    _volDataLock.unlock();
    decompressVolumeData();
}


bool RenderablePolyVoxEntityItem::inUserBounds(const PolyVox::SimpleVolume<uint8_t>* vol,
                                               PolyVoxEntityItem::PolyVoxSurfaceStyle surfaceStyle,
                                               int x, int y, int z) {
    // x, y, z are in user voxel-coords, not adjusted-for-edge voxel-coords.
    switch (surfaceStyle) {
        case PolyVoxEntityItem::SURFACE_MARCHING_CUBES:
        case PolyVoxEntityItem::SURFACE_CUBIC:
            if (x < 0 || y < 0 || z < 0 ||
                x >= vol->getWidth() || y >= vol->getHeight() || z >= vol->getDepth()) {
                return false;
            }
            return true;

        case PolyVoxEntityItem::SURFACE_EDGED_CUBIC:
        case PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES:
            if (x < 0 || y < 0 || z < 0 ||
                x >= vol->getWidth() - 2 || y >= vol->getHeight() - 2 || z >= vol->getDepth() - 2) {
                return false;
            }
            return true;
    }

    return false;
}


uint8_t RenderablePolyVoxEntityItem::getVoxel(int x, int y, int z) {
    _volDataLock.lockForRead();
    auto result = getVoxelInternal(x, y, z);
    _volDataLock.unlock();
    return result;
}


uint8_t RenderablePolyVoxEntityItem::getVoxelInternal(int x, int y, int z) {
    if (!inUserBounds(_volData, _voxelSurfaceStyle, x, y, z)) {
        return 0;
    }

    // if _voxelSurfaceStyle is SURFACE_EDGED_CUBIC, we maintain an extra layer of
    // voxels all around the requested voxel space.  Having the empty voxels around
    // the edges changes how the surface extractor behaves.

    uint8_t result;
    if (_voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_CUBIC ||
        _voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES) {
        result = _volData->getVoxelAt(x + 1, y + 1, z + 1);
    } else {
        result = _volData->getVoxelAt(x, y, z);
    }

    return result;
}


bool RenderablePolyVoxEntityItem::setVoxelInternal(int x, int y, int z, uint8_t toValue) {
    // set a voxel without recompressing the voxel data
    bool result = false;
    if (!inUserBounds(_volData, _voxelSurfaceStyle, x, y, z)) {
        return result;
    }

    result = updateOnCount(x, y, z, toValue);

    assert(_volData);
    if (_voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_CUBIC ||
        _voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES) {
        _volData->setVoxelAt(x + 1, y + 1, z + 1, toValue);
    } else {
        _volData->setVoxelAt(x, y, z, toValue);
    }

    return result;
}


bool RenderablePolyVoxEntityItem::updateOnCount(int x, int y, int z, uint8_t toValue) {
    // keep _onCount up to date
    if (!inUserBounds(_volData, _voxelSurfaceStyle, x, y, z)) {
        return false;
    }

    uint8_t uVoxelValue = getVoxelInternal(x, y, z);
    if (toValue != 0) {
        if (uVoxelValue == 0) {
            _onCount++;
            return true;
        }
    } else {
        // toValue == 0
        if (uVoxelValue != 0) {
            _onCount--;
            assert(_onCount >= 0);
            return true;
        }
    }
    return false;
}


void RenderablePolyVoxEntityItem::decompressVolumeData() {
    _threadRunning.acquire();
    QtConcurrent::run(this, &RenderablePolyVoxEntityItem::decompressVolumeDataAsync);
}


// take compressed data and expand it into _volData.
void RenderablePolyVoxEntityItem::decompressVolumeDataAsync() {
    _voxelDataLock.lockForRead();
    QDataStream reader(_voxelData);
    quint16 voxelXSize, voxelYSize, voxelZSize;
    reader >> voxelXSize;
    reader >> voxelYSize;
    reader >> voxelZSize;

    if (voxelXSize == 0 || voxelXSize > PolyVoxEntityItem::MAX_VOXEL_DIMENSION ||
        voxelYSize == 0 || voxelYSize > PolyVoxEntityItem::MAX_VOXEL_DIMENSION ||
        voxelZSize == 0 || voxelZSize > PolyVoxEntityItem::MAX_VOXEL_DIMENSION) {
        qDebug() << "voxelSize is not reasonable, skipping decompressions."
                 << voxelXSize << voxelYSize << voxelZSize << getName() << getID();
        _voxelDataDirty = false;
        _voxelDataLock.unlock();
        _threadRunning.release();
        return;
    }

    int rawSize = voxelXSize * voxelYSize * voxelZSize;

    QByteArray compressedData;
    reader >> compressedData;
    _voxelDataDirty = false;
    _voxelDataLock.unlock();
    QByteArray uncompressedData = qUncompress(compressedData);

    if (uncompressedData.size() != rawSize) {
        qDebug() << "PolyVox decompress -- size is (" << voxelXSize << voxelYSize << voxelZSize << ")"
                 << "so expected uncompressed length of" << rawSize << "but length is" << uncompressedData.size()
                 << getName() << getID();
        _threadRunning.release();
        return;
    }

    _volDataLock.lockForWrite();
    if (!_volData) {
        _volDataLock.unlock();
        _threadRunning.release();
        return;
    }
    _volDataDirty = true;
    for (int z = 0; z < voxelZSize; z++) {
        for (int y = 0; y < voxelYSize; y++) {
            for (int x = 0; x < voxelXSize; x++) {
                int uncompressedIndex = (z * voxelYSize * voxelXSize) + (y * voxelZSize) + x;
                setVoxelInternal(x, y, z, uncompressedData[uncompressedIndex]);
            }
        }
    }
    _volDataLock.unlock();
    _threadRunning.release();
}

void RenderablePolyVoxEntityItem::compressVolumeDataAndSendEditPacket() {
    _threadRunning.acquire();
    QtConcurrent::run(this, &RenderablePolyVoxEntityItem::compressVolumeDataAndSendEditPacketAsync);
}


// compress the data in _volData and save the results.  The compressed form is used during
// saves to disk and for transmission over the wire
void RenderablePolyVoxEntityItem::compressVolumeDataAndSendEditPacketAsync() {
    quint16 voxelXSize = _voxelVolumeSize.x;
    quint16 voxelYSize = _voxelVolumeSize.y;
    quint16 voxelZSize = _voxelVolumeSize.z;
    int rawSize = voxelXSize * voxelYSize * voxelZSize;

    QByteArray uncompressedData = QByteArray(rawSize, '\0');

    _volDataLock.lockForRead();
    for (int z = 0; z < voxelZSize; z++) {
        for (int y = 0; y < voxelYSize; y++) {
            for (int x = 0; x < voxelXSize; x++) {
                uint8_t uVoxelValue = getVoxelInternal(x, y, z);
                int uncompressedIndex =
                    z * voxelYSize * voxelXSize +
                    y * voxelXSize +
                    x;
                uncompressedData[uncompressedIndex] = uVoxelValue;
            }
        }
    }
    _volDataLock.unlock();

    QByteArray newVoxelData;
    QDataStream writer(&newVoxelData, QIODevice::WriteOnly | QIODevice::Truncate);

    writer << voxelXSize << voxelYSize << voxelZSize;

    QByteArray compressedData = qCompress(uncompressedData, 9);
    writer << compressedData;

    // make sure the compressed data can be sent over the wire-protocol
    if (newVoxelData.size() > 1150) {
        // HACK -- until we have a way to allow for properties larger than MTU, don't update.
        // revert the active voxel-space to the last version that fit.
        // XXX
        qDebug() << "compressed voxel data is too large" << getName() << getID();
        _threadRunning.release();
        return;
    }

    auto now = usecTimestampNow();
    setLastEdited(now);
    setLastBroadcast(now);

    _voxelDataLock.lockForWrite();
    _voxelDataDirty = true;
    _voxelData = newVoxelData;
    _voxelDataLock.unlock();

    EntityItemProperties properties = getProperties();
    properties.setVoxelDataDirty();
    properties.setLastEdited(now);

    EntityTreeElement* element = getElement();
    EntityTree* tree = element ? element->getTree() : nullptr;
    EntitySimulation* simulation = tree ? tree->getSimulation() : nullptr;
    PhysicalEntitySimulation* peSimulation = static_cast<PhysicalEntitySimulation*>(simulation);
    EntityEditPacketSender* packetSender = peSimulation ? peSimulation->getPacketSender() : nullptr;
    if (packetSender) {
        packetSender->queueEditEntityMessage(PacketType::EntityEdit, _id, properties);
    }
    _threadRunning.release();
}

void RenderablePolyVoxEntityItem::getMesh() {
    _threadRunning.acquire();
    QtConcurrent::run(this, &RenderablePolyVoxEntityItem::getMeshAsync);
}


void RenderablePolyVoxEntityItem::clearOutOfDateNeighbors() {
    if (_xNeighborID != UNKNOWN_ENTITY_ID) {
        EntityItemPointer currentXNeighbor = _xNeighbor.lock();
        if (currentXNeighbor && currentXNeighbor->getID() != _xNeighborID) {
            _xNeighbor.reset();
        }
    }
    if (_yNeighborID != UNKNOWN_ENTITY_ID) {
        EntityItemPointer currentYNeighbor = _yNeighbor.lock();
        if (currentYNeighbor && currentYNeighbor->getID() != _yNeighborID) {
            _yNeighbor.reset();
        }
    }
    if (_zNeighborID != UNKNOWN_ENTITY_ID) {
        EntityItemPointer currentZNeighbor = _zNeighbor.lock();
        if (currentZNeighbor && currentZNeighbor->getID() != _zNeighborID) {
            _zNeighbor.reset();
        }
    }
}

void RenderablePolyVoxEntityItem::cacheNeighbors() {
    if (_voxelSurfaceStyle != PolyVoxEntityItem::SURFACE_CUBIC &&
        _voxelSurfaceStyle != PolyVoxEntityItem::SURFACE_MARCHING_CUBES) {
        return;
    }

    clearOutOfDateNeighbors();
    EntityTreeElement* element = getElement();
    EntityTree* tree = element ? element->getTree() : nullptr;
    if (!tree) {
        return;
    }

    if (_xNeighborID != UNKNOWN_ENTITY_ID && _xNeighbor.expired()) {
        _xNeighbor = tree->findEntityByID(_xNeighborID);
    }
    if (_yNeighborID != UNKNOWN_ENTITY_ID && _yNeighbor.expired()) {
        _yNeighbor = tree->findEntityByID(_yNeighborID);
    }
    if (_zNeighborID != UNKNOWN_ENTITY_ID && _zNeighbor.expired()) {
        _zNeighbor = tree->findEntityByID(_zNeighborID);
    }
}

void RenderablePolyVoxEntityItem::copyUpperEdgesFromNeighbors() {
    if (_voxelSurfaceStyle != PolyVoxEntityItem::SURFACE_CUBIC &&
        _voxelSurfaceStyle != PolyVoxEntityItem::SURFACE_MARCHING_CUBES) {
        return;
    }

    EntityItemPointer currentXNeighbor = _xNeighbor.lock();
    EntityItemPointer currentYNeighbor = _yNeighbor.lock();
    EntityItemPointer currentZNeighbor = _zNeighbor.lock();

    if (currentXNeighbor) {
        auto polyVoxXNeighbor = std::dynamic_pointer_cast<PolyVoxEntityItem>(currentXNeighbor);
        for (int y = 0; y < _voxelVolumeSize.y; y++) {
            for (int z = 0; z < _voxelVolumeSize.z; y++) {
                uint8_t neighborValue = polyVoxXNeighbor->getVoxel(0, y, z);
                _volData->setVoxelAt(_voxelVolumeSize.x, y, z, neighborValue);
            }
        }
    }

    if (currentYNeighbor) {
        auto polyVoxYNeighbor = std::dynamic_pointer_cast<PolyVoxEntityItem>(currentYNeighbor);
        for (int x = 0; x < _voxelVolumeSize.x; x++) {
            for (int z = 0; z < _voxelVolumeSize.z; z++) {
                uint8_t neighborValue = polyVoxYNeighbor->getVoxel(x, 0, z);
                _volData->setVoxelAt(x, _voxelVolumeSize.y, z, neighborValue);
            }
        }
    }

    if (currentZNeighbor) {
        auto polyVoxZNeighbor = std::dynamic_pointer_cast<PolyVoxEntityItem>(currentZNeighbor);
        for (int x = 0; x < _voxelVolumeSize.x; x++) {
            for (int y = 0; y < _voxelVolumeSize.y; y++) {
                uint8_t neighborValue = polyVoxZNeighbor->getVoxel(x, y, 0);
                _volData->setVoxelAt(x, y, _voxelVolumeSize.z, neighborValue);
            }
        }
    }
}

void RenderablePolyVoxEntityItem::getMeshAsync() {
    model::MeshPointer mesh(new model::Mesh());

    cacheNeighbors();
    copyUpperEdgesFromNeighbors();

    // A mesh object to hold the result of surface extraction
    PolyVox::SurfaceMesh<PolyVox::PositionMaterialNormal> polyVoxMesh;

    _volDataLock.lockForRead();
    switch (_voxelSurfaceStyle) {
        case PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES:
        case PolyVoxEntityItem::SURFACE_MARCHING_CUBES: {
            PolyVox::MarchingCubesSurfaceExtractor<PolyVox::SimpleVolume<uint8_t>> surfaceExtractor
                (_volData, _volData->getEnclosingRegion(), &polyVoxMesh);
            surfaceExtractor.execute();
            break;
        }
        case PolyVoxEntityItem::SURFACE_EDGED_CUBIC:
        case PolyVoxEntityItem::SURFACE_CUBIC: {
            PolyVox::CubicSurfaceExtractorWithNormals<PolyVox::SimpleVolume<uint8_t>> surfaceExtractor
                (_volData, _volData->getEnclosingRegion(), &polyVoxMesh);
            surfaceExtractor.execute();
            break;
        }
    }

    // convert PolyVox mesh to a Sam mesh
    const std::vector<uint32_t>& vecIndices = polyVoxMesh.getIndices();
    auto indexBuffer = std::make_shared<gpu::Buffer>(vecIndices.size() * sizeof(uint32_t),
                                                     (gpu::Byte*)vecIndices.data());
    auto indexBufferPtr = gpu::BufferPointer(indexBuffer);
    auto indexBufferView = new gpu::BufferView(indexBufferPtr, gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW));
    mesh->setIndexBuffer(*indexBufferView);

    const std::vector<PolyVox::PositionMaterialNormal>& vecVertices = polyVoxMesh.getVertices();
    auto vertexBuffer = std::make_shared<gpu::Buffer>(vecVertices.size() * sizeof(PolyVox::PositionMaterialNormal),
                                                      (gpu::Byte*)vecVertices.data());
    auto vertexBufferPtr = gpu::BufferPointer(vertexBuffer);
    gpu::Resource::Size vertexBufferSize = 0;
    if (vertexBufferPtr->getSize() > sizeof(float) * 3) {
        vertexBufferSize = vertexBufferPtr->getSize() - sizeof(float) * 3;
    }
    auto vertexBufferView = new gpu::BufferView(vertexBufferPtr, 0, vertexBufferSize, sizeof(PolyVox::PositionMaterialNormal),
                                                gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW));
    mesh->setVertexBuffer(*vertexBufferView);
    mesh->addAttribute(gpu::Stream::NORMAL,
                       gpu::BufferView(vertexBufferPtr,
                                       sizeof(float) * 3,
                                       vertexBufferPtr->getSize() - sizeof(float) * 3,
                                       sizeof(PolyVox::PositionMaterialNormal),
                                       gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW)));

    _meshLock.lockForWrite();
    _dirtyFlags |= EntityItem::DIRTY_SHAPE | EntityItem::DIRTY_MASS;
    _mesh = mesh;
    _meshDirty = true;
    _meshLock.unlock();
    _volDataDirty = false;
    _volDataLock.unlock();

    _threadRunning.release();
}

void RenderablePolyVoxEntityItem::computeShapeInfoWorker() {
    _threadRunning.acquire();
    QtConcurrent::run(this, &RenderablePolyVoxEntityItem::computeShapeInfoWorkerAsync);
}


void RenderablePolyVoxEntityItem::computeShapeInfoWorkerAsync() {
    QVector<QVector<glm::vec3>> points;
    AABox box;
    glm::mat4 vtoM = voxelToLocalMatrix();

    if (_voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_MARCHING_CUBES ||
        _voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_MARCHING_CUBES) {
        /* pull each triangle in the mesh into a polyhedron which can be collided with */
        unsigned int i = 0;

        _meshLock.lockForRead();
        model::MeshPointer mesh = _mesh;
        const gpu::BufferView vertexBufferView = mesh->getVertexBuffer();
        const gpu::BufferView& indexBufferView = mesh->getIndexBuffer();
        _meshLock.unlock();

        gpu::BufferView::Iterator<const uint32_t> it = indexBufferView.cbegin<uint32_t>();
        while (it != indexBufferView.cend<uint32_t>()) {
            uint32_t p0Index = *(it++);
            uint32_t p1Index = *(it++);
            uint32_t p2Index = *(it++);

            const glm::vec3& p0 = vertexBufferView.get<const glm::vec3>(p0Index);
            const glm::vec3& p1 = vertexBufferView.get<const glm::vec3>(p1Index);
            const glm::vec3& p2 = vertexBufferView.get<const glm::vec3>(p2Index);

            glm::vec3 av = (p0 + p1 + p2) / 3.0f; // center of the triangular face
            glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
            glm::vec3 p3 = av - normal * MARCHING_CUBE_COLLISION_HULL_OFFSET;

            glm::vec3 p0Model = glm::vec3(vtoM * glm::vec4(p0, 1.0f));
            glm::vec3 p1Model = glm::vec3(vtoM * glm::vec4(p1, 1.0f));
            glm::vec3 p2Model = glm::vec3(vtoM * glm::vec4(p2, 1.0f));
            glm::vec3 p3Model = glm::vec3(vtoM * glm::vec4(p3, 1.0f));

            box += p0Model;
            box += p1Model;
            box += p2Model;
            box += p3Model;

            QVector<glm::vec3> pointsInPart;
            pointsInPart << p0Model;
            pointsInPart << p1Model;
            pointsInPart << p2Model;
            pointsInPart << p3Model;
            // add next convex hull
            QVector<glm::vec3> newMeshPoints;
            points << newMeshPoints;
            // add points to the new convex hull
            points[i++] << pointsInPart;
        }
    } else {
        unsigned int i = 0;

        for (int z = 0; z < _voxelVolumeSize.z; z++) {
            for (int y = 0; y < _voxelVolumeSize.y; y++) {
                for (int x = 0; x < _voxelVolumeSize.x; x++) {
                    if (getVoxel(x, y, z) > 0) {

                        if ((x > 0 && getVoxel(x - 1, y, z) > 0) &&
                            (y > 0 && getVoxel(x, y - 1, z) > 0) &&
                            (z > 0 && getVoxel(x, y, z - 1) > 0) &&
                            (x < _voxelVolumeSize.x - 1 && getVoxel(x + 1, y, z) > 0) &&
                            (y < _voxelVolumeSize.y - 1 && getVoxel(x, y + 1, z) > 0) &&
                            (z < _voxelVolumeSize.z - 1 && getVoxel(x, y, z + 1) > 0)) {
                            // this voxel has neighbors in every cardinal direction, so there's no need
                            // to include it in the collision hull.
                            continue;
                        }

                        QVector<glm::vec3> pointsInPart;

                        float offL = -0.5f;
                        float offH = 0.5f;
                        if (_voxelSurfaceStyle == PolyVoxEntityItem::SURFACE_EDGED_CUBIC) {
                            offL += 1.0f;
                            offH += 1.0f;
                        }

                        glm::vec3 p000 = glm::vec3(vtoM * glm::vec4(x + offL, y + offL, z + offL, 1.0f));
                        glm::vec3 p001 = glm::vec3(vtoM * glm::vec4(x + offL, y + offL, z + offH, 1.0f));
                        glm::vec3 p010 = glm::vec3(vtoM * glm::vec4(x + offL, y + offH, z + offL, 1.0f));
                        glm::vec3 p011 = glm::vec3(vtoM * glm::vec4(x + offL, y + offH, z + offH, 1.0f));
                        glm::vec3 p100 = glm::vec3(vtoM * glm::vec4(x + offH, y + offL, z + offL, 1.0f));
                        glm::vec3 p101 = glm::vec3(vtoM * glm::vec4(x + offH, y + offL, z + offH, 1.0f));
                        glm::vec3 p110 = glm::vec3(vtoM * glm::vec4(x + offH, y + offH, z + offL, 1.0f));
                        glm::vec3 p111 = glm::vec3(vtoM * glm::vec4(x + offH, y + offH, z + offH, 1.0f));

                        box += p000;
                        box += p001;
                        box += p010;
                        box += p011;
                        box += p100;
                        box += p101;
                        box += p110;
                        box += p111;

                        pointsInPart << p000;
                        pointsInPart << p001;
                        pointsInPart << p010;
                        pointsInPart << p011;
                        pointsInPart << p100;
                        pointsInPart << p101;
                        pointsInPart << p110;
                        pointsInPart << p111;

                        // add next convex hull
                        QVector<glm::vec3> newMeshPoints;
                        points << newMeshPoints;
                        // add points to the new convex hull
                        points[i++] << pointsInPart;
                    }
                }
            }
        }
    }

    if (points.isEmpty()) {
        _shapeInfoLock.lockForWrite();
        EntityItem::computeShapeInfo(_shapeInfo);
        _shapeInfoLock.unlock();
        _threadRunning.release();
        return;
    }

    glm::vec3 collisionModelDimensions = box.getDimensions();
    QByteArray b64 = _voxelData.toBase64();
    _shapeInfoLock.lockForWrite();
    _shapeInfo.setParams(SHAPE_TYPE_COMPOUND, collisionModelDimensions, QString(b64));
    _shapeInfo.setConvexHulls(points);
    _shapeInfoLock.unlock();

    _meshLock.lockForWrite();
    _meshDirty = false;
    _meshLock.unlock();
    _threadRunning.release();
    return;
}
