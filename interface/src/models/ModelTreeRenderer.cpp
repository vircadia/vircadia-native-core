//
//  ModelTreeRenderer.cpp
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
#include "ModelTreeRenderer.h"

ModelTreeRenderer::ModelTreeRenderer() :
    OctreeRenderer() {
}

ModelTreeRenderer::~ModelTreeRenderer() {
    // delete the models in _knownModelsItemModels
    foreach(Model* model, _knownModelsItemModels) {
        delete model;
    }
    _knownModelsItemModels.clear();

    foreach(Model* model, _unknownModelsItemModels) {
        delete model;
    }
    _unknownModelsItemModels.clear();
}

void ModelTreeRenderer::init() {
    OctreeRenderer::init();
    static_cast<ModelTree*>(_tree)->setFBXService(this);
}

void ModelTreeRenderer::setTree(Octree* newTree) {
    OctreeRenderer::setTree(newTree);
    static_cast<ModelTree*>(_tree)->setFBXService(this);
}

void ModelTreeRenderer::update() {
    if (_tree) {
        ModelTree* tree = static_cast<ModelTree*>(_tree);
        tree->update();
    }
}

void ModelTreeRenderer::render(RenderMode renderMode) {
    OctreeRenderer::render(renderMode);
}

const FBXGeometry* ModelTreeRenderer::getGeometryForModel(const ModelItem& modelItem) {
    const FBXGeometry* result = NULL;
    Model* model = getModel(modelItem);
    if (model) {
        result = &model->getGeometry()->getFBXGeometry();
        
    }
    return result;
}

Model* ModelTreeRenderer::getModel(const ModelItem& modelItem) {
    Model* model = NULL;
    
    if (modelItem.isKnownID()) {
        if (_knownModelsItemModels.find(modelItem.getID()) != _knownModelsItemModels.end()) {
            model = _knownModelsItemModels[modelItem.getID()];
        } else {
            model = new Model();
            model->init();
            model->setURL(QUrl(modelItem.getModelURL()));
            _knownModelsItemModels[modelItem.getID()] = model;
        }
    } else {
        if (_unknownModelsItemModels.find(modelItem.getCreatorTokenID()) != _unknownModelsItemModels.end()) {
            model = _unknownModelsItemModels[modelItem.getCreatorTokenID()];
        } else {
            model = new Model();
            model->init();
            model->setURL(QUrl(modelItem.getModelURL()));
            _unknownModelsItemModels[modelItem.getCreatorTokenID()] = model;
        }
    }
    return model;
}

void ModelTreeRenderer::renderElement(OctreeElement* element, RenderArgs* args) {
    args->_elementsTouched++;
    // actually render it here...
    // we need to iterate the actual modelItems of the element
    ModelTreeElement* modelTreeElement = (ModelTreeElement*)element;

    QList<ModelItem>& modelItems = modelTreeElement->getModels();

    uint16_t numberOfModels = modelItems.size();
    
    bool isShadowMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE;

    bool displayModelBounds = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelBounds);
    bool displayElementProxy = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementProxy);
    bool displayElementChildProxies = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementChildProxies);


    if (!isShadowMode && displayElementProxy && numberOfModels > 0) {
        glm::vec3 elementCenter = modelTreeElement->getAACube().calcCenter() * (float)TREE_SCALE;
        float elementSize = modelTreeElement->getScale() * (float)TREE_SCALE;
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

    for (uint16_t i = 0; i < numberOfModels; i++) {
        ModelItem& modelItem = modelItems[i];
        // render modelItem aspoints
        AACube modelCube = modelItem.getAACube();
        modelCube.scale(TREE_SCALE);
        if (args->_viewFrustum->cubeInFrustum(modelCube) != ViewFrustum::OUTSIDE) {
            glm::vec3 position = modelItem.getPosition() * (float)TREE_SCALE;
            float radius = modelItem.getRadius() * (float)TREE_SCALE;
            float size = modelItem.getSize() * (float)TREE_SCALE;

            bool drawAsModel = modelItem.hasModel();

            args->_itemsRendered++;

            if (drawAsModel) {
                glPushMatrix();
                    const float alpha = 1.0f;
                
                    Model* model = getModel(modelItem);

                    model->setScaleToFit(true, radius * 2.0f);
                    model->setSnapModelToCenter(true);
                
                    // set the rotation
                    glm::quat rotation = modelItem.getModelRotation();
                    model->setRotation(rotation);
                
                    // set the position
                    model->setTranslation(position);
                    
                    // handle animations..
                    if (modelItem.hasAnimation()) {
                        if (!modelItem.jointsMapped()) {
                            QStringList modelJointNames = model->getJointNames();
                            modelItem.mapJoints(modelJointNames);
                        }

                        QVector<glm::quat> frameData = modelItem.getAnimationFrame();
                        for (int i = 0; i < frameData.size(); i++) {
                            model->setJointState(i, true, frameData[i]);
                        }
                    }
                    
                    // make sure to simulate so everything gets set up correctly for rendering
                    model->simulate(0.0f);

                    // TODO: should we allow modelItems to have alpha on their models?
                    Model::RenderMode modelRenderMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE 
                                                            ? Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
                
                    if (modelItem.getGlowLevel() > 0.0f) {
                        Glower glower(modelItem.getGlowLevel());
                        model->render(alpha, modelRenderMode);
                    } else {
                        model->render(alpha, modelRenderMode);
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

                glPopMatrix();
            } else {
                glColor3ub(modelItem.getColor()[RED_INDEX],modelItem.getColor()[GREEN_INDEX],modelItem.getColor()[BLUE_INDEX]);
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

float ModelTreeRenderer::getSizeScale() const { 
    return Menu::getInstance()->getVoxelSizeScale();
}

int ModelTreeRenderer::getBoundaryLevelAdjust() const { 
    return Menu::getInstance()->getBoundaryLevelAdjust();
}


void ModelTreeRenderer::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    static_cast<ModelTree*>(_tree)->processEraseMessage(dataByteArray, sourceNode);
}
