//
//  ParticleSystem.h
//  hifi
//
//  Created by Jeffrey on July 10, 2013
//
//

#ifndef hifi_ParticleSystem_h
#define hifi_ParticleSystem_h

#include <glm/gtc/quaternion.hpp>

const int MAX_PARTICLES = 5000;
const int MAX_EMITTERS  = 10;

class ParticleSystem {
public:
    ParticleSystem();
    
    int addEmitter();                                           // add (create) an emitter and get its unique id
    void useOrangeBlueColorPalette();                           // apply a nice preset color palette to the particles
    void setCollisionSphere(glm::vec3 position, float radius);  // specify a sphere for the particles to collide with
    void emitParticlesNow(int e, int numParticles);             // tell this emitter to generate this many particles right now
    void simulate(float deltaTime);                             // run it
    void render();                                              // show it
    void setEmitterPosition(int e, glm::vec3 position) { _emitter[e].position = position; } // set the position of this emitter
    
private:

    struct Particle {
        bool        alive;
        glm::vec3   position;
        glm::vec3   velocity;
        glm::vec3   color;
        float       age;
        float       radius;
        int         emitterIndex;
    };  

    struct Emitter {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 right;
        glm::vec3 up;
        glm::vec3 front;
    };  
    
    float      _bounce;
    float      _gravity;
    float      _timer;
    Emitter    _emitter[MAX_EMITTERS];
    Particle   _particle[MAX_PARTICLES];
    int        _numParticles;
    int        _numEmitters;
    float      _airFriction;
    float      _jitter;
    float      _homeAttraction;
    float      _tornadoForce;
    float      _neighborAttraction;
    float      _neighborRepulsion;
    bool       _usingCollisionSphere;
    glm::vec3  _collisionSpherePosition;
    float      _collisionSphereRadius;
    
    // private methods
    void updateEmitter(int e, float deltaTime);
    void updateParticle(int index, float deltaTime);
    void runSpecialEffectsTest(float deltaTime);
    void renderEmitter(int emitterIndex, float size);
    void renderParticle(int p);
};

#endif
