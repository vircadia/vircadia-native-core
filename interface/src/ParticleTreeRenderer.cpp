//
//  ParticleTreeRenderer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include <glm/gtx/quaternion.hpp>

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
        ParticleTree* tree = (ParticleTree*)_tree;
        _tree->lockForWrite();
        tree->update();
        _tree->unlock();
    }
}

void ParticleTreeRenderer::render() {
    OctreeRenderer::render();
}

//_testModel->setURL(QUrl("http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/lotus.fbx"));
//_testModel->setURL(QUrl("http://www.fungibleinsight.com/faces/tie.fbx"));
//_testModel->setURL(QUrl("http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Angie1.fbx"));
//_testModel->setURL(QUrl("http://public.highfidelity.io/meshes/orb_model.fbx"));
//_testModel->setURL(QUrl("http://public.highfidelity.io/meshes/space_frigate_6.FBX"));
//_testModel->setURL(QUrl("http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/soccer_ball.fbx"));
//_testModel->setURL(QUrl("http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/top%20fbx.FBX"));
//_testModel->setURL(QUrl("http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/golfball_FBX2010.fbx"));
//_testModel->setURL(QUrl("http://public.highfidelity.io/meshes/Combat_tank_V01.FBX"));

//_testModel->setURL(QUrl("http://public.highfidelity.io/meshes/orc.fbx"));
//_testModel->setURL(QUrl("http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Feisar_Ship.FBX"));

Model* ParticleTreeRenderer::getModel(const QString& url) {
    Model* model = NULL;
    
    // if we don't already have this model then create it and initialize it
    if (_particleModels.find(url) == _particleModels.end()) {
        model = new Model();
        model->init();
        qDebug() << "calling model->setURL()";
        model->setURL(QUrl(url));
        qDebug() << "after calling setURL()";
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
        float sphereRadius = particle.getRadius() * (float)TREE_SCALE;

        bool drawAsModel = particle.hasModel();

        args->_renderedItems++;

        if (drawAsModel) {
            glPushMatrix();
                const float alpha = 1.0f;
                
                Model* model = getModel(particle.getModelURL());
                
                glm::vec3 translationAdjustment = particle.getModelTranslation();

                // set the position
                glm::vec3 translation(position.x, position.y, position.z);
                model->setTranslation(translation + translationAdjustment);

                // glm::angleAxis(-90.0f, 1.0f, 0.0f, 0.0f)
                // set the rotation
                glm::quat rotation = particle.getModelRotation();
                model->setRotation(rotation);

                // scale
                const float MODEL_SCALE = 0.0006f; // need to figure out correct scale adjust
                glm::vec3 scale(1.0f,1.0f,1.0f);
                model->setScale(scale * MODEL_SCALE);

                model->simulate(0.0f);
                model->render(alpha);
                //qDebug() << "called _testModel->render(alpha);";
            glPopMatrix();
        } else {
            glPushMatrix();
                glTranslatef(position.x, position.y, position.z);
                glutSolidSphere(sphereRadius, 15, 15);
            glPopMatrix();
        }
    }
}

void ParticleTreeRenderer::processEraseMessage(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr,
        Node* sourceNode) {
    static_cast<ParticleTree*>(_tree)->processEraseMessage(dataByteArray, senderSockAddr, sourceNode);
}
