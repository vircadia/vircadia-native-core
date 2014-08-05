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
#include "Menu.h"
#include "EntityTreeRenderer.h"

EntityTreeRenderer::EntityTreeRenderer() :
    OctreeRenderer() {
}

EntityTreeRenderer::~EntityTreeRenderer() {
    clearModelsCache();
}

void EntityTreeRenderer::clear() {
    OctreeRenderer::clear();
    clearModelsCache();
}

void EntityTreeRenderer::clearModelsCache() {
    qDebug() << "EntityTreeRenderer::clearModelsCache()...";
    
    // delete the models in _knownEntityItemModels
    foreach(Model* model, _knownEntityItemModels) {
        delete model;
    }
    _knownEntityItemModels.clear();

    foreach(Model* model, _unknownEntityItemModels) {
        delete model;
    }
    _unknownEntityItemModels.clear();
}

void EntityTreeRenderer::init() {
    OctreeRenderer::init();
    static_cast<EntityTree*>(_tree)->setFBXService(this);
}

void EntityTreeRenderer::setTree(Octree* newTree) {
    OctreeRenderer::setTree(newTree);
    static_cast<EntityTree*>(_tree)->setFBXService(this);
}

void EntityTreeRenderer::update() {
    if (_tree) {
        EntityTree* tree = static_cast<EntityTree*>(_tree);
        tree->update();
    }
}

void EntityTreeRenderer::render(RenderMode renderMode) {
    //qDebug() << "EntityTreeRenderer::render() ************";
    OctreeRenderer::render(renderMode);
    //qDebug() << "-------------------- EntityTreeRenderer::render() ------------";
    //static_cast<EntityTree*>(_tree)->debugDumpMap();
    //qDebug() << "******* DONE ******* EntityTreeRenderer::render() ************";
}

const FBXGeometry* EntityTreeRenderer::getGeometryForEntity(const EntityItem& entityItem) {
    const FBXGeometry* result = NULL;
    
    Model* model = getModel(entityItem);
    if (model) {
        result = &model->getGeometry()->getFBXGeometry();
    }
    return result;
}

Model* EntityTreeRenderer::getModel(const EntityItem& entityItem) {
    Model* model = NULL;
 
#if 0 //def HIDE_SUBCLASS_METHODS
    if (!entityItem.getModelURL().isEmpty()) {
        if (entityItem.isKnownID()) {
            if (_knownEntityItemModels.find(entityItem.getID()) != _knownEntityItemModels.end()) {
                model = _knownEntityItemModels[entityItem.getID()];
                if (QUrl(entityItem.getModelURL()) != model->getURL()) {
                    delete model; // delete the old model...
                    model = NULL;
                    _knownEntityItemModels.remove(entityItem.getID());
                }
            }

            // if we don't have a model... but our item does have a model URL
            if (!model) {
                // Make sure we only create new models on the thread that owns the EntityTreeRenderer
                if (QThread::currentThread() != thread()) {
                    qDebug() << "about to call QMetaObject::invokeMethod(this, 'getModel', Qt::BlockingQueuedConnection,...";
                    QMetaObject::invokeMethod(this, "getModel", Qt::BlockingQueuedConnection,
                        Q_RETURN_ARG(Model*, model), Q_ARG(const EntityItem&, entityItem));
                    qDebug() << "got it... model=" << model;
                    return model;
                }

                model = new Model();
                model->init();
                model->setURL(QUrl(entityItem.getModelURL()));
                _knownEntityItemModels[entityItem.getID()] = model;
            }
            
        } else {
            if (_unknownEntityItemModels.find(entityItem.getCreatorTokenID()) != _unknownEntityItemModels.end()) {
                model = _unknownEntityItemModels[entityItem.getCreatorTokenID()];
                if (QUrl(entityItem.getModelURL()) != model->getURL()) {
                    delete model; // delete the old model...
                    model = NULL;
                    _unknownEntityItemModels.remove(entityItem.getID());
                }
            }

            if (!model) {
                // Make sure we only create new models on the thread that owns the EntityTreeRenderer
                if (QThread::currentThread() != thread()) {
                    QMetaObject::invokeMethod(this, "getModel", Qt::BlockingQueuedConnection,
                        Q_RETURN_ARG(Model*, model), Q_ARG(const EntityItem&, entityItem));
                    return model;
                }
        
                model = new Model();
                model->init();
                model->setURL(QUrl(entityItem.getModelURL()));
                _unknownEntityItemModels[entityItem.getCreatorTokenID()] = model;
            }
        }
    }
#endif
    return model;
}

void EntityTreeRenderer::renderElement(OctreeElement* element, RenderArgs* args) {
    args->_elementsTouched++;
    // actually render it here...
    // we need to iterate the actual entityItems of the element
    EntityTreeElement* entityTreeElement = static_cast<EntityTreeElement*>(element);

    QList<EntityItem*>& entityItems = entityTreeElement->getEntities();
    

    uint16_t numberOfEntities = entityItems.size();

    //qDebug() << "EntityTreeRenderer::renderElement() element=" << element << "numberOfEntities=" << numberOfEntities;
    
    bool isShadowMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE;

    bool displayModelBounds = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelBounds);
    bool displayElementProxy = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementProxy);
    bool displayElementChildProxies = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementChildProxies);


    if (!isShadowMode && displayElementProxy && numberOfEntities > 0) {
        glm::vec3 elementCenter = entityTreeElement->getAACube().calcCenter() * (float)TREE_SCALE;
        float elementSize = entityTreeElement->getScale() * (float)TREE_SCALE;
        glColor3f(1.0f, 0.0f, 0.0f);
        glPushMatrix();
            glTranslatef(elementCenter.x, elementCenter.y, elementCenter.z);
            glutWireCube(elementSize);
        glPopMatrix();

        if (displayElementChildProxies) {
            // draw the children
            float halfSize = elementSize / 2.0f;
            float quarterSize = elementSize / 4.0f;
            glColor3f(1.0f, 1.0f, 0.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x - quarterSize, elementCenter.y - quarterSize, elementCenter.z - quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(1.0f, 0.0f, 1.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x + quarterSize, elementCenter.y - quarterSize, elementCenter.z - quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.0f, 1.0f, 0.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x - quarterSize, elementCenter.y + quarterSize, elementCenter.z - quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.0f, 0.0f, 1.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x - quarterSize, elementCenter.y - quarterSize, elementCenter.z + quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(1.0f, 1.0f, 1.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x + quarterSize, elementCenter.y + quarterSize, elementCenter.z + quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.0f, 0.5f, 0.5f);
            glPushMatrix();
                glTranslatef(elementCenter.x - quarterSize, elementCenter.y + quarterSize, elementCenter.z + quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.5f, 0.0f, 0.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x + quarterSize, elementCenter.y - quarterSize, elementCenter.z + quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();

            glColor3f(0.0f, 0.5f, 0.0f);
            glPushMatrix();
                glTranslatef(elementCenter.x + quarterSize, elementCenter.y + quarterSize, elementCenter.z - quarterSize);
                glutWireCube(halfSize);
            glPopMatrix();
        }

    }

    for (uint16_t i = 0; i < numberOfEntities; i++) {
        EntityItem* entityItem = entityItems[i];

        bool wantDebug = false;
        
        if (wantDebug) {
            bool isBestFit = entityTreeElement->bestFitEntityBounds(entityItem);
            qDebug() << "EntityTreeRenderer::renderElement() "
                     << "entityTreeElement=" << entityTreeElement 
                     << "entityItems[" << i << "]=" << entityItem
                     << "ID=" << entityItem->getEntityItemID()
                     << "isBestFit=" << isBestFit;
        }
        
        // render entityItem aspoints
        AACube entityCube = entityItem->getAACube();
        entityCube.scale(TREE_SCALE);
        if (args->_viewFrustum->cubeInFrustum(entityCube) != ViewFrustum::OUTSIDE) {
            glm::vec3 position = entityItem->getPosition() * (float)TREE_SCALE;
            float radius = entityItem->getRadius() * (float)TREE_SCALE;
            float size = entityItem->getSize() * (float)TREE_SCALE;

#if 0 //def HIDE_SUBCLASS_METHODS
            bool drawAsModel = entityItem->hasModel();
#else
            bool drawAsModel = false;
#endif

            args->_itemsRendered++;

            if (drawAsModel) {
#if 0 //def HIDE_SUBCLASS_METHODS
                glPushMatrix();
                {
                    const float alpha = 1.0f;
                
                    Model* model = getModel(*entityItem);
                    
                    if (model) {
                        model->setScaleToFit(true, radius * 2.0f);
                        model->setSnapModelToCenter(true);
                
                        // set the rotation
                        glm::quat rotation = entityItem->getRotation();
                        model->setRotation(rotation);
                
                        // set the position
                        model->setTranslation(position);
                    
                        // handle animations..
                        if (entityItem->hasAnimation()) {
                            if (!entityItem->jointsMapped()) {
                                QStringList modelJointNames = model->getJointNames();
                                entityItem->mapJoints(modelJointNames);
                            }

                            QVector<glm::quat> frameData = entityItem->getAnimationFrame();
                            for (int i = 0; i < frameData.size(); i++) {
                                model->setJointState(i, true, frameData[i]);
                            }
                        }
                    
                        // make sure to simulate so everything gets set up correctly for rendering
                        model->simulate(0.0f);

                        // TODO: should we allow entityItems to have alpha on their models?
                        Model::RenderMode modelRenderMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE 
                                                                ? Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
                
                        if (entityItem->getGlowLevel() > 0.0f) {
                            Glower glower(entityItem->getGlowLevel());
                            
                            if (model->isActive()) {
                                model->render(alpha, modelRenderMode);
                            } else {
                                // if we couldn't get a model, then just draw a sphere
                                glColor3ub(entityItem->getColor()[RED_INDEX],entityItem->getColor()[GREEN_INDEX],entityItem->getColor()[BLUE_INDEX]);
                                glPushMatrix();
                                    glTranslatef(position.x, position.y, position.z);
                                    glutSolidSphere(radius, 15, 15);
                                glPopMatrix();
                            }
                        } else {
                            if (model->isActive()) {
                                model->render(alpha, modelRenderMode);
                            } else {
                                // if we couldn't get a model, then just draw a sphere
                                glColor3ub(entityItem->getColor()[RED_INDEX],entityItem->getColor()[GREEN_INDEX],entityItem->getColor()[BLUE_INDEX]);
                                glPushMatrix();
                                    glTranslatef(position.x, position.y, position.z);
                                    glutSolidSphere(radius, 15, 15);
                                glPopMatrix();
                            }
                        }

                        if (!isShadowMode && displayModelBounds) {

                            glm::vec3 unRotatedMinimum = model->getUnscaledMeshExtents().minimum;
                            glm::vec3 unRotatedMaximum = model->getUnscaledMeshExtents().maximum;
                            glm::vec3 unRotatedExtents = unRotatedMaximum - unRotatedMinimum;
         
                            float width = unRotatedExtents.x;
                            float height = unRotatedExtents.y;
                            float depth = unRotatedExtents.z;

                            Extents rotatedExtents = model->getUnscaledMeshExtents();
                            calculateRotatedExtents(rotatedExtents, rotation);

                            glm::vec3 rotatedSize = rotatedExtents.maximum - rotatedExtents.minimum;

                            const glm::vec3& modelScale = model->getScale();

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
                        glColor3ub(entityItem->getColor()[RED_INDEX],entityItem->getColor()[GREEN_INDEX],entityItem->getColor()[BLUE_INDEX]);
                        glPushMatrix();
                            glTranslatef(position.x, position.y, position.z);
                            glutSolidSphere(radius, 15, 15);
                        glPopMatrix();
                    }
                }
                glPopMatrix();
#endif
            } else {
                //glColor3ub(entityItem->getColor()[RED_INDEX],entityItem->getColor()[GREEN_INDEX],entityItem.getColor()[BLUE_INDEX]);
                
                EntityTypes::EntityType_t type = entityItem->getType();
                
                //qDebug() << "rendering type=" << type;
                
                if (type == EntityTypes::Model) {
                    glColor3f(1.0f, 0.0f, 0.0f);
                } else {
                    glColor3f(0.0f, 1.0f, 0.0f);
                }
                glPushMatrix();
                    glTranslatef(position.x, position.y, position.z);
                    glutSolidSphere(radius, 15, 15);
                glPopMatrix();
            }
        } else {
            args->_itemsOutOfView++;
        }
    }
}

float EntityTreeRenderer::getSizeScale() const { 
    return Menu::getInstance()->getVoxelSizeScale();
}

int EntityTreeRenderer::getBoundaryLevelAdjust() const { 
    return Menu::getInstance()->getBoundaryLevelAdjust();
}


void EntityTreeRenderer::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    static_cast<EntityTree*>(_tree)->processEraseMessage(dataByteArray, sourceNode);
}
