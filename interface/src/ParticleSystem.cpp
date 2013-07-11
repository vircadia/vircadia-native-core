//
//  ParticleSystem.cpp
//  hifi
//
//  Created by Jeffrey on July 10, 2013
//

#include <glm/glm.hpp>
#include "InterfaceConfig.h"
#include <SharedUtil.h>

#include "ParticleSystem.h"

ParticleSystem::ParticleSystem() {

    _numberOfParticles = 1500;
    assert(_numberOfParticles <= MAX_PARTICLES);

    _bounce             = 0.9f;
    _timer              = 0.0f;
    _airFriction        = 6.0f;
    _jitter             = 0.1f;
    _homeAttraction     = 0.0f;
    _tornadoForce       = 0.0f;
    _neighborAttraction = 0.02f;
    _neighborRepulsion  = 0.9f;
    _tornadoAxis        = glm::normalize(glm::vec3(0.1f, 1.0f, 0.1f));
    _home               = glm::vec3(5.0f, 0.5f, 5.0f);
    
    _TEST_bigSphereRadius = 0.5f;
    _TEST_bigSpherePosition = glm::vec3( 5.0f, _TEST_bigSphereRadius, 5.0f);

    for (unsigned int p = 0; p < _numberOfParticles; p++) {
        _particle[p].position = _home;
        _particle[p].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        
        float red   = 0.5f + 0.5f * sinf(((float)p / (float)_numberOfParticles) * 3.141592 * 2.0f);
        float green = 0.3f + 0.3f * sinf(((float)p / (float)_numberOfParticles) * 3.141592 * 2.0f);
        float blue  = 0.2f - 0.2f * sinf(((float)p / (float)_numberOfParticles) * 3.141592 * 2.0f);
        
        _particle[p].color    = glm::vec3(red, green, blue);
        _particle[p].age      = 0;
        _particle[p].radius   = 0.01f;
    }
}

void ParticleSystem::simulate(float deltaTime) {

    _timer += deltaTime;

    _gravity            = 0.01f  + 0.01f  * sinf( _timer * 0.52f );
    _airFriction        = 3.0f   + 2.0f   * sinf( _timer * 0.32f );
    _jitter             = 0.05f  + 0.05f  * sinf( _timer * 0.42f );
    _homeAttraction     = 0.01f  + 0.01f  * cosf( _timer * 0.6f  );
    _tornadoForce       = 0.0f   + 0.03f  * sinf( _timer * 0.7f  );
    _neighborAttraction = 0.1f   + 0.1f   * cosf( _timer * 0.8f  );
    _neighborRepulsion  = 0.4f   + 0.3f   * sinf( _timer * 0.4f  );

    _tornadoAxis = glm::vec3
    (
        0.0f + 0.5f * sinf( _timer * 0.55f ),
        1.0f,
        0.0f + 0.5f * cosf( _timer * 0.75f )
    );

    for (unsigned int p = 0; p < _numberOfParticles; p++) {

        // apply random jitter
        _particle[p].velocity += 
        glm::vec3
        (
            -_jitter * ONE_HALF + _jitter * randFloat(), 
            -_jitter * ONE_HALF + _jitter * randFloat(), 
            -_jitter * ONE_HALF + _jitter * randFloat()
        ) * deltaTime;
        
        
        // apply attraction to home position
        glm::vec3 vectorToHome = _home - _particle[p].position;
        _particle[p].velocity += vectorToHome * _homeAttraction * deltaTime;
        
        // apply neighbor attraction
        int neighbor = p + 1;
        if (neighbor == _numberOfParticles ) {
            neighbor = 0;
        }
        glm::vec3 vectorToNeighbor = _particle[p].position - _particle[neighbor].position;
        
        _particle[p].velocity -= vectorToNeighbor * _neighborAttraction * deltaTime;

        float distanceToNeighbor = glm::length(vectorToNeighbor);
        if (distanceToNeighbor > 0.0f) {
            _particle[neighbor].velocity += (vectorToNeighbor / ( 1.0f + distanceToNeighbor * distanceToNeighbor)) * _neighborRepulsion * deltaTime;
        }
        
        // apply tornado force
        glm::vec3 tornadoDirection = glm::cross(vectorToHome, _tornadoAxis);
        _particle[p].velocity += tornadoDirection * _tornadoForce * deltaTime;

        // apply air friction
        float drag = 1.0 - _airFriction * deltaTime;
        if (drag < 0.0f) {
            _particle[p].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        } else {
            _particle[p].velocity *= drag;
        }
        
        // apply gravity
        _particle[p].velocity.y -= _gravity * deltaTime;       

        // update position by velocity
        _particle[p].position += _particle[p].velocity;
        
        // collision with ground
        if (_particle[p].position.y < _particle[p].radius) {
            _particle[p].position.y = _particle[p].radius;
            
            if (_particle[p].velocity.y < 0.0f) {
                _particle[p].velocity.y *= -_bounce;
            }
        }
        
        // collision with sphere
        glm::vec3 vectorToSphereCenter = _TEST_bigSpherePosition - _particle[p].position;
        float distanceToSphereCenter = glm::length(vectorToSphereCenter);
        float combinedRadius = _TEST_bigSphereRadius + _particle[p].radius;
        if (distanceToSphereCenter < combinedRadius) {
        
            if (distanceToSphereCenter > 0.0f){
                glm::vec3 directionToSphereCenter = vectorToSphereCenter / distanceToSphereCenter;
                _particle[p].position = _TEST_bigSpherePosition - directionToSphereCenter * combinedRadius;            
                _particle[p].velocity.y += 0.005f;            
            }
        }
    }
}



void ParticleSystem::render() {

    for (unsigned int p = 0; p < _numberOfParticles; p++) {
        glColor3f(_particle[p].color.x, _particle[p].color.y, _particle[p].color.z);
        glPushMatrix();
        glTranslatef(_particle[p].position.x, _particle[p].position.y, _particle[p].position.z);
        glutSolidSphere(_particle[p].radius, 6, 6);
        glPopMatrix();
    
        // render velocity lines
        glColor4f( _particle[p].color.x, _particle[p].color.y, _particle[p].color.z, 0.5f);
        glm::vec3 end = _particle[p].position - _particle[p].velocity * 2.0f;
        glBegin(GL_LINES);
        glVertex3f(_particle[p].position.x, _particle[p].position.y, _particle[p].position.z);
        glVertex3f(end.x, end.y, end.z);
        
        glEnd();
        
    }
}








