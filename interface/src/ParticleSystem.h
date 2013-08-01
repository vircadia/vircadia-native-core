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
const int  NULL_EMITTER  = -1;
const int  NULL_PARTICLE = -1;
const int  MAX_EMITTERS  = 100;

enum ParticleRenderStyle
{
    PARTICLE_RENDER_STYLE_SPHERE = 0,
    PARTICLE_RENDER_STYLE_BILLBOARD,
    PARTICLE_RENDER_STYLE_RIBBON,
    NUM_PARTICLE_RENDER_STYLES
};

enum ColorModulationStyle
{
    COLOR_MODULATION_STYLE_NULL = -1,
    COLOR_MODULATION_STYLE_LIGHNTESS_PULSE,
    COLOR_MODULATION_STYLE_LIGHTNESS_WAVE,
    NUM_COLOR_MODULATION_STYLES
};

enum ParticleLifeStage
{
    PARTICLE_LIFESTAGE_0 = 0,
    PARTICLE_LIFESTAGE_1,
    PARTICLE_LIFESTAGE_2,
    PARTICLE_LIFESTAGE_3,
    NUM_PARTICLE_LIFE_STAGES
};

class ParticleSystem {
public:

    struct ParticleAttributes {
        float     radius;                   // radius of the particle
        glm::vec4 color;                    // color (rgba) of the particle
        float     bounce;                   // how much reflection when the particle collides with floor/ground
        float     gravity;                  // force opposite of up direction 
        float     airFriction;              // continual dampening of velocity
        float     jitter;                   // random forces on velocity
        float     emitterAttraction;        // an attraction to the emitter position
        float     tornadoForce;             // force perpendicular to direction axis
        float     neighborAttraction;       // causes particle to be pulled towards next particle in list
        float     neighborRepulsion;        // causes particle to be repelled by previous particle in list
        bool      usingCollisionSphere;     // set to true to allow collision with a sphere
        glm::vec3 collisionSpherePosition;  // position of the collision sphere
        float     collisionSphereRadius;    // radius of the collision sphere
        bool      usingCollisionPlane;      // set to true to allow collision with a plane
        glm::vec3 collisionPlanePosition;   // reference position of the collision plane
        glm::vec3 collisionPlaneNormal;     // the surface normal of the collision plane
        float     modulationAmplitude;      // sets the degree (from 0 to 1) of the modulating effect 
        float     modulationRate;           // the period of modulation, in seconds 
        ColorModulationStyle modulationStyle; // to choose between color modulation styles
    };  

    // public methods...
    ParticleSystem();
    
    int  addEmitter(); // add (create new) emitter and get its unique id
    void simulate(float deltaTime);
    void killAllParticles();
    void render();
    
    void setUpDirection(glm::vec3 upDirection) {_upDirection = upDirection;} // tell particle system which direction is up
    void setParticleAttributesToDefault(ParticleAttributes * attributes); // set these attributes to their default values
    void setParticleAttributes        (int emitterIndex, ParticleAttributes attributes); // set attributes for whole life of particles
    void setParticleAttributes        (int emitterIndex, ParticleLifeStage lifeStage, ParticleAttributes attributes); // set attributes for this life stage
    void setEmitterPosition           (int emitterIndex, glm::vec3           position    );
    void setEmitterParticleResolution (int emitterIndex, int                 resolution  ) {_emitter[emitterIndex].particleResolution  = resolution;  } 
    void setEmitterDirection          (int emitterIndex, glm::vec3           direction   ) {_emitter[emitterIndex].direction           = direction;   } 
    void setShowingEmitter            (int emitterIndex, bool                showing     ) {_emitter[emitterIndex].visible             = showing;     }  
    void setEmitterParticleLifespan   (int emitterIndex, float               lifespan    ) {_emitter[emitterIndex].particleLifespan    = lifespan;    }
    void setParticleRenderStyle       (int emitterIndex, ParticleRenderStyle renderStyle ) {_emitter[emitterIndex].particleRenderStyle = renderStyle; }
    void setEmitterThrust             (int emitterIndex, float               thrust      ) {_emitter[emitterIndex].thrust              = thrust;      }
    void setEmitterRate               (int emitterIndex, float               rate        ) {_emitter[emitterIndex].rate                = rate;        }
    void setShowingEmitterBaseParticle(int emitterIndex, bool                showing     ) {_emitter[emitterIndex].showingBaseParticle = showing;     }

private:
    
    struct Particle {
        bool      alive;            // is the particle active?
        glm::vec3 position;         // position
        glm::vec3 velocity;         // velocity
        glm::vec4 color;            // color (rgba)
        float     age;              // age in seconds
        float     radius;           // radius
        int       emitterIndex;     // which emitter created this particle?
        int       previousParticle; // the last particle that this particle's emitter emitted;
    };  
        
   struct Emitter {
        glm::vec3 position;            // the position of the emitter in world coordinates
        glm::vec3 previousPosition;    // the position of the emitter in the previous time step
        glm::vec3 direction;           // a normalized vector used as an axis for particle emission and other effects
        bool      visible;             // whether or not a line is shown indicating the emitter (indicating its direction)
        float     particleLifespan;    // how long the particle shall live, in seconds
        int       particleResolution;  // for sphere-based particles
        float     emitReserve;         // baed on 'rate', this is the number of particles that need to be emitted at a given time step
        int       numParticlesEmittedThisTime; //the integer number of particles to emit at the preent time step
        float     thrust;              // the initial velocity upon emitting along the emitter direction 
        float     rate;                // currently, how many particles emitted during a simulation time step
        bool      showingBaseParticle; // if true, a copy of particle 0 is shown on the emitter position
        int       currentParticle;     // the index of the most recently-emitted particle
        ParticleAttributes particleAttributes[NUM_PARTICLE_LIFE_STAGES]; // the attributes of particles emitted from this emitter
        ParticleRenderStyle particleRenderStyle;
    };

    glm::vec3 _upDirection;
    Emitter   _emitter[MAX_EMITTERS];
    Particle  _particle[MAX_PARTICLES];
    int       _numEmitters;
    float     _timer;
    
    // private methods
    void updateParticle(int index, float deltaTime);
    void createParticle(int e, float timeFraction);
    void killParticle(int p);
    void renderEmitter(int emitterIndex, float size);
    void renderParticle(int p);
};

#endif
