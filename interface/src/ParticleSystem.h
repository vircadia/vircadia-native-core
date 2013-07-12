//
//  ParticleSystem.h
//  hifi
//
//  Created by Jeffrey on July 10, 2013
//
//

#ifndef hifi_ParticleSystem_h
#define hifi_ParticleSystem_h

const int MAX_PARTICLES = 5000;
const int MAX_EMITTERS  = 10;

class ParticleSystem {
public:
    ParticleSystem();
    
    void simulate(float deltaTime);
    void render();
    
private:

    struct Particle {
        glm::vec3   position;
        glm::vec3   velocity;
        glm::vec3   color;
        float       age;
        float       radius;
    };  

    struct Emitter {
        glm::vec3 position;
        glm::vec3 direction;
    };  
    
    float      _bounce;
    float      _gravity;
    float      _timer;
    Emitter    _emitter[MAX_EMITTERS];
    Particle   _particle[MAX_PARTICLES];
    int        _numberOfParticles;
    glm::vec3  _home;
    glm::vec3  _tornadoAxis;
    float      _airFriction;
    float      _jitter;
    float      _homeAttraction;
    float      _tornadoForce;
    float      _neighborAttraction;
    float      _neighborRepulsion;
    float      _TEST_bigSphereRadius;
    glm::vec3  _TEST_bigSpherePosition;
    
    // private methods
    void updateParticle(int index, float deltaTime);
    void runSpecialEffectsTest(float deltaTime);
};

#endif
