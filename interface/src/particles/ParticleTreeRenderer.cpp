//
//  ParticleTreeRenderer.cpp
//  interface/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include "Application.h"
#include "InterfaceConfig.h"

#include "ParticleTreeRenderer.h"

ParticleTreeRenderer::ParticleTreeRenderer() :
    OctreeRenderer() {
}

ParticleTreeRenderer::~ParticleTreeRenderer() {
    // delete the models in _particleModels
    foreach(Model* model, _particleModels) {
        delete model;
    }
    _particleModels.clear();
}

void ParticleTreeRenderer::init() {
    OctreeRenderer::init();
}


void ParticleTreeRenderer::update() {
    if (_tree) {
        ParticleTree* tree = static_cast<ParticleTree*>(_tree);
        tree->update();
    }
}

void ParticleTreeRenderer::render(RenderMode renderMode) {
    OctreeRenderer::render(renderMode);
}

Model* ParticleTreeRenderer::getModel(const QString& url) {
    Model* model = NULL;
    
    // if we don't already have this model then create it and initialize it
    if (_particleModels.find(url) == _particleModels.end()) {
        model = new Model();
        model->init();
        model->setURL(QUrl(url));
        _particleModels[url] = model;
    } else {
        model = _particleModels[url];
    }
    return model;
}

void ParticleTreeRenderer::renderElement(OctreeElement* element, RenderArgs* args) {
    // actually render it here...
    // we need to iterate the actual particles of the element
    ParticleTreeElement* particleTreeElement = (ParticleTreeElement*)element;

    const QList<Particle>& particles = particleTreeElement->getParticles();

    uint16_t numberOfParticles = particles.size();

    for (uint16_t i = 0; i < numberOfParticles; i++) {
        const Particle& particle = particles[i];
        // render particle aspoints
        glm::vec3 position = particle.getPosition() * (float)TREE_SCALE;
        glColor3ub(particle.getColor()[RED_INDEX],particle.getColor()[GREEN_INDEX],particle.getColor()[BLUE_INDEX]);
        float radius = particle.getRadius() * (float)TREE_SCALE;

        bool drawAsModel = particle.hasModel();

        args->_itemsRendered++;

        if (drawAsModel) {
            glPushMatrix();
                const float alpha = 1.0f;
                
                Model* model = getModel(particle.getModelURL());
                glm::vec3 translationAdjustment = particle.getModelTranslation() * radius;
                    
                // set the position
                glm::vec3 translation = position + translationAdjustment;
                model->setTranslation(translation);

                // set the rotation
                glm::quat rotation = particle.getModelRotation();
                model->setRotation(rotation);

                // scale
                // TODO: need to figure out correct scale adjust, this was arbitrarily set to make a couple models work
                const float MODEL_SCALE = 0.00575f;
                glm::vec3 scale(1.0f,1.0f,1.0f);
                
                float modelScale = particle.getModelScale();
                model->setScale(scale * MODEL_SCALE * radius * modelScale);

                model->simulate(0.0f);

                // TODO: should we allow particles to have alpha on their models?
                Model::RenderMode modelRenderMode = args->_renderMode == OctreeRenderer::SHADOW_RENDER_MODE 
                                                        ? Model::SHADOW_RENDER_MODE : Model::DEFAULT_RENDER_MODE;
                model->render(alpha, modelRenderMode);

                const bool wantDebugSphere = false;
                if (wantDebugSphere) {
                    glPushMatrix();
                        glTranslatef(position.x, position.y, position.z);
                        Application::getInstance()->getDeferredLightingEffect()->renderWireSphere(radius, 15, 15);
                    glPopMatrix();
                }

            glPopMatrix();
        } else {
            glPushMatrix();
                glTranslatef(position.x, position.y, position.z);
                Application::getInstance()->getDeferredLightingEffect()->renderSolidSphere(radius, 15, 15);
            glPopMatrix();
        }
    }
}

void ParticleTreeRenderer::processEraseMessage(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode) {
    if (_tree){
        _tree->lockForWrite();
        static_cast<ParticleTree*>(_tree)->processEraseMessage(dataByteArray, sourceNode);
        _tree->unlock();
    }
}
