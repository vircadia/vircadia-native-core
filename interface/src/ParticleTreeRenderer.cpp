//
//  ParticleTreeRenderer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include "InterfaceConfig.h"

#include "ParticleTreeRenderer.h"

ParticleTreeRenderer::ParticleTreeRenderer() : 
    OctreeRenderer() {
}

ParticleTreeRenderer::~ParticleTreeRenderer() {
}

void ParticleTreeRenderer::update() {
    if (_tree) {
        ParticleTree* tree = (ParticleTree*)_tree;
        _tree->lockForWrite();
        tree->update();
        _tree->unlock();
    }
}

void ParticleTreeRenderer::renderElement(OctreeElement* element, RenderArgs* args) {
    // actually render it here...
    // we need to iterate the actual particles of the element
    ParticleTreeElement* particleTreeElement = (ParticleTreeElement*)element;
    
    const std::vector<Particle>& particles = particleTreeElement->getParticles();
    
    uint16_t numberOfParticles = particles.size();
    
    bool drawAsSphere = true;
    
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        const Particle& particle = particles[i];
        // render particle aspoints
        glm::vec3 position = particle.getPosition() * (float)TREE_SCALE;
        glColor3ub(particle.getColor()[RED_INDEX],particle.getColor()[GREEN_INDEX],particle.getColor()[BLUE_INDEX]);
        float sphereRadius = particle.getRadius() * (float)TREE_SCALE;

        args->_renderedItems++;
        
        if (drawAsSphere) {
            glPushMatrix();
                glTranslatef(position.x, position.y, position.z);
                glutSolidSphere(sphereRadius, 15, 15);
            glPopMatrix();
        } else {
            glPointSize(sphereRadius);
            glBegin(GL_POINTS);
            glVertex3f(position.x, position.y, position.z);
            glEnd();
        }
    }
}
