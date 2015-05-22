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

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>

#include <DeferredLightingEffect.h>
#include <Model.h>
#include <PerfStat.h>

#include <PolyVoxCore/CubicSurfaceExtractorWithNormals.h>
#include <PolyVoxCore/MarchingCubesSurfaceExtractor.h>
#include <PolyVoxCore/SurfaceMesh.h>
#include <PolyVoxCore/SimpleVolume.h>

#include "model/Geometry.h"
#include "gpu/GLBackend.h"
#include "EntityTreeRenderer.h"
#include "RenderablePolyVoxEntityItem.h"

EntityItem* RenderablePolyVoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderablePolyVoxEntityItem(entityID, properties);
}

RenderablePolyVoxEntityItem::~RenderablePolyVoxEntityItem() {
    delete _volData;
}

void RenderablePolyVoxEntityItem::setVoxelVolumeSize(glm::vec3 voxelVolumeSize) {
    PolyVoxEntityItem::setVoxelVolumeSize(voxelVolumeSize);

    if (_volData) {
        delete _volData;
    }

    PolyVox::Vector3DInt32 lowCorner(0, 0, 0);
    PolyVox::Vector3DInt32 highCorner(_voxelVolumeSize[0] - 1, // -1 because these corners are inclusive
                                      _voxelVolumeSize[1] - 1,
                                      _voxelVolumeSize[2] - 1);

    _volData = new PolyVox::SimpleVolume<uint8_t>(PolyVox::Region(lowCorner, highCorner));
}

glm::vec3 RenderablePolyVoxEntityItem::metersToVoxelCoordinates(glm::vec3 metersOffCenter) {
    // convert a vector from the center of the entity with units of meters to a vector with units
    // of index-into-volData.
    glm::vec3 dimensions = getDimensions();
    glm::vec3 scale = dimensions / _voxelVolumeSize; // meters / voxel-units
    glm::vec3 voxelCoords = metersOffCenter / scale;
    // the voxel volume is entirely in positive space, but the voxel space is centered at the entity's coords, so:
    return voxelCoords + _voxelVolumeSize / 2.0f;
}

glm::vec3 RenderablePolyVoxEntityItem::voxelCoordinatesToMeters(glm::vec3 voxelCoords) {
    // convert a vector is voxel-space to an offset from the entity-center in meters
    glm::vec3 dimensions = getDimensions();
    glm::vec3 scale = dimensions / _voxelVolumeSize; // meters / voxel-units
    glm::vec3 centeredCoords = voxelCoords - _voxelVolumeSize / 2.0f;
    return centeredCoords * scale;
}

void RenderablePolyVoxEntityItem::setSphereInVolume(glm::vec3 center, float radius, uint8_t toValue) {
    // This three-level for loop iterates over every voxel in the volume
    for (int z = 0; z < _volData->getDepth(); z++) {
        for (int y = 0; y < _volData->getHeight(); y++) {
            for (int x = 0; x < _volData->getWidth(); x++) {
                // Store our current position as a vector...
                glm::vec3 pos(x, y, z);
                // And compute how far the current position is from the center of the volume
                float fDistToCenter = glm::distance(pos, center);
                uint8_t uVoxelValue = _volData->getVoxelAt(x, y, z);
                // If the current voxel is less than 'radius' units from the center then we make it solid.
                if (fDistToCenter <= radius) {
                    // Our new voxel value
                    uVoxelValue = toValue;
                }

                // Wrte the voxel value into the volume	
                _volData->setVoxelAt(x, y, z, uVoxelValue);
            }
        }
    }
}

void RenderablePolyVoxEntityItem::createSphereInVolume(glm::vec3 center, float radius) {
    setSphereInVolume(center, radius, 255);
}


void RenderablePolyVoxEntityItem::eraseSphereInVolume(glm::vec3 center, float radius) {
    setSphereInVolume(center, radius, 0);
}

void RenderablePolyVoxEntityItem::getModel() {
    if (!_volData) {
        // this will cause the allocation of _volData
        setVoxelVolumeSize(_voxelVolumeSize);
    }

    glm::vec3 center(_volData->getDepth() / 2.0f,
                     _volData->getHeight() / 2.0f,
                     _volData->getWidth() / 2.0f);
    createSphereInVolume(center, 15);
    createSphereInVolume(center + glm::vec3(8.0f, 0.0f, 0.0f), 15);
    eraseSphereInVolume(center + glm::vec3(4.0f, 0.0f, 4.0f), 15);

    // A mesh object to hold the result of surface extraction
    PolyVox::SurfaceMesh<PolyVox::PositionMaterialNormal> polyVoxMesh;

    //Create a surface extractor. Comment out one of the following two lines to decide which type gets created.
    PolyVox::CubicSurfaceExtractorWithNormals<PolyVox::SimpleVolume<uint8_t>> surfaceExtractor
        (_volData, _volData->getEnclosingRegion(), &polyVoxMesh);
    // PolyVox::MarchingCubesSurfaceExtractor<PolyVox::SimpleVolume<uint8_t>> surfaceExtractor
    //     (_volData, _volData->getEnclosingRegion(), &polyVoxMesh);

    //Execute the surface extractor.
    surfaceExtractor.execute();


    // convert PolyVox mesh to a Sam mesh
    model::Mesh* mesh = new model::Mesh();
    model::MeshPointer meshPtr(mesh);

    const std::vector<uint32_t>& vecIndices = polyVoxMesh.getIndices();
    auto indexBuffer = new gpu::Buffer(vecIndices.size() * sizeof(uint32_t), (gpu::Byte*)vecIndices.data());
    auto indexBufferPtr = gpu::BufferPointer(indexBuffer);
    mesh->setIndexBuffer(gpu::BufferView(indexBufferPtr, gpu::Element(gpu::SCALAR, gpu::UINT32, gpu::RAW)));


    const std::vector<PolyVox::PositionMaterialNormal>& vecVertices = polyVoxMesh.getVertices();
    auto vertexBuffer = new gpu::Buffer(vecVertices.size() * sizeof(PolyVox::PositionMaterialNormal),
                                        (gpu::Byte*)vecVertices.data());
    auto vertexBufferPtr = gpu::BufferPointer(vertexBuffer);
    mesh->setVertexBuffer(gpu::BufferView(vertexBufferPtr,
                                          0,
                                          vertexBufferPtr->getSize() - sizeof(float) * 3,
                                          sizeof(PolyVox::PositionMaterialNormal),
                                          gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW)));
    mesh->addAttribute(gpu::Stream::NORMAL,
                       gpu::BufferView(vertexBufferPtr,
                                       sizeof(float) * 3,
                                       vertexBufferPtr->getSize() - sizeof(float) * 3,
                                       sizeof(PolyVox::PositionMaterialNormal),
                                       gpu::Element(gpu::VEC3, gpu::FLOAT, gpu::RAW)));


    qDebug() << "-------------XXXXXXXXXXXXXXXXXXXX-------------------";
    qDebug() << "---- vecIndices.size() =" << vecIndices.size();
    qDebug() << "---- sizeof(vecIndices[0]) =" << sizeof(vecIndices[0]);
    qDebug() << "---- vecVertices.size() =" << vecVertices.size();
    qDebug() << "---- sizeof(vecVertices[0]) =" << sizeof(vecVertices[0]);
    qDebug() << "---- sizeof(uint32_t) =" << sizeof(uint32_t);
    qDebug() << "---- sizeof(float) =" << sizeof(float);
    qDebug() << "---- sizeof(PolyVox::PositionMaterialNormal) =" << sizeof(PolyVox::PositionMaterialNormal);

    //           -------------XXXXXXXXXXXXXXXXXXXX-------------------
    //           ---- vecIndices.size() = 25524
    //           ---- sizeof(vecIndices[0]) = 4
    //           ---- vecVertices.size() = 17016
    //           ---- sizeof(vecVertices[0]) = 28
    //           ---- sizeof(uint32_t) = 4
    //           ---- sizeof(float) = 4
    //           ---- sizeof(PolyVox::PositionMaterialNormal) = 28

    _modelGeometry.setMesh(meshPtr);
    _needsModelReload = false;
}



void RenderablePolyVoxEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderablePolyVoxEntityItem::render");
    assert(getType() == EntityTypes::PolyVox);

    if (_needsModelReload) {
        getModel();
    }

    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();
    glm::vec3 scale = dimensions / _voxelVolumeSize;
    glm::vec3 center = getCenter();
    glm::quat rotation = getRotation();
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glm::vec3 positionToCenter = center - position;
        // make the rendered voxel volume be centered on the entity's position
        positionToCenter -= _dimensions * glm::vec3(0.5f,0.5f,0.5f);
        glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);
        glScalef(scale.x, scale.y, scale.z);

        auto mesh = _modelGeometry.getMesh();
        gpu::Batch batch;
        batch.setInputFormat(mesh->getVertexFormat());
        batch.setInputBuffer(gpu::Stream::POSITION, mesh->getVertexBuffer());
        batch.setInputBuffer(gpu::Stream::NORMAL,
                             mesh->getVertexBuffer()._buffer,
                             sizeof(float) * 3,
                             mesh->getVertexBuffer()._stride);
        batch.setIndexBuffer(gpu::UINT32, mesh->getIndexBuffer()._buffer, 0);
        batch.drawIndexed(gpu::TRIANGLES, mesh->getNumIndices(), 0);
        gpu::GLBackend::renderBatch(batch);
    glPopMatrix();
    RenderableDebugableEntityItem::render(this, args);
}
