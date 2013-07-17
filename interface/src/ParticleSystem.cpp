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

const float DEFAULT_PARTICLE_RADIUS       = 0.01f;
const float DEFAULT_PARTICLE_BOUNCE       = 1.0f;
const float DEFAULT_PARTICLE_AIR_FRICTION = 2.0f;

ParticleSystem::ParticleSystem() {

    _numEmitters  = 0;
    _numParticles = 0;
    _upDirection  = glm::vec3(0.0f, 1.0f, 0.0f); // default
            
    for (unsigned int e = 0; e < MAX_EMITTERS; e++) {
        _emitter[e].position = glm::vec3(0.0f, 0.0f, 0.0f);
        _emitter[e].rotation = glm::quat();
        _emitter[e].right    = IDENTITY_RIGHT;
        _emitter[e].up       = IDENTITY_UP;
        _emitter[e].front    = IDENTITY_FRONT;
        _emitter[e].visible  = false;
        _emitter[e].baseParticle.alive        = false;
        _emitter[e].baseParticle.age          = 0.0f;
        _emitter[e].baseParticle.lifespan     = 0.0f;
        _emitter[e].baseParticle.radius       = 0.0f;
        _emitter[e].baseParticle.emitterIndex = 0;
        _emitter[e].baseParticle.position     = glm::vec3(0.0f, 0.0f, 0.0f);
        _emitter[e].baseParticle.velocity     = glm::vec3(0.0f, 0.0f, 0.0f);
    
            
        for (int s = 0; s<NUM_PARTICLE_LIFE_STAGES; s++) {
            _emitter[e].particleAttributes[s].radius                  = DEFAULT_PARTICLE_RADIUS;
            _emitter[e].particleAttributes[s].color                   = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
            _emitter[e].particleAttributes[s].bounce                  = DEFAULT_PARTICLE_BOUNCE;
            _emitter[e].particleAttributes[s].airFriction             = DEFAULT_PARTICLE_AIR_FRICTION;
            _emitter[e].particleAttributes[s].gravity                 = 0.0f;
            _emitter[e].particleAttributes[s].jitter                  = 0.0f;
            _emitter[e].particleAttributes[s].emitterAttraction       = 0.0f;
            _emitter[e].particleAttributes[s].tornadoForce            = 0.0f;
            _emitter[e].particleAttributes[s].neighborAttraction      = 0.0f;
            _emitter[e].particleAttributes[s].neighborRepulsion       = 0.0f;
            _emitter[e].particleAttributes[s].collisionSphereRadius   = 0.0f;
            _emitter[e].particleAttributes[s].collisionSpherePosition = glm::vec3(0.0f, 0.0f, 0.0f);
            _emitter[e].particleAttributes[s].usingCollisionSphere    = false;
        }
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
            if (_particle[p].age > _particle[p].lifespan) {
                killParticle(p);
            } else {
                updateParticle(p, deltaTime);
            }
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
            _particle[p].color        = color;

            _particle[p].radius = _emitter[e].particleAttributes[0].radius;
            _particle[p].color  = _emitter[e].particleAttributes[0].color;

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


void ParticleSystem::setParticleAttributes(int emitterIndex, ParticleAttributes attributes) {

    for (int lifeStage = 0; lifeStage < NUM_PARTICLE_LIFE_STAGES; lifeStage ++ ) {
        setParticleAttributes(emitterIndex, lifeStage, attributes);
    }
}

void ParticleSystem::setParticleAttributes(int emitterIndex, int lifeStage, ParticleAttributes attributes) {

    ParticleAttributes * a = &_emitter[emitterIndex].particleAttributes[lifeStage];
    
    a->radius                  = attributes.radius;
    a->color                   = attributes.color;
    a->bounce                  = attributes.bounce;
    a->gravity                 = attributes.gravity;
    a->airFriction             = attributes.airFriction;
    a->jitter                  = attributes.jitter;
    a->emitterAttraction       = attributes.emitterAttraction;
    a->tornadoForce            = attributes.tornadoForce;
    a->neighborAttraction      = attributes.neighborAttraction;
    a->neighborRepulsion       = attributes.neighborRepulsion;
    a->usingCollisionSphere    = attributes.usingCollisionSphere;
    a->collisionSpherePosition = attributes.collisionSpherePosition;
    a->collisionSphereRadius   = attributes.collisionSphereRadius;
}


void ParticleSystem::updateParticle(int p, float deltaTime) {

    assert(_particle[p].age <= _particle[p].lifespan);
    
    float ageFraction = _particle[p].age / _particle[p].lifespan;
        
    int lifeStage = (int)( ageFraction * (NUM_PARTICLE_LIFE_STAGES-1) );

    float lifeStageFraction = ageFraction * ( NUM_PARTICLE_LIFE_STAGES - 1 ) - lifeStage;
    
    /*
    if ( p == 0 ) {
        printf( "lifespan = %f    ageFraction = %f   lifeStage = %d   lifeStageFraction = %f\n", _particle[p].lifespan, ageFraction, lifeStage, lifeStageFraction );
    }
    */
    
    _particle[p].radius
    = _emitter[_particle[p].emitterIndex].particleAttributes[lifeStage  ].radius * (1.0f - lifeStageFraction)
    + _emitter[_particle[p].emitterIndex].particleAttributes[lifeStage+1].radius * lifeStageFraction;

    _particle[p].color
    = _emitter[_particle[p].emitterIndex].particleAttributes[lifeStage  ].color * (1.0f - lifeStageFraction)
    + _emitter[_particle[p].emitterIndex].particleAttributes[lifeStage+1].color * lifeStageFraction;
        
    Emitter myEmitter = _emitter[_particle[p].emitterIndex];

    // apply random jitter
    float j = myEmitter.particleAttributes[lifeStage].jitter;
    _particle[p].velocity += 
    glm::vec3
    (
        -j * ONE_HALF + j * randFloat(), 
        -j * ONE_HALF + j * randFloat(), 
        -j * ONE_HALF + j * randFloat()
    ) * deltaTime;
    
    // apply attraction to home position
    glm::vec3 vectorToHome = myEmitter.position - _particle[p].position;
    _particle[p].velocity += vectorToHome * myEmitter.particleAttributes[lifeStage].emitterAttraction * deltaTime;
    
    // apply neighbor attraction
    int neighbor = p + 1;
    if (neighbor == _numParticles ) {
        neighbor = 0;
    }
    
    if ( _particle[neighbor].emitterIndex == _particle[p].emitterIndex) {
        glm::vec3 vectorToNeighbor = _particle[p].position - _particle[neighbor].position;
    
        _particle[p].velocity -= vectorToNeighbor * myEmitter.particleAttributes[lifeStage].neighborAttraction * deltaTime;

        float distanceToNeighbor = glm::length(vectorToNeighbor);
        if (distanceToNeighbor > 0.0f) {
            _particle[neighbor].velocity += (vectorToNeighbor / ( 1.0f + distanceToNeighbor * distanceToNeighbor)) * myEmitter.particleAttributes[lifeStage].neighborRepulsion * deltaTime;
        }
    }
    
    // apply tornado force
    glm::vec3 tornadoDirection = glm::cross(vectorToHome, myEmitter.up);
    _particle[p].velocity += tornadoDirection * myEmitter.particleAttributes[lifeStage].tornadoForce * deltaTime;

    // apply air friction
    float drag = 1.0 - myEmitter.particleAttributes[lifeStage].airFriction * deltaTime;
    if (drag < 0.0f) {
        _particle[p].velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    } else {
        _particle[p].velocity *= drag;
    }
    
    // apply gravity
    _particle[p].velocity -= _upDirection * myEmitter.particleAttributes[lifeStage].gravity * deltaTime;       

    // update position by velocity
    _particle[p].position += _particle[p].velocity;
    
    // collision with ground
    if (_particle[p].position.y < _particle[p].radius) {
        _particle[p].position.y = _particle[p].radius;
        
        if (_particle[p].velocity.y < 0.0f) {
            _particle[p].velocity.y *= -myEmitter.particleAttributes[lifeStage].bounce;
        }
    }
    
    // collision with sphere
    if (myEmitter.particleAttributes[lifeStage].usingCollisionSphere) {
        glm::vec3 vectorToSphereCenter = myEmitter.particleAttributes[lifeStage].collisionSpherePosition - _particle[p].position;
        float distanceToSphereCenter = glm::length(vectorToSphereCenter);
        float combinedRadius = myEmitter.particleAttributes[lifeStage].collisionSphereRadius + _particle[p].radius;
        if (distanceToSphereCenter < combinedRadius) {
        
            if (distanceToSphereCenter > 0.0f){
                glm::vec3 directionToSphereCenter = vectorToSphereCenter / distanceToSphereCenter;
                _particle[p].position = myEmitter.particleAttributes[lifeStage].collisionSpherePosition - directionToSphereCenter * combinedRadius;            
            }
        }
    }

    // do this at the end...
    _particle[p].age += deltaTime;    
}

void ParticleSystem::setCollisionSphere(int e, glm::vec3 position, float radius) {

    int lifeStage = 0;
    
    _emitter[e].particleAttributes[lifeStage].usingCollisionSphere    = true;
    _emitter[e].particleAttributes[lifeStage].collisionSpherePosition = position; 
    _emitter[e].particleAttributes[lifeStage].collisionSphereRadius   = radius;
}

void ParticleSystem::setEmitterBaseParticle(int emitterIndex, bool showing ) {

    _emitter[emitterIndex].baseParticle.alive = true;
    _emitter[emitterIndex].baseParticle.emitterIndex = emitterIndex;
}

void ParticleSystem::setEmitterBaseParticle(int emitterIndex, bool showing, float radius, glm::vec4 color ) {

    _emitter[emitterIndex].baseParticle.alive        = true;
    _emitter[emitterIndex].baseParticle.emitterIndex = emitterIndex;
    _emitter[emitterIndex].baseParticle.radius       = radius;
    _emitter[emitterIndex].baseParticle.color        = color;
}


void ParticleSystem::render() {

    // render the emitters
    for (int e = 0; e < _numEmitters; e++) {

        if (_emitter[e].baseParticle.alive) {
            glColor4f(_emitter[e].baseParticle.color.r, _emitter[e].baseParticle.color.g, _emitter[e].baseParticle.color.b, _emitter[e].baseParticle.color.a );
            glPushMatrix();
            glTranslatef(_emitter[e].position.x, _emitter[e].position.y, _emitter[e].position.z);
            glutSolidSphere(_emitter[e].baseParticle.radius, 6, 6);
            glPopMatrix();
        }

        if (_emitter[e].visible) {
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

    if (USE_BILLBOARD_RENDERING) {
        glm::vec3 cameraPosition = Application::getInstance()->getCamera()->getPosition();
        glm::vec3 viewVector = _particle[p].position - cameraPosition;
        float distance = glm::length(viewVector);
        
        if (distance >= 0.0f) {
            viewVector /= distance;
            glm::vec3 up    = glm::vec3(viewVector.y, viewVector.z, viewVector.x);
            glm::vec3 right = glm::vec3(viewVector.z, viewVector.x, viewVector.y);
            
            glm::vec3 p0 = _particle[p].position - right * _particle[p].radius - up * _particle[p].radius;
            glm::vec3 p1 = _particle[p].position + right * _particle[p].radius - up * _particle[p].radius;
            glm::vec3 p2 = _particle[p].position + right * _particle[p].radius + up * _particle[p].radius;
            glm::vec3 p3 = _particle[p].position - right * _particle[p].radius + up * _particle[p].radius;
            
            glBegin(GL_TRIANGLES);             
            glVertex3f(p0.x, p0.y, p0.z); 
            glVertex3f(p1.x, p1.y, p1.z); 
            glVertex3f(p2.x, p2.y, p2.z); 
            glEnd();

            glBegin(GL_TRIANGLES);             
            glVertex3f(p0.x, p0.y, p0.z); 
            glVertex3f(p2.x, p2.y, p2.z); 
            glVertex3f(p3.x, p3.y, p3.z); 
            glEnd();
        }
    } else {
        glPushMatrix();
        glTranslatef(_particle[p].position.x, _particle[p].position.y, _particle[p].position.z);
        glutSolidSphere(_particle[p].radius, 6, 6);
        glPopMatrix();

        if (SHOW_VELOCITY_TAILS) {
            glColor4f( _particle[p].color.x, _particle[p].color.y, _particle[p].color.z, 0.5f);
            glm::vec3 end = _particle[p].position - _particle[p].velocity * 2.0f;
            glBegin(GL_LINES);
            glVertex3f(_particle[p].position.x, _particle[p].position.y, _particle[p].position.z);
            glVertex3f(end.x, end.y, end.z);            
            glEnd();
        }
    }
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







