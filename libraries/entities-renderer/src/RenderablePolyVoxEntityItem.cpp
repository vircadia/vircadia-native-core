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

#include "EntityTreeRenderer.h"
#include "RenderablePolyVoxEntityItem.h"

EntityItem* RenderablePolyVoxEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderablePolyVoxEntityItem(entityID, properties);
}


void createSphereInVolume(PolyVox::SimpleVolume<uint8_t>& volData, float fRadius) {
    // This vector hold the position of the center of the volume
    PolyVox::Vector3DFloat v3dVolCenter(volData.getWidth() / 2, volData.getHeight() / 2, volData.getDepth() / 2);

    // This three-level for loop iterates over every voxel in the volume
    for (int z = 0; z < volData.getDepth(); z++) {
        for (int y = 0; y < volData.getHeight(); y++) {
            for (int x = 0; x < volData.getWidth(); x++) {
                // Store our current position as a vector...
                PolyVox::Vector3DFloat v3dCurrentPos(x,y,z);	
                // And compute how far the current position is from the center of the volume
                float fDistToCenter = (v3dCurrentPos - v3dVolCenter).length();

                uint8_t uVoxelValue = 0;

                // If the current voxel is less than 'radius' units from the center then we make it solid.
                if(fDistToCenter <= fRadius) {
                    // Our new voxel value
                    uVoxelValue = 255;
                }

                // Wrte the voxel value into the volume	
                volData.setVoxelAt(x, y, z, uVoxelValue);
            }
        }
    }
}



void RenderablePolyVoxEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("PolyVoxrender");
    assert(getType() == EntityTypes::PolyVox);
    
    bool drawAsModel = hasModel();

    glm::vec3 position = getPosition();
    glm::vec3 dimensions = getDimensions();

    bool didDraw = false;
    if (drawAsModel) {
        glPushMatrix();
        {
            float alpha = getLocalRenderAlpha();

            if (!_model || _needsModelReload) {
                // TODO: this getModel() appears to be about 3% of model render time. We should optimize
                PerformanceTimer perfTimer("getModel");
                EntityTreeRenderer* renderer = static_cast<EntityTreeRenderer*>(args->_renderer);
                getModel(renderer);
            }
            
            if (_model) {
                glm::quat rotation = getRotation();
                bool movingOrAnimating = isMoving();
                if ((movingOrAnimating || _needsInitialSimulation) && _model->isActive()) {
                    _model->setScaleToFit(true, dimensions);
                    _model->setSnapModelToRegistrationPoint(true, getRegistrationPoint());
                    _model->setRotation(rotation);
                    _model->setTranslation(position);
                    
                    // make sure to simulate so everything gets set up correctly for rendering
                    {
                        PerformanceTimer perfTimer("_model->simulate");
                        _model->simulate(0.0f);
                    }
                    _needsInitialSimulation = false;
                }

                if (_model->isActive()) {
                    // TODO: this is the majority of model render time. And rendering of a cube model vs the basic Box render
                    // is significantly more expensive. Is there a way to call this that doesn't cost us as much? 
                    PerformanceTimer perfTimer("model->render");
                    // filter out if not needed to render
                    if (args && (args->_renderMode == RenderArgs::SHADOW_RENDER_MODE)) {
                        if (movingOrAnimating) {
                            _model->renderInScene(alpha, args);
                            didDraw = true;
                        }
                    } else {
                        _model->renderInScene(alpha, args);
                        didDraw = true;
                    }
                }
            }
        }
        glPopMatrix();
    }

    if (!didDraw) {
        glm::vec4 greenColor(0.0f, 1.0f, 0.0f, 1.0f);
        RenderableDebugableEntityItem::renderBoundingBox(this, args, 0.0f, greenColor);
    }

    RenderableDebugableEntityItem::render(this, args);
}


Model* RenderablePolyVoxEntityItem::getModel(EntityTreeRenderer* renderer) {
    PolyVox::SimpleVolume<uint8_t> volData(PolyVox::Region(PolyVox::Vector3DInt32(0,0,0),
                                                           PolyVox::Vector3DInt32(63, 63, 63)));
    createSphereInVolume(volData, 15);

    // A mesh object to hold the result of surface extraction
    PolyVox::SurfaceMesh<PolyVox::PositionMaterialNormal> mesh;

    //Create a surface extractor. Comment out one of the following two lines to decide which type gets created.
    PolyVox::CubicSurfaceExtractorWithNormals<PolyVox::SimpleVolume<uint8_t>> surfaceExtractor
        (&volData, volData.getEnclosingRegion(), &mesh);
    // MarchingCubesSurfaceExtractor<SimpleVolume<uint8_t>> surfaceExtractor(&volData,
    //                                                                       volData.getEnclosingRegion(),
    //                                                                       &mesh);

    //Execute the surface extractor.
    surfaceExtractor.execute();

    const std::vector<uint32_t>& vecIndices = mesh.getIndices();
    const std::vector<PolyVox::PositionMaterialNormal>& vecVertices = mesh.getVertices();

    qDebug() << "-------------XXXXXXXXXXXXXXXXXXXX-------------------";
    qDebug() << "---- vecIndices.size() =" << vecIndices.size();
    qDebug() << "---- vecVertices.size() =" << vecVertices.size();


    // [DEBUG] [05/19 20:46:38] ---- vecIndices.size() = 101556
    // [DEBUG] [05/19 20:46:38] ---- vecVertices.size() = 67704


    Model* result = NULL;
    // make sure our renderer is setup
    if (!_myRenderer) {
        _myRenderer = renderer;
    }
    assert(_myRenderer == renderer); // you should only ever render on one renderer

    // result = _model = _myRenderer->allocateModel("", "");
    // assert(_model);

    _needsInitialSimulation = true;

    return result;
}
