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

const float DEFAULT_PARTICLE_RADIUS            = 0.01f;
const float DEFAULT_PARTICLE_BOUNCE            = 1.0f;
const float DEFAULT_PARTICLE_AIR_FRICTION      = 2.0f;
const float DEFAULT_PARTICLE_LIFESPAN          = 1.0f;
const int   DEFAULT_PARTICLE_SPHERE_RESOLUTION = 6;
const float DEFAULT_EMITTER_RENDER_LENGTH      = 0.2f;

ParticleSystem::ParticleSystem() {

    _timer        = 0.0f;
    _numEmitters  = 0;
    _upDirection  = glm::vec3(0.0f, 1.0f, 0.0f); // default
            
    for (unsigned int emitterIndex = 0; emitterIndex < MAX_EMITTERS; emitterIndex++) {
        
        Emitter * e = &_emitter[emitterIndex];
        e->position            = glm::vec3(0.0f, 0.0f, 0.0f);
        e->previousPosition    = glm::vec3(0.0f, 0.0f, 0.0f);
        e->direction           = glm::vec3(0.0f, 1.0f, 0.0f);
        e->visible             = false;
        e->particleResolution  = DEFAULT_PARTICLE_SPHERE_RESOLUTION;
        e->particleLifespan    = DEFAULT_PARTICLE_LIFESPAN;
        e->showingBaseParticle = false;
        e->emitReserve         = 0.0;
        e->thrust              = 0.0f;
        e->rate                = 0.0f;
        e->currentParticle     = 0;
        e->particleRenderStyle = PARTICLE_RENDER_STYLE_SPHERE;
        e->numParticlesEmittedThisTime = 0;
            
        for (int lifeStage = 0; lifeStage < NUM_PARTICLE_LIFE_STAGES; lifeStage++) {
            setParticleAttributesToDefault(&_emitter[emitterIndex].particleAttributes[lifeStage]);
        }
    };  
    
    for (unsigned int p = 0; p < MAX_PARTICLES; p++) {
        _particle[p].alive            = false;
        _particle[p].age              = 0.0f;
        _particle[p].radius           = 0.0f;
        _particle[p].emitterIndex     = 0;
        _particle[p].previousParticle = NULL_PARTICLE;
        _particle[p].position         = glm::vec3(0.0f, 0.0f, 0.0f);
        _particle[p].velocity         = glm::vec3(0.0f, 0.0f, 0.0f);
    }    
}

int ParticleSystem::addEmitter() {

    if (_numEmitters < MAX_EMITTERS) {
        _numEmitters ++;
        return _numEmitters - 1;
    }
    
    return NULL_EMITTER;
}


void ParticleSystem::simulate(float deltaTime) {
    
    _timer += deltaTime;
    
    // emit particles
    for (int e = 0; e < _numEmitters; e++) {
        
        assert(e >= 0);
        assert(e <= MAX_EMITTERS);
        assert(_emitter[e].rate >= 0);
        
        _emitter[e].emitReserve += _emitter[e].rate * deltaTime;
        _emitter[e].numParticlesEmittedThisTime = (int)_emitter[e].emitReserve;
        _emitter[e].emitReserve -= _emitter[e].numParticlesEmittedThisTime;
    
        for (int p = 0; p < _emitter[e].numParticlesEmittedThisTime; p++) {
            float timeFraction = (float)p / (float)_emitter[e].numParticlesEmittedThisTime;
            createParticle(e, timeFraction);
        }
    }

    // update particles
    
    for (int p = 0; p < MAX_PARTICLES; p++) {
        if (_particle[p].alive) {        
            if (_particle[p].age > _emitter[_particle[p].emitterIndex].particleLifespan) {            
                killParticle(p);
            } else {
                updateParticle(p, deltaTime);
            }
        }
    }
}

void ParticleSystem::createParticle(int e, float timeFraction) {
        
    for (unsigned int p = 0; p < MAX_PARTICLES; p++) {
        if (!_particle[p].alive) {
                
            _particle[p].emitterIndex     = e;    
            _particle[p].alive            = true;
            _particle[p].age              = 0.0f;
            _particle[p].velocity         = _emitter[e].direction * _emitter[e].thrust;
            _particle[p].position         = _emitter[e].previousPosition + timeFraction * (_emitter[e].position - _emitter[e].previousPosition);
            _particle[p].radius           = _emitter[e].particleAttributes[PARTICLE_LIFESTAGE_0].radius;
            _particle[p].color            = _emitter[e].particleAttributes[PARTICLE_LIFESTAGE_0].color;
            _particle[p].previousParticle = NULL_PARTICLE;
            
            if (_particle[_emitter[e].currentParticle].alive) {            
                if (_particle[_emitter[e].currentParticle].emitterIndex == e) {
                    _particle[p].previousParticle = _emitter[e].currentParticle;
                }
            }
            
            _emitter[e].currentParticle = p;
            
            break;
        }
    }
}

void ParticleSystem::killParticle(int p) {

    assert(p >= 0);
    assert(p < MAX_PARTICLES);

    _particle[p].alive            = false;
    _particle[p].previousParticle = NULL_PARTICLE;
    _particle[p].position         = _emitter[_particle[p].emitterIndex].position;
    _particle[p].velocity         = glm::vec3(0.0f, 0.0f, 0.0f);
    _particle[p].age              = 0.0f;
    _particle[p].emitterIndex     = NULL_PARTICLE; 
    _particle[p].color            = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    _particle[p].radius           = 0.0f;
 }


void ParticleSystem::setEmitterPosition(int emitterIndex, glm::vec3 position) {
    _emitter[emitterIndex].previousPosition = _emitter[emitterIndex].position;    
    _emitter[emitterIndex].position = position;    
} 


void ParticleSystem::setParticleAttributes(int emitterIndex, ParticleAttributes attributes) {

    for (int lifeStage = 0; lifeStage < NUM_PARTICLE_LIFE_STAGES; lifeStage ++) {
        setParticleAttributes(emitterIndex, (ParticleLifeStage)lifeStage, attributes);
    }
}

void ParticleSystem::setParticleAttributesToDefault(ParticleAttributes * a) {

    a->radius                  = DEFAULT_PARTICLE_RADIUS;
    a->color                   = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    a->bounce                  = DEFAULT_PARTICLE_BOUNCE;
    a->airFriction             = DEFAULT_PARTICLE_AIR_FRICTION;
    a->gravity                 = 0.0f;
    a->jitter                  = 0.0f;
    a->emitterAttraction       = 0.0f;
    a->tornadoForce            = 0.0f;
    a->neighborAttraction      = 0.0f;
    a->neighborRepulsion       = 0.0f;
    a->collisionSphereRadius   = 0.0f;
    a->collisionSpherePosition = glm::vec3(0.0f, 0.0f, 0.0f);
    a->usingCollisionSphere    = false;
    a->collisionPlaneNormal    = _upDirection;
    a->collisionPlanePosition  = glm::vec3(0.0f, 0.0f, 0.0f);
    a->usingCollisionPlane     = false;
    a->modulationAmplitude     = 0.0f;
    a->modulationRate          = 0.0;
    a->modulationStyle         = COLOR_MODULATION_STYLE_NULL;

}


void ParticleSystem::setParticleAttributes(int emitterIndex, ParticleLifeStage lifeStage, ParticleAttributes attributes) {

    assert(lifeStage >= 0);
    assert(lifeStage <  NUM_PARTICLE_LIFE_STAGES);

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
    a->usingCollisionPlane     = attributes.usingCollisionPlane;
    a->collisionPlanePosition  = attributes.collisionPlanePosition;
    a->collisionPlaneNormal    = attributes.collisionPlaneNormal;
    a->modulationAmplitude     = attributes.modulationAmplitude;
    a->modulationRate          = attributes.modulationRate;
    a->modulationStyle         = attributes.modulationStyle;
}



void ParticleSystem::updateParticle(int p, float deltaTime) {

    Emitter myEmitter = _emitter[_particle[p].emitterIndex];

    assert(_particle[p].age <= myEmitter.particleLifespan);

    float ageFraction = 0.0f;
    int   lifeStage = 0;
    float lifeStageFraction = 0.0f;

    if (_emitter[_particle[p].emitterIndex].particleLifespan > 0.0) {
        
        ageFraction       = _particle[p].age / myEmitter.particleLifespan;
        lifeStage         = (int)(ageFraction * (NUM_PARTICLE_LIFE_STAGES - 1));
        lifeStageFraction = ageFraction * (NUM_PARTICLE_LIFE_STAGES - 1) - lifeStage;
            
        // adjust radius
        _particle[p].radius
        = myEmitter.particleAttributes[lifeStage  ].radius * (1.0f - lifeStageFraction)
        + myEmitter.particleAttributes[lifeStage+1].radius * lifeStageFraction;
        
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
        if (neighbor == MAX_PARTICLES) {
            neighbor = 0;
        }
        
        if (_particle[neighbor].emitterIndex == _particle[p].emitterIndex) {
            glm::vec3 vectorToNeighbor = _particle[p].position - _particle[neighbor].position;
        
            _particle[p].velocity -= vectorToNeighbor * myEmitter.particleAttributes[lifeStage].neighborAttraction * deltaTime;

            float distanceToNeighbor = glm::length(vectorToNeighbor);
            if (distanceToNeighbor > 0.0f) {
                _particle[neighbor].velocity += (vectorToNeighbor / (1.0f + distanceToNeighbor * distanceToNeighbor)) * myEmitter.particleAttributes[lifeStage].neighborRepulsion * deltaTime;
            }
        }
        
        // apply tornado force
        glm::vec3 tornadoDirection = glm::cross(vectorToHome, myEmitter.direction);
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
                
        // collision with the plane surface
        if (myEmitter.particleAttributes[lifeStage].usingCollisionPlane) {
            glm::vec3 vectorFromParticleToPlanePosition = _particle[p].position - myEmitter.particleAttributes[lifeStage].collisionPlanePosition;
            glm::vec3 normal = myEmitter.particleAttributes[lifeStage].collisionPlaneNormal;
            float dot = glm::dot(vectorFromParticleToPlanePosition, normal);
            if (dot < _particle[p].radius) {
                _particle[p].position += normal * (_particle[p].radius - dot);
                float planeNormalComponentOfVelocity = glm::dot(_particle[p].velocity, normal);
                _particle[p].velocity -= normal * planeNormalComponentOfVelocity * (1.0f + myEmitter.particleAttributes[lifeStage].bounce);
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
    }

    // adjust color
    _particle[p].color
    = myEmitter.particleAttributes[lifeStage  ].color * (1.0f - lifeStageFraction)
    + myEmitter.particleAttributes[lifeStage+1].color * lifeStageFraction;
       
    // apply color modulation
    if (myEmitter.particleAttributes[lifeStage  ].modulationAmplitude > 0.0f) {
        float modulation = 0.0f;
        float radian = _timer * myEmitter.particleAttributes[lifeStage  ].modulationRate * PI_TIMES_TWO;
        if (myEmitter.particleAttributes[lifeStage  ].modulationStyle == COLOR_MODULATION_STYLE_LIGHNTESS_PULSE) {
            if (sinf(radian) > 0.0f) {
                modulation = myEmitter.particleAttributes[lifeStage].modulationAmplitude;
            }
        } else if (myEmitter.particleAttributes[lifeStage].modulationStyle == COLOR_MODULATION_STYLE_LIGHTNESS_WAVE) {
            float a = myEmitter.particleAttributes[lifeStage].modulationAmplitude;
            modulation = a * ONE_HALF + sinf(radian) * a * ONE_HALF;
        }
        
        _particle[p].color.r += modulation;
        _particle[p].color.g += modulation;
        _particle[p].color.b += modulation;
        _particle[p].color.a += modulation;
        
        if (_particle[p].color.r > 1.0f) {_particle[p].color.r = 1.0f;}
        if (_particle[p].color.g > 1.0f) {_particle[p].color.g = 1.0f;}
        if (_particle[p].color.b > 1.0f) {_particle[p].color.b = 1.0f;}
        if (_particle[p].color.a > 1.0f) {_particle[p].color.a = 1.0f;}
    }
    
    // do this at the end...
    _particle[p].age += deltaTime;  
}


void ParticleSystem::killAllParticles() {

    for (int e = 0; e < _numEmitters; e++) {
        _emitter[e].currentParticle             = NULL_PARTICLE;
        _emitter[e].emitReserve                 = 0.0f;
        _emitter[e].previousPosition            = _emitter[e].position;
        _emitter[e].rate                        = 0.0f;
        _emitter[e].currentParticle             = 0;
        _emitter[e].numParticlesEmittedThisTime = 0;
    }
    
    for (int p = 0; p < MAX_PARTICLES; p++) {
        killParticle(p);
    }
}

void ParticleSystem::render() {

    // render the emitters
    for (int e = 0; e < _numEmitters; e++) {

        if (_emitter[e].showingBaseParticle) {
            glColor4f(_particle[0].color.r, _particle[0].color.g, _particle[0].color.b, _particle[0].color.a);
            glPushMatrix();
            glTranslatef(_emitter[e].position.x, _emitter[e].position.y, _emitter[e].position.z);
            glutSolidSphere(_particle[0].radius, _emitter[e].particleResolution, _emitter[e].particleResolution);
            glPopMatrix();
        }

        if (_emitter[e].visible) {
            renderEmitter(e, DEFAULT_EMITTER_RENDER_LENGTH);
        }
    };  
    
        // render the particles
    for (int p = 0; p < MAX_PARTICLES; p++) {
        if (_particle[p].alive) {
            if (_emitter[_particle[p].emitterIndex].particleLifespan > 0.0) {
                renderParticle(p);
             }
        }
    }
}

void ParticleSystem::renderParticle(int p) {

    glColor4f(_particle[p].color.r, _particle[p].color.g, _particle[p].color.b, _particle[p].color.a);

    if (_emitter[_particle[p].emitterIndex].particleRenderStyle == PARTICLE_RENDER_STYLE_BILLBOARD) {
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
    } else if (_emitter[_particle[p].emitterIndex].particleRenderStyle == PARTICLE_RENDER_STYLE_SPHERE) {

        glPushMatrix();
            glTranslatef(_particle[p].position.x, _particle[p].position.y, _particle[p].position.z);            
            glutSolidSphere(_particle[p].radius, _emitter[_particle[p].emitterIndex].particleResolution, _emitter[_particle[p].emitterIndex].particleResolution);
        glPopMatrix();

    } else if (_emitter[_particle[p].emitterIndex].particleRenderStyle == PARTICLE_RENDER_STYLE_RIBBON) {
    
        if (_particle[p].previousParticle != NULL_PARTICLE) {
            if ((_particle[p].alive)
            &&  (_particle[_particle[p].previousParticle].alive)
            &&  (_particle[_particle[p].previousParticle].emitterIndex == _particle[p].emitterIndex)) {
            
                glm::vec3 vectorFromPreviousParticle = _particle[p].position - _particle[_particle[p].previousParticle].position;
                float distance = glm::length(vectorFromPreviousParticle);

                if (distance > 0.0f) {
                
                    vectorFromPreviousParticle /= distance;

                    glm::vec3 up    = glm::normalize(glm::cross(vectorFromPreviousParticle, _upDirection)) * _particle[p].radius;
                    glm::vec3 right = glm::normalize(glm::cross(up, vectorFromPreviousParticle          )) * _particle[p].radius;
                                        
                    glm::vec3 p0Left  = _particle[p ].position - right;
                    glm::vec3 p0Right = _particle[p ].position + right;
                    glm::vec3 p0Down  = _particle[p ].position - up;
                    glm::vec3 p0Up    = _particle[p ].position + up;

                    glm::vec3 ppLeft  = _particle[_particle[p].previousParticle].position - right;
                    glm::vec3 ppRight = _particle[_particle[p].previousParticle].position + right;
                    glm::vec3 ppDown  = _particle[_particle[p].previousParticle].position - up;
                    glm::vec3 ppUp    = _particle[_particle[p].previousParticle].position + up;
                                        
                    glBegin(GL_TRIANGLES);    
                             
                        glVertex3f(p0Left.x,  p0Left.y,  p0Left.z ); 
                        glVertex3f(p0Right.x, p0Right.y, p0Right.z); 
                        glVertex3f(ppLeft.x,  ppLeft.y,  ppLeft.z ); 

                        glVertex3f(p0Right.x, p0Right.y, p0Right.z); 
                        glVertex3f(ppLeft.x,  ppLeft.y,  ppLeft.z ); 
                        glVertex3f(ppRight.x, ppRight.y, ppRight.z); 

                        glVertex3f(p0Up.x,    p0Up.y,    p0Up.z   ); 
                        glVertex3f(p0Down.x,  p0Down.y,  p0Down.z ); 
                        glVertex3f(ppDown.x,  ppDown.y,  ppDown.z ); 

                        glVertex3f(p0Up.x,    p0Up.y,    p0Up.z   ); 
                        glVertex3f(ppUp.x,    ppUp.y,    ppUp.z   ); 
                        glVertex3f(ppDown.x,  ppDown.y,  ppDown.z ); 

                        glVertex3f(p0Up.x,    p0Up.y,    p0Left.z ); 
                        glVertex3f(p0Right.x, p0Right.y, p0Right.z); 
                        glVertex3f(p0Down.x,  p0Down.y,  p0Down.z ); 

                        glVertex3f(p0Up.x,    p0Up.y,    p0Left.z ); 
                        glVertex3f(p0Left.x,  p0Left.y,  p0Left.z ); 
                        glVertex3f(p0Down.x,  p0Down.y,  p0Down.z ); 

                        glVertex3f(ppUp.x,    ppUp.y,    ppLeft.z ); 
                        glVertex3f(ppRight.x, ppRight.y, ppRight.z); 
                        glVertex3f(ppDown.x,  ppDown.y,  ppDown.z ); 

                        glVertex3f(ppUp.x,    ppUp.y,    ppLeft.z ); 
                        glVertex3f(ppLeft.x,  ppLeft.y,  ppLeft.z ); 
                        glVertex3f(ppDown.x,  ppDown.y,  ppDown.z ); 
                        
                    glEnd();
                }
            }                        
        }
    }
}

void ParticleSystem::renderEmitter(int e, float size) {

    glm::vec3 v = _emitter[e].direction * size;
    
    glColor3f(0.4f, 0.4, 0.8);
    glBegin(GL_LINES);
    glVertex3f(_emitter[e].position.x, _emitter[e].position.y, _emitter[e].position.z);
    glVertex3f(_emitter[e].position.x + v.x, _emitter[e].position.y + v.y, _emitter[e].position.z + v.z);
    glEnd();
}





