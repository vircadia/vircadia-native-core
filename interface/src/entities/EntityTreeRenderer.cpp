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

#include <ModelEntityItem.h>
#include <BoxEntityItem.h>


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
    //REGISTER_ENTITY_TYPE_RENDERER(Model, this->renderEntityTypeModel);
    //REGISTER_ENTITY_TYPE_RENDERER(Box, this->renderEntityTypeBox);

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

const FBXGeometry* EntityTreeRenderer::getGeometryForEntity(const EntityItem* entityItem) {
    const FBXGeometry* result = NULL;
    
    if (entityItem->getType() == EntityTypes::Model) {
        const ModelEntityItem* modelEntityItem = static_cast<const ModelEntityItem*>(entityItem);
    
        Model* model = getModel(modelEntityItem);
        if (model) {
            result = &model->getGeometry()->getFBXGeometry();
        }
    }
    return result;
}

const Model* EntityTreeRenderer::getModelForEntityItem(const EntityItem* entityItem) {
    const Model* result = NULL;
    if (entityItem->getType() == EntityTypes::Model) {
        const ModelEntityItem* modelEntityItem = static_cast<const ModelEntityItem*>(entityItem);
    
        result = getModel(modelEntityItem);
    }
    return result;
}

Model* EntityTreeRenderer::getModel(const ModelEntityItem* modelEntityItem) {
    Model* model = NULL;
 
    if (!modelEntityItem->getModelURL().isEmpty()) {
        if (modelEntityItem->isKnownID()) {
            if (_knownEntityItemModels.find(modelEntityItem->getID()) != _knownEntityItemModels.end()) {
                model = _knownEntityItemModels[modelEntityItem->getID()];
                if (QUrl(modelEntityItem->getModelURL()) != model->getURL()) {
                    delete model; // delete the old model...
                    model = NULL;
                    _knownEntityItemModels.remove(modelEntityItem->getID());
                }
            }

            // if we don't have a model... but our item does have a model URL
            if (!model) {
                // Make sure we only create new models on the thread that owns the EntityTreeRenderer
                if (QThread::currentThread() != thread()) {
                    qDebug() << "about to call QMetaObject::invokeMethod(this, 'getModel', Qt::BlockingQueuedConnection,...";
                    QMetaObject::invokeMethod(this, "getModel", Qt::BlockingQueuedConnection,
                        Q_RETURN_ARG(Model*, model), Q_ARG(const ModelEntityItem*, modelEntityItem));
                    qDebug() << "got it... model=" << model;
                    return model;
                }

                model = new Model();
                model->init();
                model->setURL(QUrl(modelEntityItem->getModelURL()));
                _knownEntityItemModels[modelEntityItem->getID()] = model;
            }
            
        } else {
            if (_unknownEntityItemModels.find(modelEntityItem->getCreatorTokenID()) != _unknownEntityItemModels.end()) {
                model = _unknownEntityItemModels[modelEntityItem->getCreatorTokenID()];
                if (QUrl(modelEntityItem->getModelURL()) != model->getURL()) {
                    delete model; // delete the old model...
                    model = NULL;
                    _unknownEntityItemModels.remove(modelEntityItem->getCreatorTokenID());
                }
            }

            if (!model) {
                // Make sure we only create new models on the thread that owns the EntityTreeRenderer
                if (QThread::currentThread() != thread()) {
                    QMetaObject::invokeMethod(this, "getModel", Qt::BlockingQueuedConnection,
                        Q_RETURN_ARG(Model*, model), Q_ARG(const ModelEntityItem*, modelEntityItem));
                    return model;
                }
        
                model = new Model();
                model->init();
                model->setURL(QUrl(modelEntityItem->getModelURL()));
                _unknownEntityItemModels[modelEntityItem->getCreatorTokenID()] = model;
            }
        }
    }
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
        
        // TODO: some entity types (like lights) might want to be rendered even
        // when they are outside of the view frustum...
        if (args->_viewFrustum->cubeInFrustum(entityCube) != ViewFrustum::OUTSIDE) {

            Glower* glower = NULL;
            if (entityItem->getGlowLevel() > 0.0f) {
                glower = new Glower(entityItem->getGlowLevel());
            }
                    
            if (entityItem->getType() == EntityTypes::Model) {
                renderEntityTypeModel(entityItem, args);
            } else if (entityItem->getType() == EntityTypes::Box) {
                renderEntityTypeBox(entityItem, args);
            }
            
            if (glower) {
                delete glower;
            }

        } else {
            args->_itemsOutOfView++;
        }
    }
}

void EntityTreeRenderer::renderEntityTypeModel(EntityItem* entity, RenderArgs* args) {
    assert(entity->getType() == EntityTypes::Model);
    
    ModelEntityItem* entityItem = static_cast<ModelEntityItem*>(entity);
    bool drawAsModel = entityItem->hasModel();

    glm::vec3 position = entityItem->getPosition() * (float)TREE_SCALE;
    float radius = entityItem->getRadius() * (float)TREE_SCALE;
    float size = entity->getSize() * (float)TREE_SCALE;
    
    if (drawAsModel) {
        glPushMatrix();
        {
            const float alpha = 1.0f;
        
            Model* model = getModel(entityItem);
            
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
                    //qDebug() << "entityItem->hasAnimation()...";                
                    if (!entityItem->jointsMapped()) {
                        QStringList modelJointNames = model->getJointNames();
                        entityItem->mapJoints(modelJointNames);
                        //qDebug() << "entityItem->mapJoints()...";                
                    }

                    if (entityItem->jointsMapped()) {
                        //qDebug() << "model->setJointState()...";
                        QVector<glm::quat> frameData = entityItem->getAnimationFrame();
                        for (int i = 0; i < frameData.size(); i++) {
                            model->setJointState(i, true, frameData[i]);
                        }
                    }
                }
            
                // make sure to simulate so everything gets set up correctly for rendering
                model->simulate(0.0f);

                // TODO: should we allow entityItems to have alpha on their models?
                Model::RenderMode modelRenderMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE 
                                                        ? Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
        
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
                
                bool isShadowMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE;
                bool displayModelBounds = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelBounds);

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
    } else {
        glColor3f(1.0f, 0.0f, 0.0f);
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glutSolidSphere(radius, 15, 15);
        glPopMatrix();
    }
}


// vertex array for glDrawElements() and glDrawRangeElement() =================
// Notice that the sizes of these arrays become samller than the arrays for
// glDrawArrays() because glDrawElements() uses an additional index array to
// choose designated vertices with the indices. The size of vertex array is now
// 24 instead of 36, but the index array size is 36, same as the number of
// vertices required to draw a cube.
GLfloat vertices2[] = { 1, 1, 1,  -1, 1, 1,  -1,-1, 1,   1,-1, 1,   // v0,v1,v2,v3 (front)
                        1, 1, 1,   1,-1, 1,   1,-1,-1,   1, 1,-1,   // v0,v3,v4,v5 (right)
                        1, 1, 1,   1, 1,-1,  -1, 1,-1,  -1, 1, 1,   // v0,v5,v6,v1 (top)
                       -1, 1, 1,  -1, 1,-1,  -1,-1,-1,  -1,-1, 1,   // v1,v6,v7,v2 (left)
                       -1,-1,-1,   1,-1,-1,   1,-1, 1,  -1,-1, 1,   // v7,v4,v3,v2 (bottom)
                        1,-1,-1,  -1,-1,-1,  -1, 1,-1,   1, 1,-1 }; // v4,v7,v6,v5 (back)

// normal array
GLfloat normals2[]  = { 0, 0, 1,   0, 0, 1,   0, 0, 1,   0, 0, 1,   // v0,v1,v2,v3 (front)
                        1, 0, 0,   1, 0, 0,   1, 0, 0,   1, 0, 0,   // v0,v3,v4,v5 (right)
                        0, 1, 0,   0, 1, 0,   0, 1, 0,   0, 1, 0,   // v0,v5,v6,v1 (top)
                       -1, 0, 0,  -1, 0, 0,  -1, 0, 0,  -1, 0, 0,   // v1,v6,v7,v2 (left)
                        0,-1, 0,   0,-1, 0,   0,-1, 0,   0,-1, 0,   // v7,v4,v3,v2 (bottom)
                        0, 0,-1,   0, 0,-1,   0, 0,-1,   0, 0,-1 }; // v4,v7,v6,v5 (back)

// color array
GLfloat colors2[]   = { 1, 1, 1,   1, 1, 0,   1, 0, 0,   1, 0, 1,   // v0,v1,v2,v3 (front)
                        1, 1, 1,   1, 0, 1,   0, 0, 1,   0, 1, 1,   // v0,v3,v4,v5 (right)
                        1, 1, 1,   0, 1, 1,   0, 1, 0,   1, 1, 0,   // v0,v5,v6,v1 (top)
                        1, 1, 0,   0, 1, 0,   0, 0, 0,   1, 0, 0,   // v1,v6,v7,v2 (left)
                        0, 0, 0,   0, 0, 1,   1, 0, 1,   1, 0, 0,   // v7,v4,v3,v2 (bottom)
                        0, 0, 1,   0, 0, 0,   0, 1, 0,   0, 1, 1 }; // v4,v7,v6,v5 (back)

// index array of vertex array for glDrawElements() & glDrawRangeElement()
GLubyte indices[]  = { 0, 1, 2,   2, 3, 0,      // front
                       4, 5, 6,   6, 7, 4,      // right
                       8, 9,10,  10,11, 8,      // top
                      12,13,14,  14,15,12,      // left
                      16,17,18,  18,19,16,      // bottom
                      20,21,22,  22,23,20 };    // back

void EntityTreeRenderer::renderEntityTypeBox(EntityItem* entity, RenderArgs* args) {
    assert(entity->getType() == EntityTypes::Box);
    glm::vec3 position = entity->getPosition() * (float)TREE_SCALE;
    float size = entity->getSize() * (float)TREE_SCALE;
    glm::quat rotation = entity->getRotation();
    BoxEntityItem* boxEntityItem = static_cast<BoxEntityItem*>(entity);


    /*
    glColor3f(0.0f, 1.0f, 0.0f);
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glutSolidCube(size);
    
    glPopMatrix();
    */
    
    // enable and specify pointers to vertex arrays
    glEnableClientState(GL_NORMAL_ARRAY);
    //glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glNormalPointer(GL_FLOAT, 0, normals2);
    //glColorPointer(3, GL_FLOAT, 0, colors2);
    glVertexPointer(3, GL_FLOAT, 0, vertices2);

    //glEnable(GL_BLEND);

    glColor3ub(boxEntityItem->getColor()[RED_INDEX], boxEntityItem->getColor()[GREEN_INDEX], boxEntityItem->getColor()[BLUE_INDEX]);
        
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glScalef(size, size, size);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, indices);
    glPopMatrix();

    glDisableClientState(GL_VERTEX_ARRAY);  // disable vertex arrays
    //glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
};


float EntityTreeRenderer::getSizeScale() const { 
    return Menu::getInstance()->getVoxelSizeScale();
}

int EntityTreeRenderer::getBoundaryLevelAdjust() const { 
    return Menu::getInstance()->getBoundaryLevelAdjust();
}


void EntityTreeRenderer::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    static_cast<EntityTree*>(_tree)->processEraseMessage(dataByteArray, sourceNode);
}





