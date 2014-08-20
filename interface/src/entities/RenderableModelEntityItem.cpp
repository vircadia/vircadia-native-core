//
//  EntityTreeRenderer.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
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
    qDebug() << "RenderableModelEntityItem::factory(const EntityItemID& entityItemID, const EntityItemProperties& properties)...";
    return new RenderableModelEntityItem(entityID, properties);
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
    float radius = getRadius() * (float)TREE_SCALE;
    float size = getSize() * (float)TREE_SCALE;
    
    if (drawAsModel) {
        glPushMatrix();
        {
            const float alpha = 1.0f;
        
            if (!_model || _needsModelReload) {
                // TODO: this getModel() appears to be about 3% of model render time. We should optimize
                PerformanceTimer perfTimer("getModel");
                getModel();
            }
            
            if (_model) {
                // handle animations..
                if (hasAnimation()) {
                    //qDebug() << "hasAnimation()...";                
                    if (!jointsMapped()) {
                        QStringList modelJointNames = _model->getJointNames();
                        mapJoints(modelJointNames);
                        //qDebug() << "mapJoints()...";                
                    }

                    if (jointsMapped()) {
                        //qDebug() << "_model->setJointState()...";
                        QVector<glm::quat> frameData = getAnimationFrame();
                        for (int i = 0; i < frameData.size(); i++) {
                            _model->setJointState(i, true, frameData[i]);
                        }
                    }
                }

                glm::quat rotation = getRotation();
                if (_needsSimulation && _model->isActive()) {
                    _model->setScaleToFit(true, radius * 2.0f);
                    _model->setSnapModelToCenter(true);
                    _model->setRotation(rotation);
                    _model->setTranslation(position);
                    
                    // make sure to simulate so everything gets set up correctly for rendering
                    {
                        // TODO: _model->simulate() appears to be about 28% of model render time.
                        // do we really need to call this every frame? I think not. Look into how to
                        // reduce calls to this.
                        PerformanceTimer perfTimer("_model->simulate");
                        _model->simulate(0.0f);
                    }
                    _needsSimulation = false;
                }

                // TODO: should we allow entityItems to have alpha on their models?
                Model::RenderMode modelRenderMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE 
                                                        ? Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
        
                if (_model->isActive()) {
                    // TODO: this appears to be about 62% of model render time. Is there a way to call this that doesn't 
                    // cost us as much? For example if the same model is used but rendered multiple places is it less
                    // expensive?
                    PerformanceTimer perfTimer("model->render");
                    _model->render(alpha, modelRenderMode);
                } else {
                    // if we couldn't get a model, then just draw a sphere
                    glColor3ub(getColor()[RED_INDEX],getColor()[GREEN_INDEX],getColor()[BLUE_INDEX]);
                    glPushMatrix();
                        glTranslatef(position.x, position.y, position.z);
                        glutSolidSphere(radius, 15, 15);
                    glPopMatrix();
                }
                
                bool isShadowMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE;
                bool displayModelBounds = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelBounds);

                if (!isShadowMode && displayModelBounds) {
                    PerformanceTimer perfTimer("displayModelBounds");

                    glm::vec3 unRotatedMinimum = _model->getUnscaledMeshExtents().minimum;
                    glm::vec3 unRotatedMaximum = _model->getUnscaledMeshExtents().maximum;
                    glm::vec3 unRotatedExtents = unRotatedMaximum - unRotatedMinimum;
 
                    float width = unRotatedExtents.x;
                    float height = unRotatedExtents.y;
                    float depth = unRotatedExtents.z;

                    Extents rotatedExtents = _model->getUnscaledMeshExtents();
                    calculateRotatedExtents(rotatedExtents, rotation);

                    glm::vec3 rotatedSize = rotatedExtents.maximum - rotatedExtents.minimum;

                    const glm::vec3& modelScale = _model->getScale();

                    glPushMatrix();
                        glTranslatef(position.x, position.y, position.z);
                    
                        // draw the orignal bounding cube
                        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
                        glutWireCube(size);

                        // draw the rotated bounding cube
                        glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
                        glPushMatrix();
                            glScalef(rotatedSize.x * modelScale.x, rotatedSize.y * modelScale.y, rotatedSize.z * modelScale.z);
                            glutWireCube(1.0);
                        glPopMatrix();
                    
                        // draw the model relative bounding box
                        glm::vec3 axis = glm::axis(rotation);
                        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
                        glScalef(width * modelScale.x, height * modelScale.y, depth * modelScale.z);
                        glColor3f(0.0f, 1.0f, 0.0f);
                        glutWireCube(1.0);

                    glPopMatrix();
                
                }
            } else {
                // if we couldn't get a model, then just draw a sphere
                glColor3ub(getColor()[RED_INDEX],getColor()[GREEN_INDEX],getColor()[BLUE_INDEX]);
                glPushMatrix();
                    glTranslatef(position.x, position.y, position.z);
                    glutSolidSphere(radius, 15, 15);
                glPopMatrix();
            }
        }
        glPopMatrix();
    } else {
        glColor3f(1.0f, 0.0f, 0.0f);
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glutSolidSphere(radius, 15, 15);
        glPopMatrix();
    }
}

Model* RenderableModelEntityItem::getModel() {
    _needsModelReload = false; // this is the reload
    if (!getModelURL().isEmpty()) {
        // double check our URLS match...
        if (_model && QUrl(getModelURL()) != _model->getURL()) {
            delete _model; // delete the old model...
            _model = NULL;
            _needsSimulation = true;
        }

        // if we don't have a model... but our item does have a model URL
        if (!_model) {
            // Make sure we only create new models on the thread that owns the EntityTreeRenderer
            if (QThread::currentThread() != EntityTreeRenderer::getMainThread()) {
            
                // TODO: how do we better handle this??? we may need this for scripting...
                // possible go back to a getModel() service on the TreeRenderer, but have it take a URL
                // this would allow the entity items to use that service for loading, and since the
                // EntityTreeRenderer is a Q_OBJECT it can call invokeMethod()
                
                qDebug() << "can't call getModel() on thread other than rendering thread...";
                //qDebug() << "about to call QMetaObject::invokeMethod(this, 'getModel', Qt::BlockingQueuedConnection,...";
                //QMetaObject::invokeMethod(this, "getModel", Qt::BlockingQueuedConnection, Q_RETURN_ARG(Model*, _model));
                //qDebug() << "got it... _model=" << _model;
                return _model;
            }

            _model = new Model();
            _model->init();
            _model->setURL(QUrl(getModelURL()));
            _needsSimulation = true;
        }
    } else {
        // our URL is empty, we should clean up our old model
        if (_model) {
            delete _model; // delete the old model...
            _model = NULL;
            _needsSimulation = true;
        }
    }
    return _model;
}





