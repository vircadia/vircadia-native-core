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

const float DEFAULT_PARTICLE_BOUNCE       = 1.0f;
const float DEFAULT_PARTICLE_AIR_FRICTION = 2.0f;

ParticleSystem::ParticleSystem() {

    _timer        = 0.0f;
    _numEmitters  = 0;
    _numParticles = 0;
    _upDirection  = glm::vec3(0.0f, 1.0f, 0.0f); // default
        
    for (unsigned int e = 0; e < MAX_EMITTERS; e++) {
        _emitter[e].position       = glm::vec3(0.0f, 0.0f, 0.0f);
        _emitter[e].rotation       = glm::quat();
        _emitter[e].right          = IDENTITY_RIGHT;
        _emitter[e].up             = IDENTITY_UP;
        _emitter[e].front          = IDENTITY_FRONT;
        _emitter[e].showingEmitter = false;
        _emitter[e].particleAttributes.bounce                  = DEFAULT_PARTICLE_BOUNCE;
        _emitter[e].particleAttributes.airFriction             = DEFAULT_PARTICLE_AIR_FRICTION;
        _emitter[e].particleAttributes.gravity                 = 0.0f;
        _emitter[e].particleAttributes.jitter                  = 0.0f;
        _emitter[e].particleAttributes.emitterAttraction       = 0.0f;
        _emitter[e].particleAttributes.tornadoForce            = 0.0f;
        _emitter[e].particleAttributes.neighborAttraction      = 0.0f;
        _emitter[e].particleAttributes.neighborRepulsion       = 0.0f;
        _emitter[e].particleAttributes.collisionSphereRadius   = 0.0f;
        _emitter[e].particleAttributes.collisionSpherePosition = glm::vec3(0.0f, 0.0f, 0.0f);
        _emitter[e].particleAttributes.usingCollisionSphere    = false;
    };  
    
    for (unsigned int p = 0; p < MAX_PARTICLES; p++) {
        _particle[p].alive        = false;
        _particle[p].age          = 0.0f;
        _particle[p].lifespan     = 0.0f;
        _particle[p].radius       = 0.0f;
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
}

void ParticleSystem::updateEmitter(int e, float deltaTime) {

    _emitter[e].front = _emitter[e].rotation * IDENTITY_FRONT;
    _emitter[e].right = _emitter[e].rotation * IDENTITY_RIGHT;
    _emitter[e].up    = _emitter[e].rotation * IDENTITY_UP;    
}


void ParticleSystem::emitParticlesNow(int e, int num, float radius, glm::vec4 color, glm::vec3 velocity, float lifespan) {

    for (unsigned int p = 0; p < num; p++) {
        createParticle(e, _emitter[e].position, velocity, radius, color, lifespan);
    }
}

void ParticleSystem::createParticle(int e, glm::vec3 position, glm::vec3 velocity, float radius, glm::vec4 color, float lifespan) {
        
    for (unsigned int p = 0; p < MAX_PARTICLES; p++) {
        if (!_particle[p].alive) {
        
            _particle[p].emitterIndex = e;    
            _particle[p].lifespan     = lifespan;
            _particle[p].alive        = true;
            _particle[p].age          = 0.0f;
            _particle[p].position     = position;
            _particle[p].velocity     = velocity;
            _particle[p].radius       = radius;
            _particle[p].color        = color;

            _numParticles ++;            
                        
            assert(_numParticles <= MAX_PARTICLES);
            
            return;
        }
    }
}

void ParticleSystem::killParticle(int p) {

    assert( p >= 0);
    assert( p < MAX_PARTICLES);
    assert( _numParticles > 0);

    _particle[p].alive = false;
    _numParticles --;
}


void ParticleSystem::setOrangeBlueColorPalette() {

    for (unsigned int p = 0; p < _numParticles; p++) {

        float radian = ((float)p / (float)_numParticles) * PI_TIMES_TWO;
        float wave   = sinf(radian);
        
        float red    = 0.5f + 0.5f * wave;
        float green  = 0.3f + 0.3f * wave;
        float blue   = 0.2f - 0.2f * wave;
        float alpha  = 1.0f;
        
        _particle[p].color = glm::vec4(red, green, blue, alpha);
    }
}


void ParticleSystem::setParticleAttributesForEmitter(int emitterIndex, ParticleAttributes attributes) {

    _emitter[emitterIndex].particleAttributes.bounce                  = attributes.bounce;
    _emitter[emitterIndex].particleAttributes.gravity                 = attributes.gravity;
    _emitter[emitterIndex].particleAttributes.airFriction             = attributes.airFriction;
    _emitter[emitterIndex].particleAttributes.jitter                  = attributes.jitter;
    _emitter[emitterIndex].particleAttributes.emitterAttraction       = attributes.emitterAttraction;
    _emitter[emitterIndex].particleAttributes.tornadoForce            = attributes.tornadoForce;
    _emitter[emitterIndex].particleAttributes.neighborAttraction      = attributes.neighborAttraction;
    _emitter[emitterIndex].particleAttributes.neighborRepulsion       = attributes.neighborRepulsion;
    _emitter[emitterIndex].particleAttributes.usingCollisionSphere    = attributes.usingCollisionSphere;
    _emitter[emitterIndex].particleAttributes.collisionSpherePosition = attributes.collisionSpherePosition;
    _emitter[emitterIndex].particleAttributes.collisionSphereRadius   = attributes.collisionSphereRadius;
}



void ParticleSystem::updateParticle(int p, float deltaTime) {

    _particle[p].age += deltaTime;
    
    if (_particle[p].age > _particle[p].lifespan) {
        killParticle(p);
    }
    
    Emitter myEmitter = _emitter[_particle[p].emitterIndex];

    // apply random jitter
    _particle[p].velocity += 
    glm::vec3
    (
        -myEmitter.particleAttributes.jitter * ONE_HALF + myEmitter.particleAttributes.jitter * randFloat(), 
        -myEmitter.particleAttributes.jitter * ONE_HALF + myEmitter.particleAttributes.jitter * randFloat(), 
        -myEmitter.particleAttributes.jitter * ONE_HALF + myEmitter.particleAttributes.jitter * randFloat()
    ) * deltaTime;
    
    // apply attraction to home position
    glm::vec3 vectorToHome = myEmitter.position - _particle[p].position;
    _particle[p].velocity += vectorToHome * myEmitter.particleAttributes.emitterAttraction * deltaTime;
    
    // apply neighbor attraction
    int neighbor = p + 1;
    if (neighbor == _numParticles ) {
        neighbor = 0;
    }
    
    if ( _particle[neighbor].emitterIndex == _particle[p].emitterIndex) {
        glm::vec3 vectorToNeighbor = _particle[p].position - _particle[neighbor].position;
    
        _particle[p].velocity -= vectorToNeighbor * myEmitter.particleAttributes.neighborAttraction * deltaTime;

        float distanceToNeighbor = glm::length(vectorToNeighbor);
        if (distanceToNeighbor > 0.0f) {
            _particle[neighbor].velocity += (vectorToNeighbor / ( 1.0f + distanceToNeighbor * distanceToNeighbor)) * myEmitter.particleAttributes.neighborRepulsion * deltaTime;
        }
    }
    
    // apply tornado force
    glm::vec3 tornadoDirection = glm::cross(vectorToHome, myEmitter.up);
    _particle[p].velocity += tornadoDirection * myEmitter.particleAttributes.tornadoForce * deltaTime;

    // apply air friction
    float drag = 1.0 - myEmitter.particleAttributes.airFriction * deltaTime;
    if (drag < 0.0f) {
        _particle[p].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    } else {
        _particle[p].velocity *= drag;
    }
    
    // apply gravity
    _particle[p].velocity -= _upDirection * myEmitter.particleAttributes.gravity * deltaTime;       

    // update position by velocity
    _particle[p].position += _particle[p].velocity;
    
    // collision with ground
    if (_particle[p].position.y < _particle[p].radius) {
        _particle[p].position.y = _particle[p].radius;
        
        if (_particle[p].velocity.y < 0.0f) {
            _particle[p].velocity.y *= -myEmitter.particleAttributes.bounce;
        }
    }
    
    // collision with sphere
    if (myEmitter.particleAttributes.usingCollisionSphere) {
        glm::vec3 vectorToSphereCenter = myEmitter.particleAttributes.collisionSpherePosition - _particle[p].position;
        float distanceToSphereCenter = glm::length(vectorToSphereCenter);
        float combinedRadius = myEmitter.particleAttributes.collisionSphereRadius + _particle[p].radius;
        if (distanceToSphereCenter < combinedRadius) {
        
            if (distanceToSphereCenter > 0.0f){
                glm::vec3 directionToSphereCenter = vectorToSphereCenter / distanceToSphereCenter;
                _particle[p].position = myEmitter.particleAttributes.collisionSpherePosition - directionToSphereCenter * combinedRadius;            
            }
        }
    }
}

void ParticleSystem::setCollisionSphere(int e, glm::vec3 position, float radius) {
    _emitter[e].particleAttributes.usingCollisionSphere    = true;
    _emitter[e].particleAttributes.collisionSpherePosition = position; 
    _emitter[e].particleAttributes.collisionSphereRadius   = radius;
}

void ParticleSystem::render() {

    // render the emitters
    for (unsigned int e = 0; e < _numEmitters; e++) {
        if (_emitter[e].showingEmitter) {
            renderEmitter(e, 0.2f);
        }
    };  

    // render the particles
    for (unsigned int p = 0; p < _numParticles; p++) {
        if (_particle[p].alive) {
            renderParticle(p);
        }
    }
}

void ParticleSystem::renderParticle(int p) {

    glColor4f(_particle[p].color.r, _particle[p].color.g, _particle[p].color.b, _particle[p].color.a );
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







