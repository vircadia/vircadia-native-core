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

const int  MAX_PARTICLES = 5000;
const int  MAX_EMITTERS  = 20;
const int  NUM_PARTICLE_LIFE_STAGES = 3;
const bool USE_BILLBOARD_RENDERING = false;

class ParticleSystem {
public:

    struct ParticleAttributes {
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
    
    int  addEmitter(); // add (create) an emitter and get its unique id
    void emitParticlesNow(int emitterIndex, int numParticles, float radius, glm::vec4 color, glm::vec3 velocity, float lifespan);
    void simulate(float deltaTime);
    void render();
     
    void setOrangeBlueColorPalette(); // apply a nice preset color palette to the particles
    void setUpDirection(glm::vec3 upDirection) {_upDirection = upDirection;} // tell particle system which direction is up
    void setParticleAttributes(int emitterIndex,                ParticleAttributes attributes);
    void setParticleAttributes(int emitterIndex, int lifeStage, ParticleAttributes attributes);
    void setCollisionSphere   (int emitterIndex, glm::vec3 position, float radius);  // specify a sphere for the particles to collide with
    void setEmitterPosition   (int emitterIndex, glm::vec3 position) { _emitter[emitterIndex].position = position; } // set position of emitter
    void setEmitterRotation   (int emitterIndex, glm::quat rotation) { _emitter[emitterIndex].rotation = rotation; } // set rotation of emitter
    void setShowingEmitter    (int emitterIndex, bool showing      ) { _emitter[emitterIndex].visible  = showing;  } // set its visibiity
    
private:
    
   struct Emitter {
        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 right;    // derived from rotation
        glm::vec3 up;       // derived from rotation
        glm::vec3 front;    // derived from rotation
        bool      visible;
        ParticleAttributes particleAttributes[NUM_PARTICLE_LIFE_STAGES]; // the attributes of particles emitted from this emitter
    };  

    struct Particle {
        bool        alive;        // is the particle active?
        glm::vec3   position;     // position
        glm::vec3   velocity;     // velocity
        glm::vec4   color;        // color (rgba)
        float       age;          // age in seconds
        float       radius;       // radius
        float       lifespan;     // how long this particle stays alive (in seconds)
        int         emitterIndex; // which emitter created this particle?
    };  
        
    glm::vec3  _upDirection;
    Emitter    _emitter[MAX_EMITTERS];
    Particle   _particle[MAX_PARTICLES];
    int        _numParticles;
    int        _numEmitters;
    
    // private methods
    void updateEmitter(int e, float deltaTime);
    void updateParticle(int index, float deltaTime);
    void createParticle(int e, glm::vec3 position, glm::vec3 velocity, float radius, glm::vec4 color, float lifespan);
    void killParticle(int p);
    void renderEmitter(int emitterIndex, float size);
    void renderParticle(int p);
};

#endif
