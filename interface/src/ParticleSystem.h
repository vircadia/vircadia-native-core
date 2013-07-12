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
    
    void setEmitterPosition(int e, glm::vec3 position) { _emitter[e].position = position; }
    void simulate(float deltaTime);
    void render();
    
private:

    struct Particle {
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
    float      _TEST_bigSphereRadius;
    glm::vec3  _TEST_bigSpherePosition;
    
    // private methods
    void updateEmitter(int e, float deltaTime);
    void updateParticle(int index, float deltaTime);
    void runSpecialEffectsTest(float deltaTime);
    void renderEmitter(int emitterIndex, float size);
    void renderParticle(int p);
};

#endif
