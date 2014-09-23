//
//  RenderableModelEntityItem.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 8/6/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <FBXReader.h>

#include "InterfaceConfig.h"

#include <BoxEntityItem.h>
#include <ModelEntityItem.h>
#include <PerfStat.h>


#include "Menu.h"
#include "EntityTreeRenderer.h"
#include "RenderableModelEntityItem.h"

EntityItem* RenderableModelEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new RenderableModelEntityItem(entityID, properties);
}

RenderableModelEntityItem::~RenderableModelEntityItem() {
    assert(_myRenderer || !_model); // if we have a model, we need to know our renderer
    if (_myRenderer && _model) {
        _myRenderer->releaseModel(_model);
        _model = NULL;
    }
}

bool RenderableModelEntityItem::setProperties(const EntityItemProperties& properties, bool forceCopy) {
    QString oldModelURL = getModelURL();
    bool somethingChanged = ModelEntityItem::setProperties(properties, forceCopy);
    if (somethingChanged && oldModelURL != getModelURL()) {
        _needsModelReload = true;
    }
    return somethingChanged;
}

int RenderableModelEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    QString oldModelURL = getModelURL();
    int bytesRead = ModelEntityItem::readEntitySubclassDataFromBuffer(data, bytesLeftToRead, 
                                                                        args, propertyFlags, overwriteLocalData);
    if (oldModelURL != getModelURL()) {
        _needsModelReload = true;
    }
    return bytesRead;
}


void RenderableModelEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableModelEntityItem::render");
    assert(getType() == EntityTypes::Model);
    
    bool drawAsModel = hasModel();

    glm::vec3 position = getPosition() * (float)TREE_SCALE;
    float size = getSize() * (float)TREE_SCALE;
    glm::vec3 dimensions = getDimensions() * (float)TREE_SCALE;
    
    if (drawAsModel) {
        glPushMatrix();
        {
            const float alpha = 1.0f;
        
            if (!_model || _needsModelReload) {
                // TODO: this getModel() appears to be about 3% of model render time. We should optimize
                PerformanceTimer perfTimer("getModel");
                EntityTreeRenderer* renderer = static_cast<EntityTreeRenderer*>(args->_renderer);
                getModel(renderer);
            }
            
            if (_model) {
                // handle animations..
                if (hasAnimation()) {
                    if (!jointsMapped()) {
                        QStringList modelJointNames = _model->getJointNames();
                        mapJoints(modelJointNames);
                    }

                    if (jointsMapped()) {
                        QVector<glm::quat> frameData = getAnimationFrame();
                        for (int i = 0; i < frameData.size(); i++) {
                            _model->setJointState(i, true, frameData[i]);
                        }
                    }
                }

                glm::quat rotation = getRotation();
                if (needsSimulation() && _model->isActive()) {
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

                // TODO: should we allow entityItems to have alpha on their models?
                Model::RenderMode modelRenderMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE 
                                                        ? Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
        
                if (_model->isActive()) {
                    // TODO: this is the majority of model render time. And rendering of a cube model vs the basic Box render
                    // is significantly more expensive. Is there a way to call this that doesn't cost us as much? 
                    PerformanceTimer perfTimer("model->render");
                    _model->render(alpha, modelRenderMode);
                } else {
                    // if we couldn't get a model, then just draw a cube
                    glColor3ub(getColor()[RED_INDEX],getColor()[GREEN_INDEX],getColor()[BLUE_INDEX]);
                    glPushMatrix();
                        glTranslatef(position.x, position.y, position.z);
                        Application::getInstance()->getDeferredLightingEffect()->renderWireCube(size);
                    glPopMatrix();
                }
            } else {
                // if we couldn't get a model, then just draw a cube
                glColor3ub(getColor()[RED_INDEX],getColor()[GREEN_INDEX],getColor()[BLUE_INDEX]);
                glPushMatrix();
                    glTranslatef(position.x, position.y, position.z);
                    Application::getInstance()->getDeferredLightingEffect()->renderWireCube(size);
                glPopMatrix();
            }
        }
        glPopMatrix();
    } else {
        glColor3ub(getColor()[RED_INDEX],getColor()[GREEN_INDEX],getColor()[BLUE_INDEX]);
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        Application::getInstance()->getDeferredLightingEffect()->renderWireCube(size);
        glPopMatrix();
    }
}

Model* RenderableModelEntityItem::getModel(EntityTreeRenderer* renderer) {
    Model* result = NULL;

    // make sure our renderer is setup
    if (!_myRenderer) {
        _myRenderer = renderer;
    }
    assert(_myRenderer == renderer); // you should only ever render on one renderer
    
    _needsModelReload = false; // this is the reload

    // if we have a URL, then we will want to end up returning a model...
    if (!getModelURL().isEmpty()) {
    
        // if we have a previously allocated model, but it's URL doesn't match
        // then we need to let our renderer update our model for us.
        if (_model && QUrl(getModelURL()) != _model->getURL()) {
            result = _model = _myRenderer->updateModel(_model, getModelURL());
            _needsInitialSimulation = true;
        } else if (!_model) { // if we don't yet have a model, then we want our renderer to allocate one
            result = _model = _myRenderer->allocateModel(getModelURL());
            _needsInitialSimulation = true;
        } else { // we already have the model we want...
            result = _model;
        }
    } else { // if our desired URL is empty, we may need to delete our existing model
        if (_model) {
            _myRenderer->releaseModel(_model);
            result = _model = NULL;
            _needsInitialSimulation = true;
        }
    }
    
    return result;
}

bool RenderableModelEntityItem::needsSimulation() const {
    SimulationState simulationState = getSimulationState();
    return _needsInitialSimulation || simulationState == Moving || simulationState == Changing;
}





