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

#include "InterfaceConfig.h"
#include "Menu.h"
#include "ModelTreeRenderer.h"

ModelTreeRenderer::ModelTreeRenderer() :
    OctreeRenderer() {
}

ModelTreeRenderer::~ModelTreeRenderer() {
    // delete the models in _modelsItemModels
    foreach(Model* model, _modelsItemModels) {
        delete model;
    }
    _modelsItemModels.clear();
}

void ModelTreeRenderer::init() {
    OctreeRenderer::init();
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

Model* ModelTreeRenderer::getModel(const QString& url) {
    Model* model = NULL;
    
    // if we don't already have this model then create it and initialize it
    if (_modelsItemModels.find(url) == _modelsItemModels.end()) {
        model = new Model();
        model->init();
        model->setURL(QUrl(url));
        _modelsItemModels[url] = model;
    } else {
        model = _modelsItemModels[url];
    }
    return model;
}

void ModelTreeRenderer::renderElement(OctreeElement* element, RenderArgs* args) {
    // actually render it here...
    // we need to iterate the actual modelItems of the element
    ModelTreeElement* modelTreeElement = (ModelTreeElement*)element;

    const QList<ModelItem>& modelItems = modelTreeElement->getModels();

    uint16_t numberOfModels = modelItems.size();
    
    bool isShadowMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE;

    bool displayModelBounds = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelBounds);
    bool displayElementProxy = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementProxy);
    bool displayElementChildProxies = Menu::getInstance()->isOptionChecked(MenuOption::DisplayModelElementChildProxies);


    if (!isShadowMode && displayElementProxy && numberOfModels > 0) {
        glm::vec3 elementCenter = modelTreeElement->getAABox().calcCenter() * (float)TREE_SCALE;
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
        const ModelItem& modelItem = modelItems[i];
        // render modelItem aspoints
        AABox modelBox = modelItem.getAABox();
        modelBox.scale(TREE_SCALE);
        if (args->_viewFrustum->boxInFrustum(modelBox) != ViewFrustum::OUTSIDE) {
            glm::vec3 position = modelItem.getPosition() * (float)TREE_SCALE;
            float radius = modelItem.getRadius() * (float)TREE_SCALE;
            float size = modelItem.getSize() * (float)TREE_SCALE;

            bool drawAsModel = modelItem.hasModel();

            args->_renderedItems++;

            if (drawAsModel) {
                glPushMatrix();
                    const float alpha = 1.0f;
                
                    Model* model = getModel(modelItem.getModelURL());

                    model->setScaleToFit(true, radius * 2.0f);
                    model->setSnapModelToCenter(true);
                
                    // set the rotation
                    glm::quat rotation = modelItem.getModelRotation();
                    model->setRotation(rotation);
                
                    // set the position
                    model->setTranslation(position);
                    model->simulate(0.0f);

                    // TODO: should we allow modelItems to have alpha on their models?
                    Model::RenderMode modelRenderMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE 
                                                            ? Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
                    model->render(alpha, modelRenderMode);

                    if (!isShadowMode && displayModelBounds) {
                        glColor3f(0.0f, 1.0f, 0.0f);
                        glPushMatrix();
                            glTranslatef(position.x, position.y, position.z);
                            glutWireCube(size);
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
