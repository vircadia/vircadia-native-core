//
//  ParticleEffectEntityItem.cpp
//  libraries/entities/src
//
//  Some starter code for a particle simulation entity, which could ideally be used for a variety of effects.
//  This is some really early and rough stuff here.  It was enough for now to just get it up and running in the interface.
//
//  Todo's and other notes:
//  - The simulation should restart when the AnimationLoop's max frame is reached (or passed), but there doesn't seem
//    to be a good way to set that max frame to something reasonable right now.
//  - There seems to be a bug whereby entities on the edge of screen will just pop off or on.  This is probably due
//    to my lack of understanding of how entities in the octree are picked for rendering.  I am updating the entity
//    dimensions based on the bounds of the sim, but maybe I need to update a dirty flag or something.
//  - This should support some kind of pre-roll of the simulation.
//  - Just to get this out the door, I just did forward Euler integration.  There are better ways.
//  - Gravity always points along the Y axis.  Support an actual gravity vector.
//  - Add the ability to add arbitrary forces to the simulation.
//  - Add drag.
//  - Add some kind of support for collisions.
//  - There's no synchronization of the simulation across clients at all.  In fact, it's using rand() under the hood, so
//    there's no gaurantee that different clients will see simulations that look anything like the other.
//
//  Created by Jason Rickwald on 3/2/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include <glm/gtx/transform.hpp>
#include <QtCore/QJsonDocument>

#include <ByteCountCoding.h>
#include <GeometryUtil.h>
#include <Interpolate.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"
#include "EntityScriptingInterface.h"
#include "ParticleEffectEntityItem.h"

const float SCRIPT_MAXIMUM_PI = 3.1416f;  // Round up so that reasonable property values work

const xColor ParticleEffectEntityItem::DEFAULT_COLOR = { 255, 255, 255 };
const xColor ParticleEffectEntityItem::DEFAULT_COLOR_SPREAD = { 0, 0, 0 };
const float ParticleEffectEntityItem::DEFAULT_ALPHA = 1.0f;
const float ParticleEffectEntityItem::DEFAULT_ALPHA_SPREAD = 0.0f;
const float ParticleEffectEntityItem::DEFAULT_ALPHA_START = DEFAULT_ALPHA;
const float ParticleEffectEntityItem::DEFAULT_ALPHA_FINISH = DEFAULT_ALPHA;
const float ParticleEffectEntityItem::MINIMUM_ALPHA = 0.0f;
const float ParticleEffectEntityItem::MAXIMUM_ALPHA = 1.0f;
const quint32 ParticleEffectEntityItem::DEFAULT_MAX_PARTICLES = 1000;
const quint32 ParticleEffectEntityItem::MINIMUM_MAX_PARTICLES = 1;
const quint32 ParticleEffectEntityItem::MAXIMUM_MAX_PARTICLES = 100000;
const float ParticleEffectEntityItem::DEFAULT_LIFESPAN = 3.0f;
const float ParticleEffectEntityItem::MINIMUM_LIFESPAN = 0.0f;
const float ParticleEffectEntityItem::MAXIMUM_LIFESPAN = 86400.0f;  // 1 day
const float ParticleEffectEntityItem::DEFAULT_EMIT_RATE = 15.0f;
const float ParticleEffectEntityItem::MINIMUM_EMIT_RATE = 0.0f;
const float ParticleEffectEntityItem::MAXIMUM_EMIT_RATE = 100000.0f;
const float ParticleEffectEntityItem::DEFAULT_EMIT_SPEED = 5.0f;
const float ParticleEffectEntityItem::MINIMUM_EMIT_SPEED = 0.0f;
const float ParticleEffectEntityItem::MAXIMUM_EMIT_SPEED = 1000.0f;  // Approx mach 3
const float ParticleEffectEntityItem::DEFAULT_SPEED_SPREAD = 1.0f;
const glm::quat ParticleEffectEntityItem::DEFAULT_EMIT_ORIENTATION = glm::angleAxis(-PI_OVER_TWO, Vectors::UNIT_X);  // Vertical
const glm::vec3 ParticleEffectEntityItem::DEFAULT_EMIT_DIMENSIONS = Vectors::ZERO;  // Emit from point
const float ParticleEffectEntityItem::MINIMUM_EMIT_DIMENSION = 0.0f;
const float ParticleEffectEntityItem::MAXIMUM_EMIT_DIMENSION = (float)TREE_SCALE;
const float ParticleEffectEntityItem::DEFAULT_EMIT_RADIUS_START = 1.0f;  // Emit from surface (when emitDimensions > 0)
const float ParticleEffectEntityItem::MINIMUM_EMIT_RADIUS_START = 0.0f;
const float ParticleEffectEntityItem::MAXIMUM_EMIT_RADIUS_START = 1.0f;
const float ParticleEffectEntityItem::MINIMUM_POLAR = 0.0f;
const float ParticleEffectEntityItem::MAXIMUM_POLAR = SCRIPT_MAXIMUM_PI;
const float ParticleEffectEntityItem::DEFAULT_POLAR_START = 0.0f;  // Emit along z-axis
const float ParticleEffectEntityItem::DEFAULT_POLAR_FINISH = 0.0f; // ""
const float ParticleEffectEntityItem::MINIMUM_AZIMUTH = -SCRIPT_MAXIMUM_PI;
const float ParticleEffectEntityItem::MAXIMUM_AZIMUTH = SCRIPT_MAXIMUM_PI;
const float ParticleEffectEntityItem::DEFAULT_AZIMUTH_START = -PI;  // Emit full circumference (when polarFinish > 0)
const float ParticleEffectEntityItem::DEFAULT_AZIMUTH_FINISH = PI;  // ""
const glm::vec3 ParticleEffectEntityItem::DEFAULT_EMIT_ACCELERATION(0.0f, -9.8f, 0.0f);
const float ParticleEffectEntityItem::MINIMUM_EMIT_ACCELERATION = -100.0f; // ~ 10g
const float ParticleEffectEntityItem::MAXIMUM_EMIT_ACCELERATION = 100.0f;
const glm::vec3 ParticleEffectEntityItem::DEFAULT_ACCELERATION_SPREAD(0.0f, 0.0f, 0.0f);
const float ParticleEffectEntityItem::MINIMUM_ACCELERATION_SPREAD = 0.0f;
const float ParticleEffectEntityItem::MAXIMUM_ACCELERATION_SPREAD = 100.0f;
const float ParticleEffectEntityItem::DEFAULT_PARTICLE_RADIUS = 0.025f;
const float ParticleEffectEntityItem::MINIMUM_PARTICLE_RADIUS = 0.0f;
const float ParticleEffectEntityItem::MAXIMUM_PARTICLE_RADIUS = (float)TREE_SCALE;
const float ParticleEffectEntityItem::DEFAULT_RADIUS_SPREAD = 0.0f;
const float ParticleEffectEntityItem::DEFAULT_RADIUS_START = DEFAULT_PARTICLE_RADIUS;
const float ParticleEffectEntityItem::DEFAULT_RADIUS_FINISH = DEFAULT_PARTICLE_RADIUS;
const QString ParticleEffectEntityItem::DEFAULT_TEXTURES = "";
const bool ParticleEffectEntityItem::DEFAULT_EMITTER_SHOULD_TRAIL = false;


EntityItemPointer ParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity { new ParticleEffectEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

// our non-pure virtual subclass for now...
ParticleEffectEntityItem::ParticleEffectEntityItem(const EntityItemID& entityItemID) :
    EntityItem(entityItemID),
    _previousPosition(getPosition()),
    _lastSimulated(usecTimestampNow())
{
    _type = EntityTypes::ParticleEffect;
    setColor(DEFAULT_COLOR);
}

void ParticleEffectEntityItem::setAlpha(float alpha) {
    if (MINIMUM_ALPHA <= alpha && alpha <= MAXIMUM_ALPHA) {
        _alpha = alpha;
    }
}

void ParticleEffectEntityItem::setAlphaStart(float alphaStart) {
    if (MINIMUM_ALPHA <= alphaStart && alphaStart <= MAXIMUM_ALPHA) {
        _alphaStart = alphaStart;
        _isAlphaStartInitialized = true;
    }
}

void ParticleEffectEntityItem::setAlphaFinish(float alphaFinish) {
    if (MINIMUM_ALPHA <= alphaFinish && alphaFinish <= MAXIMUM_ALPHA) {
        _alphaFinish = alphaFinish;
        _isAlphaFinishInitialized = true;
    }
}

void ParticleEffectEntityItem::setAlphaSpread(float alphaSpread) {
    if (MINIMUM_ALPHA <= alphaSpread && alphaSpread <= MAXIMUM_ALPHA) {
        _alphaSpread = alphaSpread;
    }
}

void ParticleEffectEntityItem::setLifespan(float lifespan) {
    if (MINIMUM_LIFESPAN <= lifespan && lifespan <= MAXIMUM_LIFESPAN) {
        _lifespan = lifespan;
    }
}

void ParticleEffectEntityItem::setEmitRate(float emitRate) {
    if (MINIMUM_EMIT_RATE <= emitRate && emitRate <= MAXIMUM_EMIT_RATE) {
        _emitRate = emitRate;
    }
}

void ParticleEffectEntityItem::setEmitSpeed(float emitSpeed) {
    if (MINIMUM_EMIT_SPEED <= emitSpeed && emitSpeed <= MAXIMUM_EMIT_SPEED) {
        _emitSpeed = emitSpeed;
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setSpeedSpread(float speedSpread) {
    if (MINIMUM_EMIT_SPEED <= speedSpread && speedSpread <= MAXIMUM_EMIT_SPEED) {
        _speedSpread = speedSpread;
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setEmitOrientation(const glm::quat& emitOrientation) {
    _emitOrientation = glm::normalize(emitOrientation);
    computeAndUpdateDimensions();
}

void ParticleEffectEntityItem::setEmitDimensions(const glm::vec3& emitDimensions) {
    bool updated = false;
    if (MINIMUM_EMIT_DIMENSION <= emitDimensions.x && emitDimensions.x <= MAXIMUM_EMIT_DIMENSION) {
        _emitDimensions.x = emitDimensions.x;
        updated = true;
    }
    if (MINIMUM_EMIT_DIMENSION <= emitDimensions.y && emitDimensions.y <= MAXIMUM_EMIT_DIMENSION) {
        _emitDimensions.y = emitDimensions.y;
        updated = true;
    }
    if (MINIMUM_EMIT_DIMENSION <= emitDimensions.z && emitDimensions.z <= MAXIMUM_EMIT_DIMENSION) {
        _emitDimensions.z = emitDimensions.z;
        updated = true;
    }
    if (updated) {
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setEmitRadiusStart(float emitRadiusStart) {
    if (MINIMUM_EMIT_RADIUS_START <= emitRadiusStart && emitRadiusStart <= MAXIMUM_EMIT_RADIUS_START) {
        _emitRadiusStart = emitRadiusStart;
    }
}

void ParticleEffectEntityItem::setPolarStart(float polarStart) {
    if (MINIMUM_POLAR <= polarStart && polarStart <= MAXIMUM_POLAR) {
        _polarStart = polarStart;
    }
}

void ParticleEffectEntityItem::setPolarFinish(float polarFinish) {
    if (MINIMUM_POLAR <= polarFinish && polarFinish <= MAXIMUM_POLAR) {
        _polarFinish = polarFinish;
    }
}

void ParticleEffectEntityItem::setAzimuthStart(float azimuthStart) {
    if (MINIMUM_AZIMUTH <= azimuthStart && azimuthStart <= MAXIMUM_AZIMUTH) {
        _azimuthStart = azimuthStart;
    }
}

void ParticleEffectEntityItem::setAzimuthFinish(float azimuthFinish) {
    if (MINIMUM_AZIMUTH <= azimuthFinish && azimuthFinish <= MAXIMUM_AZIMUTH) {
        _azimuthFinish = azimuthFinish;
    }
}

void ParticleEffectEntityItem::setEmitAcceleration(const glm::vec3& emitAcceleration) {
    bool updated = false;
    if (MINIMUM_EMIT_ACCELERATION <= emitAcceleration.x && emitAcceleration.x <= MAXIMUM_EMIT_ACCELERATION) {
        _emitAcceleration.x = emitAcceleration.x;
        updated = true;
    }
    if (MINIMUM_EMIT_ACCELERATION <= emitAcceleration.y && emitAcceleration.y <= MAXIMUM_EMIT_ACCELERATION) {
        _emitAcceleration.y = emitAcceleration.y;
        updated = true;
    }
    if (MINIMUM_EMIT_ACCELERATION <= emitAcceleration.z && emitAcceleration.z <= MAXIMUM_EMIT_ACCELERATION) {
        _emitAcceleration.z = emitAcceleration.z;
        updated = true;
    }
    if (updated) {
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setAccelerationSpread(const glm::vec3& accelerationSpread){
    bool updated = false;
    if (MINIMUM_ACCELERATION_SPREAD <= accelerationSpread.x && accelerationSpread.x <= MAXIMUM_ACCELERATION_SPREAD) {
        _accelerationSpread.x = accelerationSpread.x;
        updated = true;
    }
    if (MINIMUM_ACCELERATION_SPREAD <= accelerationSpread.y && accelerationSpread.y <= MAXIMUM_ACCELERATION_SPREAD) {
        _accelerationSpread.y = accelerationSpread.y;
        updated = true;
    }
    if (MINIMUM_ACCELERATION_SPREAD <= accelerationSpread.z && accelerationSpread.z <= MAXIMUM_ACCELERATION_SPREAD) {
        _accelerationSpread.z = accelerationSpread.z;
        updated = true;
    }
    if (updated) {
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setParticleRadius(float particleRadius) {
    if (MINIMUM_PARTICLE_RADIUS <= particleRadius && particleRadius <= MAXIMUM_PARTICLE_RADIUS) {
        _particleRadius = particleRadius;
    }
}

void ParticleEffectEntityItem::setRadiusStart(float radiusStart) {
    if (MINIMUM_PARTICLE_RADIUS <= radiusStart && radiusStart <= MAXIMUM_PARTICLE_RADIUS) {
        _radiusStart = radiusStart;
        _isRadiusStartInitialized = true;
    }
}

void ParticleEffectEntityItem::setRadiusFinish(float radiusFinish) {
    if (MINIMUM_PARTICLE_RADIUS <= radiusFinish && radiusFinish <= MAXIMUM_PARTICLE_RADIUS) {
        _radiusFinish = radiusFinish;
        _isRadiusFinishInitialized = true;
    }
}

void ParticleEffectEntityItem::setRadiusSpread(float radiusSpread) { 
    if (MINIMUM_PARTICLE_RADIUS <= radiusSpread && radiusSpread <= MAXIMUM_PARTICLE_RADIUS) {
        _radiusSpread = radiusSpread;
    }
}


void ParticleEffectEntityItem::computeAndUpdateDimensions() {
    const float time = _lifespan * 1.1f; // add 10% extra time to account for incremental timer accumulation error

    glm::vec3 velocity = _emitSpeed * (_emitOrientation * Vectors::UNIT_Z);
    glm::vec3 velocitySpread = _speedSpread * (_emitOrientation * Vectors::UNIT_Z);

    glm::vec3 maxVelocity = glm::abs(velocity) + velocitySpread;
    glm::vec3 maxAccleration = glm::abs(_acceleration) + _accelerationSpread;
    glm::vec3 maxDistance = 0.5f * _emitDimensions + time * maxVelocity + (0.5f * time * time) * maxAccleration;

    float maxDistanceValue = std::max(maxDistance.x, std::max(maxDistance.y, maxDistance.z));

    //times 2 because dimensions are diameters not radii
    glm::vec3 dims(2.0f * maxDistanceValue);
    EntityItem::setDimensions(dims);
}


EntityItemProperties ParticleEffectEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(alpha, getAlpha);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeType); // FIXME - this doesn't appear to get used
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(maxParticles, getMaxParticles);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lifespan, getLifespan);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(isEmitting, getIsEmitting);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitRate, getEmitRate);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitSpeed, getEmitSpeed);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(speedSpread, getSpeedSpread);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitOrientation, getEmitOrientation);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitDimensions, getEmitDimensions);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitRadiusStart, getEmitRadiusStart);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(polarStart, getPolarStart);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(polarFinish, getPolarFinish);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(azimuthStart, getAzimuthStart);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(azimuthFinish, getAzimuthFinish);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitAcceleration, getEmitAcceleration);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(accelerationSpread, getAccelerationSpread);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(particleRadius, getParticleRadius);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(radiusSpread, getRadiusSpread);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(radiusStart, getRadiusStart);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(radiusFinish, getRadiusFinish);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(colorSpread, getColorSpread);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(colorStart, getColorStart);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(colorFinish, getColorFinish);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(alphaSpread, getAlphaSpread);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(alphaStart, getAlphaStart);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(alphaFinish, getAlphaFinish);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textures, getTextures);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitterShouldTrail, getEmitterShouldTrail);


    return properties;
}

bool ParticleEffectEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(alpha, setAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, setShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(maxParticles, setMaxParticles);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifespan, setLifespan);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(isEmitting, setIsEmitting);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitRate, setEmitRate);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitSpeed, setEmitSpeed);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(speedSpread, setSpeedSpread);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitOrientation, setEmitOrientation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitDimensions, setEmitDimensions);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitRadiusStart, setEmitRadiusStart);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(polarStart, setPolarStart);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(polarFinish, setPolarFinish);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(azimuthStart, setAzimuthStart);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(azimuthFinish, setAzimuthFinish);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitAcceleration, setEmitAcceleration);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(accelerationSpread, setAccelerationSpread);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(particleRadius, setParticleRadius);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(radiusSpread, setRadiusSpread);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(radiusStart, setRadiusStart);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(radiusFinish, setRadiusFinish);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(colorSpread, setColorSpread);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(colorStart, setColorStart);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(colorFinish, setColorFinish);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(alphaSpread, setAlphaSpread);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(alphaStart, setAlphaStart);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(alphaFinish, setAlphaFinish);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textures, setTextures);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitterShouldTrail, setEmitterShouldTrail);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "ParticleEffectEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties.getLastEdited());
    }
    return somethingChanged;
}

int ParticleEffectEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                               ReadBitstreamToTreeParams& args,
                                                               EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                               bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);

    // Because we're using AnimationLoop which will reset the frame index if you change it's running state
    // we want to read these values in the order they appear in the buffer, but call our setters in an
    // order that allows AnimationLoop to preserve the correct frame rate.
    if (args.bitstreamVersion < VERSION_ENTITIES_ANIMATION_PROPERTIES_GROUP) {
        SKIP_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float);
        SKIP_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float);
        SKIP_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool);
        SKIP_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, QString);
    } else {
        READ_ENTITY_PROPERTY(PROP_EMITTING_PARTICLES, bool, setIsEmitting);
    }

    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, setShapeType);
    READ_ENTITY_PROPERTY(PROP_MAX_PARTICLES, quint32, setMaxParticles);
    READ_ENTITY_PROPERTY(PROP_LIFESPAN, float, setLifespan);
    READ_ENTITY_PROPERTY(PROP_EMIT_RATE, float, setEmitRate);
    if (args.bitstreamVersion < VERSION_ENTITIES_PARTICLE_ELLIPSOID_EMITTER) {
        // OLD PROP_EMIT_VELOCITY FAKEOUT
        SKIP_ENTITY_PROPERTY(PROP_EMIT_SPEED, glm::vec3);
    }
    
    if (args.bitstreamVersion >= VERSION_ENTITIES_PARTICLE_MODIFICATIONS) {
        READ_ENTITY_PROPERTY(PROP_EMIT_ACCELERATION, glm::vec3, setEmitAcceleration);
        READ_ENTITY_PROPERTY(PROP_ACCELERATION_SPREAD, glm::vec3, setAccelerationSpread);
        READ_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, float, setParticleRadius);
        READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTextures);
        if (args.bitstreamVersion < VERSION_ENTITIES_PARTICLE_ELLIPSOID_EMITTER) {
            // OLD PROP_VELOCITY_SPREAD FAKEOUT
            SKIP_ENTITY_PROPERTY(PROP_SPEED_SPREAD, glm::vec3);
        }
    } else {
        // OLD PROP_EMIT_ACCELERATION FAKEOUT
        SKIP_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, float);
        // OLD PROP_ACCELERATION_SPREAD FAKEOUT
        SKIP_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, float);
        READ_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, float, setParticleRadius);
        READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTextures);
    }

    if (args.bitstreamVersion >= VERSION_ENTITIES_PARTICLE_RADIUS_PROPERTIES) {
        READ_ENTITY_PROPERTY(PROP_RADIUS_SPREAD, float, setRadiusSpread);
        READ_ENTITY_PROPERTY(PROP_RADIUS_START, float, setRadiusStart);
        READ_ENTITY_PROPERTY(PROP_RADIUS_FINISH, float, setRadiusFinish);
    }

    if (args.bitstreamVersion >= VERSION_ENTITIES_PARTICLE_COLOR_PROPERTIES) {
        READ_ENTITY_PROPERTY(PROP_COLOR_SPREAD, xColor, setColorSpread);
        READ_ENTITY_PROPERTY(PROP_COLOR_START, xColor, setColorStart);
        READ_ENTITY_PROPERTY(PROP_COLOR_FINISH, xColor, setColorFinish);
        READ_ENTITY_PROPERTY(PROP_ALPHA, float, setAlpha);
        READ_ENTITY_PROPERTY(PROP_ALPHA_SPREAD, float, setAlphaSpread);
        READ_ENTITY_PROPERTY(PROP_ALPHA_START, float, setAlphaStart);
        READ_ENTITY_PROPERTY(PROP_ALPHA_FINISH, float, setAlphaFinish);
    }

    if (args.bitstreamVersion >= VERSION_ENTITIES_PARTICLE_ELLIPSOID_EMITTER) {
        READ_ENTITY_PROPERTY(PROP_EMIT_SPEED, float, setEmitSpeed);
        READ_ENTITY_PROPERTY(PROP_SPEED_SPREAD, float, setSpeedSpread);
        READ_ENTITY_PROPERTY(PROP_EMIT_ORIENTATION, glm::quat, setEmitOrientation);
        READ_ENTITY_PROPERTY(PROP_EMIT_DIMENSIONS, glm::vec3, setEmitDimensions);
        READ_ENTITY_PROPERTY(PROP_EMIT_RADIUS_START, float, setEmitRadiusStart);
        READ_ENTITY_PROPERTY(PROP_POLAR_START, float, setPolarStart);
        READ_ENTITY_PROPERTY(PROP_POLAR_FINISH, float, setPolarFinish);
        READ_ENTITY_PROPERTY(PROP_AZIMUTH_START, float, setAzimuthStart);
        READ_ENTITY_PROPERTY(PROP_AZIMUTH_FINISH, float, setAzimuthFinish);
    }

    if (args.bitstreamVersion >= VERSION_ENTITIES_PARTICLES_ADDITIVE_BLENDING) {
        READ_ENTITY_PROPERTY(PROP_EMITTER_SHOULD_TRAIL, bool, setEmitterShouldTrail);
    }

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ParticleEffectEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += PROP_MAX_PARTICLES;
    requestedProperties += PROP_LIFESPAN;
    requestedProperties += PROP_EMITTING_PARTICLES;
    requestedProperties += PROP_EMIT_RATE;
    requestedProperties += PROP_EMIT_ACCELERATION;
    requestedProperties += PROP_ACCELERATION_SPREAD;
    requestedProperties += PROP_PARTICLE_RADIUS;
    requestedProperties += PROP_TEXTURES;
    requestedProperties += PROP_RADIUS_SPREAD;
    requestedProperties += PROP_RADIUS_START;
    requestedProperties += PROP_RADIUS_FINISH;
    requestedProperties += PROP_COLOR_SPREAD;
    requestedProperties += PROP_COLOR_START;
    requestedProperties += PROP_COLOR_FINISH;
    requestedProperties += PROP_ALPHA;
    requestedProperties += PROP_ALPHA_SPREAD;
    requestedProperties += PROP_ALPHA_START;
    requestedProperties += PROP_ALPHA_FINISH;
    requestedProperties += PROP_EMIT_SPEED;
    requestedProperties += PROP_SPEED_SPREAD;
    requestedProperties += PROP_EMIT_ORIENTATION;
    requestedProperties += PROP_EMIT_DIMENSIONS;
    requestedProperties += PROP_EMIT_RADIUS_START;
    requestedProperties += PROP_POLAR_START;
    requestedProperties += PROP_POLAR_FINISH;
    requestedProperties += PROP_AZIMUTH_START;
    requestedProperties += PROP_AZIMUTH_FINISH;
    requestedProperties += PROP_EMITTER_SHOULD_TRAIL;

    return requestedProperties;
}

void ParticleEffectEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                                  EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                                  EntityPropertyFlags& requestedProperties,
                                                  EntityPropertyFlags& propertyFlags,
                                                  EntityPropertyFlags& propertiesDidntFit,
                                                  int& propertyCount,
                                                  OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_EMITTING_PARTICLES, getIsEmitting());
    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeType());
    APPEND_ENTITY_PROPERTY(PROP_MAX_PARTICLES, getMaxParticles());
    APPEND_ENTITY_PROPERTY(PROP_LIFESPAN, getLifespan());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_RATE, getEmitRate());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_ACCELERATION, getEmitAcceleration());
    APPEND_ENTITY_PROPERTY(PROP_ACCELERATION_SPREAD, getAccelerationSpread());
    APPEND_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, getParticleRadius());
    APPEND_ENTITY_PROPERTY(PROP_TEXTURES, getTextures());
    APPEND_ENTITY_PROPERTY(PROP_RADIUS_SPREAD, getRadiusSpread());
    APPEND_ENTITY_PROPERTY(PROP_RADIUS_START, getRadiusStart());
    APPEND_ENTITY_PROPERTY(PROP_RADIUS_FINISH, getRadiusFinish());
    APPEND_ENTITY_PROPERTY(PROP_COLOR_SPREAD, getColorSpread());
    APPEND_ENTITY_PROPERTY(PROP_COLOR_START, getColorStart());
    APPEND_ENTITY_PROPERTY(PROP_COLOR_FINISH, getColorFinish());
    APPEND_ENTITY_PROPERTY(PROP_ALPHA, getAlpha());
    APPEND_ENTITY_PROPERTY(PROP_ALPHA_SPREAD, getAlphaSpread());
    APPEND_ENTITY_PROPERTY(PROP_ALPHA_START, getAlphaStart());
    APPEND_ENTITY_PROPERTY(PROP_ALPHA_FINISH, getAlphaFinish());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_SPEED, getEmitSpeed());
    APPEND_ENTITY_PROPERTY(PROP_SPEED_SPREAD, getSpeedSpread());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_ORIENTATION, getEmitOrientation());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_DIMENSIONS, getEmitDimensions());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_RADIUS_START, getEmitRadiusStart());
    APPEND_ENTITY_PROPERTY(PROP_POLAR_START, getPolarStart());
    APPEND_ENTITY_PROPERTY(PROP_POLAR_FINISH, getPolarFinish());
    APPEND_ENTITY_PROPERTY(PROP_AZIMUTH_START, getAzimuthStart());
    APPEND_ENTITY_PROPERTY(PROP_AZIMUTH_FINISH, getAzimuthFinish());
    APPEND_ENTITY_PROPERTY(PROP_EMITTER_SHOULD_TRAIL, getEmitterShouldTrail());
}

bool ParticleEffectEntityItem::isEmittingParticles() const {
    // keep emitting if there are particles still alive.
    return (getIsEmitting() || !_particles.empty());
}

bool ParticleEffectEntityItem::needsToCallUpdate() const {
    return true;
}

void ParticleEffectEntityItem::update(const quint64& now) {

    // we check for 'now' in the past in case users set their clock backward
    if (now < _lastSimulated) {
        _lastSimulated = now;
        return;
    }

    float deltaTime = (float)(now - _lastSimulated) / (float)USECS_PER_SECOND;
    _lastSimulated = now;

    if (isEmittingParticles()) {
        stepSimulation(deltaTime);
    }

    EntityItem::update(now); // let our base class handle it's updates...
}

void ParticleEffectEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "PA EFFECT EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                  color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "               position:" << debugTreeVector(getPosition());
    qCDebug(entities) << "             dimensions:" << debugTreeVector(getDimensions());
    qCDebug(entities) << "          getLastEdited:" << debugTime(getLastEdited(), now);
}

void ParticleEffectEntityItem::setShapeType(ShapeType type) {
    if (type != _shapeType) {
        _shapeType = type;
        _dirtyFlags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
    }
}

void ParticleEffectEntityItem::integrateParticle(Particle& particle, float deltaTime) {
    glm::vec3 atSquared = (0.5f * deltaTime * deltaTime) * particle.acceleration;
    glm::vec3 at = particle.acceleration * deltaTime;
    particle.position += particle.velocity * deltaTime + atSquared;
    particle.velocity += at;
}

void ParticleEffectEntityItem::stepSimulation(float deltaTime) {
    // update particles between head and tail
    int popCount = 0;
    for (Particle& particle : _particles) {
        particle.lifetime += deltaTime;

        // if particle has died.
        if (particle.lifetime >= _lifespan) {
            // move head forward
            popCount++;
        } else {
            // Otherwise update it
            integrateParticle(particle, deltaTime);
        }
    }
    _particles.erase(_particles.begin(), _particles.begin() + popCount);

    // emit new particles, but only if we are emmitting
    if (getIsEmitting() && _emitRate > 0.0f && _lifespan > 0.0f && _polarStart <= _polarFinish) {

        float timeLeftInFrame = deltaTime;
        while (_timeUntilNextEmit < timeLeftInFrame) {
            // overflow! move head forward by one.
            // because the case of head == tail indicates an empty array, not a full one.
            // This can drop an existing older particle, but this is by design, newer particles are a higher priority.
            if (_particles.size() >= _maxParticles) {
                _particles.pop_front();
            }
            
            // emit a new particle at tail index.
            _particles.push_back(createParticle(glm::mix(_previousPosition, getPosition(),
                (deltaTime - timeLeftInFrame) / deltaTime)));
            auto particle = _particles.back();
            particle.lifetime += timeLeftInFrame;
            
            // Initialize it
            integrateParticle(particle, deltaTime);
            
            // Advance in frame
            timeLeftInFrame -= _timeUntilNextEmit;
            _timeUntilNextEmit = 1.0f / _emitRate;
        }

        _timeUntilNextEmit -= timeLeftInFrame;
    }
    _previousPosition = getPosition();
}

ParticleEffectEntityItem::Particle ParticleEffectEntityItem::createParticle(const glm::vec3& position) {
    Particle particle;

 
    particle.seed = randFloatInRange(-1.0f, 1.0f);
    if (getEmitterShouldTrail()) {
        particle.position = position;
    }
    // Position, velocity, and acceleration
    if (_polarStart == 0.0f && _polarFinish == 0.0f && _emitDimensions.z == 0.0f) {
        // Emit along z-axis from position

        particle.velocity = (_emitSpeed + 0.2f * _speedSpread) * (_emitOrientation * Vectors::UNIT_Z);
        particle.acceleration = _emitAcceleration + randFloatInRange(-1.0f, 1.0f) * _accelerationSpread;
        
    } else {
        // Emit around point or from ellipsoid
        // - Distribute directions evenly around point
        // - Distribute points relatively evenly over ellipsoid surface
        // - Distribute points relatively evenly within ellipsoid volume
        
        float elevationMinZ = sin(PI_OVER_TWO - _polarFinish);
        float elevationMaxZ = sin(PI_OVER_TWO - _polarStart);
      //  float elevation = asin(elevationMinZ + (elevationMaxZ - elevationMinZ) * randFloat());
        float elevation = asin(elevationMinZ + (elevationMaxZ - elevationMinZ) *randFloat());
        
        float azimuth;
        if (_azimuthFinish >= _azimuthStart) {
            azimuth = _azimuthStart + (_azimuthFinish - _azimuthStart) *  randFloat();
        } else {
            azimuth = _azimuthStart + (TWO_PI + _azimuthFinish - _azimuthStart) * randFloat();
        }
        
        glm::vec3 emitDirection;
        
        if (_emitDimensions == Vectors::ZERO) {
            // Point
            emitDirection = glm::quat(glm::vec3(PI_OVER_TWO - elevation, 0.0f, azimuth)) * Vectors::UNIT_Z;
        } else {
            // Ellipsoid
            float radiusScale = 1.0f;
            if (_emitRadiusStart < 1.0f) {
                float emitRadiusStart = glm::max(_emitRadiusStart, EPSILON);  // Avoid math complications at center
                float randRadius =
                emitRadiusStart + randFloatInRange(0.0f, MAXIMUM_EMIT_RADIUS_START - emitRadiusStart);
                radiusScale = 1.0f - std::pow(1.0f - randRadius, 3.0f);
            }
            
            glm::vec3 radii = radiusScale * 0.5f * _emitDimensions;
            float x = radii.x * glm::cos(elevation) * glm::cos(azimuth);
            float y = radii.y * glm::cos(elevation) * glm::sin(azimuth);
            float z = radii.z * glm::sin(elevation);
            glm::vec3 emitPosition = glm::vec3(x, y, z);
            emitDirection = glm::normalize(glm::vec3(
                radii.x > 0.0f ? x / (radii.x * radii.x) : 0.0f,
                radii.y > 0.0f ? y / (radii.y * radii.y) : 0.0f,
                radii.z > 0.0f ? z / (radii.z * radii.z) : 0.0f
            ));
            
            if (getEmitterShouldTrail()) {
                particle.position += _emitOrientation * emitPosition;
            }
            else {
                particle.position = _emitOrientation * emitPosition;
            }
        }
        
        particle.velocity = (_emitSpeed + randFloatInRange(-1.0f, 1.0f) * _speedSpread) * (_emitOrientation * emitDirection);
        particle.acceleration = _emitAcceleration + randFloatInRange(-1.0f, 1.0f) * _accelerationSpread;
    }
    
    return particle;
}

void ParticleEffectEntityItem::setMaxParticles(quint32 maxParticles) {
    if (_maxParticles != maxParticles && MINIMUM_MAX_PARTICLES <= maxParticles && maxParticles <= MAXIMUM_MAX_PARTICLES) {
        _maxParticles = maxParticles;

        // Pop all the overflowing oldest particles
        while (_particles.size() > _maxParticles) {
            _particles.pop_front();
        }

        // effectively clear all particles and start emitting new ones from scratch.
        _timeUntilNextEmit = 0.0f;
    }
}
