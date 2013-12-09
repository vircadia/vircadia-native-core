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

void ParticleTreeRenderer::renderElement(OctreeElement* element) {
    // actually render it here...
    // we need to iterate the actual particles of the element
    ParticleTreeElement* particleTreeElement = (ParticleTreeElement*)element;
    
    const std::vector<Particle>& particles = particleTreeElement->getParticles();
    
    uint16_t numberOfParticles = particles.size();
    
    glPointSize(20.0f);
    glBegin(GL_POINTS);
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        const Particle& particle = particles[i];
        // render particle aspoints
        glm::vec3 position = particle.getPosition() * (float)TREE_SCALE;
        
        printf("glVertex3f(%f, %f, %f)\n", position.x, position.y, position.z);
        
        glVertex3f(position.x, position.y, position.z);
    }
    glEnd();
}
