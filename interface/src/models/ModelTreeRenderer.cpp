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

void ModelTreeRenderer::render() {
    OctreeRenderer::render();
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

    uint16_t numberOfParticles = modelItems.size();

    for (uint16_t i = 0; i < numberOfParticles; i++) {
        const ModelItem& modelItem = modelItems[i];
        // render modelItem aspoints
        glm::vec3 position = modelItem.getPosition() * (float)TREE_SCALE;
        glColor3ub(modelItem.getColor()[RED_INDEX],modelItem.getColor()[GREEN_INDEX],modelItem.getColor()[BLUE_INDEX]);
        float radius = modelItem.getRadius() * (float)TREE_SCALE;
        //glm::vec3 center = position + glm::vec3(radius, radius, radius); // center it around the position

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


                model->render(alpha); // TODO: should we allow modelItems to have alpha on their models?

                const bool wantDebugSphere = false;
                if (wantDebugSphere) {
                    glPushMatrix();
                        glTranslatef(position.x, position.y, position.z);
                        glutWireSphere(radius, 15, 15);
                    glPopMatrix();
                }

            glPopMatrix();
        } else {
            glPushMatrix();
                glTranslatef(position.x, position.y, position.z);
                glutSolidSphere(radius, 15, 15);
            glPopMatrix();
        }
    }
}

void ModelTreeRenderer::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    static_cast<ModelTree*>(_tree)->processEraseMessage(dataByteArray, sourceNode);
}
