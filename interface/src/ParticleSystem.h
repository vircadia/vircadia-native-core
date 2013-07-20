//
//  ParticleSystem.h
//  hifi
//
//  Created by Jeffrey on July 10, 2013
//

#ifndef hifi_ParticleSystem_h
#define hifi_ParticleSystem_h

#include <glm/gtc/quaternion.hpp>

const int  MAX_PARTICLES = 5000;
const int  MAX_EMITTERS = 20;
const int  NUM_PARTICLE_LIFE_STAGES = 4;
const bool USE_BILLBOARD_RENDERING = false;
const bool SHOW_VELOCITY_TAILS = false;

class ParticleSystem {
public:

    struct ParticleAttributes {
        float     radius;
        glm::vec4 color;
        float     bounce;
        float     gravity;
        float     airFriction;
        float     jitter;
        float     emitterAttraction;
        float     tornadoForce;
        float     neighborAttraction;
        float     neighborRepulsion;
        bool      usingCollisionSphere;
        glm::vec3 collisionSpherePosition;
        float     collisionSphereRadius;
    };  

    ParticleSystem();
    
    int  addEmitter(); // add (create new) emitter and get its unique id
    void emitNow(int emitterIndex);
    void simulate(float deltaTime);
    void killAllParticles();
    void render();
    
    void setUpDirection(glm::vec3 upDirection) {_upDirection = upDirection;} // tell particle system which direction is up
    void setShowingEmitterBaseParticle(int emitterIndex, bool showing );
    void setParticleAttributes        (int emitterIndex, ParticleAttributes attributes); // set attributes for whole life of particles
    void setParticleAttributes        (int emitterIndex, int lifeStage, ParticleAttributes attributes); // set attributes for this life stage of particles
    void setEmitterBaseParticleColor  (int emitterIndex, glm::vec4 color    ) {_emitter[emitterIndex].baseParticle.color  = color;     }
    void setEmitterBaseParticleRadius (int emitterIndex, float     radius   ) {_emitter[emitterIndex].baseParticle.radius = radius;    }
    void setEmitterPosition           (int emitterIndex, glm::vec3 position ) {_emitter[emitterIndex].position            = position;  } 
    void setEmitterDirection          (int emitterIndex, glm::vec3 direction) {_emitter[emitterIndex].direction           = direction; } 
    void setShowingEmitter            (int emitterIndex, bool      showing  ) {_emitter[emitterIndex].visible             = showing;   }  
    void setEmitterParticleLifespan   (int emitterIndex, float     lifespan ) {_emitter[emitterIndex].particleLifespan    = lifespan;  }
    void setEmitterThrust             (int emitterIndex, float     thrust   ) {_emitter[emitterIndex].thrust              = thrust;    }
    void setEmitterRate               (int emitterIndex, float     rate     ) {_emitter[emitterIndex].rate                = rate;      }
    
private:
    
    struct Particle {
        bool      alive;        // is the particle active?
        glm::vec3 position;     // position
        glm::vec3 velocity;     // velocity
        glm::vec4 color;        // color (rgba)
        float     age;          // age in seconds
        float     radius;       // radius
        int       emitterIndex; // which emitter created this particle?
    };  
        
   struct Emitter {
        glm::vec3 position;
        glm::vec3 direction;
        bool      visible;
        float     particleLifespan;
        float     thrust;
        float     rate;
        Particle  baseParticle; // a non-physical particle at the emitter position
        ParticleAttributes particleAttributes[NUM_PARTICLE_LIFE_STAGES]; // the attributes of particles emitted from this emitter
    };  

    glm::vec3 _upDirection;
    Emitter   _emitter[MAX_EMITTERS];
    Particle  _particle[MAX_PARTICLES];
    int       _numParticles;
    int       _numEmitters;
    
    // private methods
    void updateParticle(int index, float deltaTime);
    void createParticle(int e, glm::vec3 velocity);
    void killParticle(int p);
    void renderEmitter(int emitterIndex, float size);
    void renderParticle(int p);
};

#endif
