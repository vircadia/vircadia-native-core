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
#include "Application.h"

ParticleSystem::ParticleSystem() {

    _gravity                 = 0.005;
    _numEmitters             = 0;
    _bounce                  = 0.9f;
    _timer                   = 0.0f;
    _airFriction             = 6.0f;
    _jitter                  = 0.1f;
    _homeAttraction          = 0.0f;
    _tornadoForce            = 0.0f;
    _neighborAttraction      = 0.02f;
    _neighborRepulsion       = 0.9f;
    _collisionSphereRadius   = 0.0f;
    _collisionSpherePosition = glm::vec3(0.0f, 0.0f, 0.0f);
    _numParticles            = 0;
    _usingCollisionSphere    = false;
    
    for (unsigned int e = 0; e < MAX_EMITTERS; e++) {
        _emitter[e].position  = glm::vec3(0.0f, 0.0f, 0.0f);
        _emitter[e].rotation  = glm::quat();
        _emitter[e].right     = IDENTITY_RIGHT;
        _emitter[e].up        = IDENTITY_UP;
        _emitter[e].front     = IDENTITY_FRONT;
    };  
    
    for (unsigned int p = 0; p < MAX_PARTICLES; p++) {
        _particle[p].alive        = false;
        _particle[p].age          = 0.0f;
        _particle[p].radius       = 0.01f;
        _particle[p].emitterIndex = 0;
        _particle[p].position     = glm::vec3(0.0f, 0.0f, 0.0f);
        _particle[p].velocity     = glm::vec3(0.0f, 0.0f, 0.0f);
    }    
}


int ParticleSystem::addEmitter() {
    _numEmitters ++;
    
    if (_numEmitters > MAX_EMITTERS) {
        return -1;
    }
    
    return _numEmitters - 1;
}


void ParticleSystem::simulate(float deltaTime) {
    
    // update emitters
    for (unsigned int e = 0; e < _numEmitters; e++) {
        updateEmitter(e, deltaTime);
    }
    
    // update particles
    for (unsigned int p = 0; p < _numParticles; p++) {
        if (_particle[p].alive) {
            updateParticle(p, deltaTime);
        }
    }
    
    // apply special effects
    runSpecialEffectsTest(deltaTime);
}

void ParticleSystem::updateEmitter(int e, float deltaTime) {

    _emitter[e].front = _emitter[e].rotation * IDENTITY_FRONT;
    _emitter[e].right = _emitter[e].rotation * IDENTITY_RIGHT;
    _emitter[e].up    = _emitter[e].rotation * IDENTITY_UP;    
}



void ParticleSystem::emitParticlesNow(int e, int num) {

    _numParticles = num;
    
    if (_numParticles > MAX_PARTICLES) {
        _numParticles = MAX_PARTICLES;
    }

    for (unsigned int p = 0; p < num; p++) {
        _particle[p].alive = true;
        _particle[p].position = _emitter[e].position;
    }
}

void ParticleSystem::useOrangeBlueColorPalette() {

    for (unsigned int p = 0; p < _numParticles; p++) {

        float radian = ((float)p / (float)_numParticles) * PI_TIMES_TWO;
        float wave   = sinf(radian);
        float red    = 0.5f + 0.5f * wave;
        float green  = 0.3f + 0.3f * wave;
        float blue   = 0.2f - 0.2f * wave;
        _particle[p].color        = glm::vec3(red, green, blue);
    }
}






void ParticleSystem::runSpecialEffectsTest(float deltaTime) {

    _timer += deltaTime;
   
   glm::vec3 tilt = glm::vec3
    (
        30.0f * sinf( _timer * 0.55f ),
        0.0f,
        30.0f * cosf( _timer * 0.75f )
    );
   
    _emitter[0].rotation = glm::quat(glm::radians(tilt));
    
    _gravity            = 0.0f   + 0.02f  * sinf( _timer * 0.52f );
    _airFriction        = 3.0f   + 1.0f   * sinf( _timer * 0.32f );
    _jitter             = 0.05f  + 0.05f  * sinf( _timer * 0.42f );
    _homeAttraction     = 0.01f  + 0.01f  * cosf( _timer * 0.6f  );
    _tornadoForce       = 0.0f   + 0.03f  * sinf( _timer * 0.7f  );
    _neighborAttraction = 0.1f   + 0.1f   * cosf( _timer * 0.8f  );
    _neighborRepulsion  = 0.4f   + 0.3f   * sinf( _timer * 0.4f  );

    if (_gravity < 0.0f) {
        _gravity = 0.0f;
    }
}



void ParticleSystem::updateParticle(int p, float deltaTime) {

    _particle[p].age += deltaTime;

    // apply random jitter
    _particle[p].velocity += 
    glm::vec3
    (
        -_jitter * ONE_HALF + _jitter * randFloat(), 
        -_jitter * ONE_HALF + _jitter * randFloat(), 
        -_jitter * ONE_HALF + _jitter * randFloat()
    ) * deltaTime;
    
    // apply attraction to home position
    glm::vec3 vectorToHome = _emitter[_particle[p].emitterIndex].position - _particle[p].position;
    _particle[p].velocity += vectorToHome * _homeAttraction * deltaTime;
    
    // apply neighbor attraction
    int neighbor = p + 1;
    if (neighbor == _numParticles ) {
        neighbor = 0;
    }
    glm::vec3 vectorToNeighbor = _particle[p].position - _particle[neighbor].position;
    
    _particle[p].velocity -= vectorToNeighbor * _neighborAttraction * deltaTime;

    float distanceToNeighbor = glm::length(vectorToNeighbor);
    if (distanceToNeighbor > 0.0f) {
        _particle[neighbor].velocity += (vectorToNeighbor / ( 1.0f + distanceToNeighbor * distanceToNeighbor)) * _neighborRepulsion * deltaTime;
    }
    
    // apply tornado force
    glm::vec3 tornadoDirection = glm::cross(vectorToHome, _emitter[_particle[p].emitterIndex].up);
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
    if (_usingCollisionSphere) {
        glm::vec3 vectorToSphereCenter = _collisionSpherePosition - _particle[p].position;
        float distanceToSphereCenter = glm::length(vectorToSphereCenter);
        float combinedRadius = _collisionSphereRadius + _particle[p].radius;
        if (distanceToSphereCenter < combinedRadius) {
        
            if (distanceToSphereCenter > 0.0f){
                glm::vec3 directionToSphereCenter = vectorToSphereCenter / distanceToSphereCenter;
                _particle[p].position = _collisionSpherePosition - directionToSphereCenter * combinedRadius;            
            }
        }
    }
}

void ParticleSystem::setCollisionSphere(glm::vec3 position, float radius) {
    _usingCollisionSphere = true;
    _collisionSpherePosition = position; 
    _collisionSphereRadius = radius;
}

void ParticleSystem::render() {

    // render the emitters
    for (unsigned int e = 0; e < _numEmitters; e++) {
        renderEmitter(e, 0.2f);
    };  

    // render the particles
    for (unsigned int p = 0; p < _numParticles; p++) {
        if (_particle[p].alive) {
            renderParticle(p);
        }
    }
}



void ParticleSystem::renderParticle(int p) {

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



void ParticleSystem::renderEmitter(int e, float size) {
    
    glm::vec3 r = _emitter[e].right * size;
    glm::vec3 u = _emitter[e].up    * size;
    glm::vec3 f = _emitter[e].front * size;

    glLineWidth(2.0f);

    glColor3f(0.8f, 0.4, 0.4);
    glBegin(GL_LINES);
    glVertex3f(_emitter[e].position.x, _emitter[e].position.y, _emitter[e].position.z);
    glVertex3f(_emitter[e].position.x + r.x, _emitter[e].position.y + r.y, _emitter[e].position.z + r.z);
    glEnd();

    glColor3f(0.4f, 0.8, 0.4);
    glBegin(GL_LINES);
    glVertex3f(_emitter[e].position.x, _emitter[e].position.y, _emitter[e].position.z);
    glVertex3f(_emitter[e].position.x + u.x, _emitter[e].position.y + u.y, _emitter[e].position.z + u.z);
    glEnd();

    glColor3f(0.4f, 0.4, 0.8);
    glBegin(GL_LINES);
    glVertex3f(_emitter[e].position.x, _emitter[e].position.y, _emitter[e].position.z);
    glVertex3f(_emitter[e].position.x + f.x, _emitter[e].position.y + f.y, _emitter[e].position.z + f.z);
    glEnd();
}







