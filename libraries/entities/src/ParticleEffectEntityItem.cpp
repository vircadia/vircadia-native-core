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


using namespace particle;

template <typename T>
bool operator==(const Range<T>& a, const Range<T>& b) {
    return (a.start == b.start && a.finish == b.finish);
}

template <typename T>
bool operator==(const Gradient<T>& a, const Gradient<T>& b) {
    return (a.target == b.target && a.spread == b.spread);
}

template <typename T>
bool operator==(const RangeGradient<T>& a, const RangeGradient<T>& b) {
    return (a.gradient == b.gradient && a.range == b.range);
}

template <typename T>
bool operator!=(const Range<T>& a, const Range<T>& b) {
    return !(a == b);
}

template <typename T>
bool operator!=(const Gradient<T>& a, const Gradient<T>& b) {
    return !(a == b);
}

template <typename T>
bool operator!=(const RangeGradient<T>& a, const RangeGradient<T>& b) {
    return !(a == b);
}

bool operator==(const EmitProperties& a, const EmitProperties& b) {
    return 
        (a.rate == b.rate) &&
        (a.speed == b.speed) &&
        (a.acceleration == b.acceleration) &&
        (a.orientation == b.orientation) &&
        (a.dimensions == b.dimensions) &&
        (a.shouldTrail == b.shouldTrail);
}

bool operator!=(const EmitProperties& a, const EmitProperties& b) {
    return !(a == b);
}

bool operator==(const Properties& a, const Properties& b) {
    return
        (a.color == b.color) &&
        (a.alpha == b.alpha) &&
        (a.radius == b.radius) &&
        (a.radiusStart == b.radiusStart) &&
        (a.lifespan == b.lifespan) &&
        (a.maxParticles == b.maxParticles) &&
        (a.emission == b.emission) &&
        (a.polar == b.polar) &&
        (a.azimuth == b.azimuth) &&
        (a.textures == b.textures);
}

bool operator!=(const Properties& a, const Properties& b) {
    return !(a == b);
}

bool Properties::valid() const {
    if (glm::any(glm::isnan(emission.orientation))) {
        qCWarning(entities) << "Bad particle data";
        return false;
    }

    return
        (alpha.gradient.target == glm::clamp(alpha.gradient.target, MINIMUM_ALPHA, MAXIMUM_ALPHA)) &&
        (alpha.range.start == glm::clamp(alpha.range.start, MINIMUM_ALPHA, MAXIMUM_ALPHA)) &&
        (alpha.range.finish == glm::clamp(alpha.range.finish, MINIMUM_ALPHA, MAXIMUM_ALPHA)) &&
        (alpha.gradient.spread == glm::clamp(alpha.gradient.spread, MINIMUM_ALPHA, MAXIMUM_ALPHA)) &&
        (lifespan == glm::clamp(lifespan, MINIMUM_LIFESPAN, MAXIMUM_LIFESPAN)) &&
        (emission.rate == glm::clamp(emission.rate, MINIMUM_EMIT_RATE, MAXIMUM_EMIT_RATE)) &&
        (emission.speed.target == glm::clamp(emission.speed.target, MINIMUM_EMIT_SPEED, MAXIMUM_EMIT_SPEED)) &&
        (emission.speed.spread == glm::clamp(emission.speed.spread, MINIMUM_EMIT_SPEED, MAXIMUM_EMIT_SPEED)) &&
        (emission.dimensions == glm::clamp(emission.dimensions, vec3(MINIMUM_EMIT_DIMENSION), vec3(MAXIMUM_EMIT_DIMENSION))) &&
        (radiusStart == glm::clamp(radiusStart, MINIMUM_EMIT_RADIUS_START, MAXIMUM_EMIT_RADIUS_START)) &&
        (polar.start == glm::clamp(polar.start, MINIMUM_POLAR, MAXIMUM_POLAR)) &&
        (polar.finish == glm::clamp(polar.finish, MINIMUM_POLAR, MAXIMUM_POLAR)) &&
        (azimuth.start == glm::clamp(azimuth.start, MINIMUM_AZIMUTH, MAXIMUM_AZIMUTH)) &&
        (azimuth.finish == glm::clamp(azimuth.finish, MINIMUM_AZIMUTH, MAXIMUM_AZIMUTH)) &&
        (emission.acceleration.target == glm::clamp(emission.acceleration.target, vec3(MINIMUM_EMIT_ACCELERATION), vec3(MAXIMUM_EMIT_ACCELERATION))) &&
        (emission.acceleration.spread == glm::clamp(emission.acceleration.spread, vec3(MINIMUM_ACCELERATION_SPREAD), vec3(MAXIMUM_ACCELERATION_SPREAD))) &&
        (radius.gradient.target == glm::clamp(radius.gradient.target, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS)) &&
        (radius.range.start == glm::clamp(radius.range.start, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS)) &&
        (radius.range.finish == glm::clamp(radius.range.finish, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS)) &&
        (radius.gradient.spread == glm::clamp(radius.gradient.spread, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS));
}

bool Properties::emitting() const {
    return emission.rate > 0.0f && lifespan > 0.0f && polar.start <= polar.finish;
    
}

uint64_t Properties::emitIntervalUsecs() const {
    if (emission.rate > 0.0f) {
        return (uint64_t)(USECS_PER_SECOND / emission.rate);
    }
    return 0;
}


EntityItemPointer ParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity(new ParticleEffectEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

#if 0
void ParticleEffectEntityItem::checkValid() {
    bool result;
    withReadLock([&] {
        result = _particleProperties.valid();
    });
    if (!result) {
        qCWarning(entities) << "Invalid particle properties";
    }
}
#endif

// our non-pure virtual subclass for now...
ParticleEffectEntityItem::ParticleEffectEntityItem(const EntityItemID& entityItemID) :
    EntityItem(entityItemID)
{
    _type = EntityTypes::ParticleEffect;
    setColor(DEFAULT_COLOR);
}

void ParticleEffectEntityItem::setAlpha(float alpha) {
    withWriteLock([&] {
        _particleProperties.alpha.gradient.target  = glm::clamp(alpha, MINIMUM_ALPHA, MAXIMUM_ALPHA);
    });
}

void ParticleEffectEntityItem::setAlphaStart(float alphaStart) {
    withWriteLock([&] {
        _particleProperties.alpha.range.start = glm::clamp(alphaStart, MINIMUM_ALPHA, MAXIMUM_ALPHA);
        _isAlphaStartInitialized = true;
    });
}

void ParticleEffectEntityItem::setAlphaFinish(float alphaFinish) {
    withWriteLock([&] {
        _particleProperties.alpha.range.finish = glm::clamp(alphaFinish, MINIMUM_ALPHA, MAXIMUM_ALPHA);
        _isAlphaFinishInitialized = true;
    });
}

void ParticleEffectEntityItem::setAlphaSpread(float alphaSpread) {
    withWriteLock([&] {
        _particleProperties.alpha.gradient.spread = glm::clamp(alphaSpread, MINIMUM_ALPHA, MAXIMUM_ALPHA);
    });
}

void ParticleEffectEntityItem::setLifespan(float lifespan) {
    withWriteLock([&] {
        _particleProperties.lifespan = glm::clamp(lifespan, MINIMUM_LIFESPAN, MAXIMUM_LIFESPAN);
    });
}

void ParticleEffectEntityItem::setEmitRate(float emitRate) {
    withWriteLock([&] {
        _particleProperties.emission.rate = glm::clamp(emitRate, MINIMUM_EMIT_RATE, MAXIMUM_EMIT_RATE);
    });
}

void ParticleEffectEntityItem::setEmitSpeed(float emitSpeed) {
    emitSpeed = glm::clamp(emitSpeed, MINIMUM_EMIT_SPEED, MAXIMUM_EMIT_SPEED);
    if (emitSpeed != _particleProperties.emission.speed.target) {
        withWriteLock([&] { 
            _particleProperties.emission.speed.target = emitSpeed; 
        });
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setSpeedSpread(float speedSpread) {
    speedSpread = glm::clamp(speedSpread, MINIMUM_EMIT_SPEED, MAXIMUM_EMIT_SPEED);
    if (speedSpread != _particleProperties.emission.speed.spread) {
        withWriteLock([&] { 
            _particleProperties.emission.speed.spread = speedSpread;
        });
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setEmitOrientation(const glm::quat& emitOrientation_) {
    auto emitOrientation = glm::normalize(emitOrientation_);
    withWriteLock([&] {
        _particleProperties.emission.orientation = emitOrientation; 
    });
    computeAndUpdateDimensions();
}

void ParticleEffectEntityItem::setEmitDimensions(const glm::vec3& emitDimensions_) {
    auto emitDimensions = glm::clamp(emitDimensions_, vec3(MINIMUM_EMIT_DIMENSION), vec3(MAXIMUM_EMIT_DIMENSION));
    if (emitDimensions != _particleProperties.emission.dimensions) {
        withWriteLock([&] { 
            _particleProperties.emission.dimensions = emitDimensions;
        });
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setEmitRadiusStart(float emitRadiusStart) {
    withWriteLock([&] {
        _particleProperties.radiusStart = glm::clamp(emitRadiusStart, MINIMUM_EMIT_RADIUS_START, MAXIMUM_EMIT_RADIUS_START);
    });
}

void ParticleEffectEntityItem::setPolarStart(float polarStart) {
    withWriteLock([&] {
        _particleProperties.polar.start = glm::clamp(polarStart, MINIMUM_POLAR, MAXIMUM_POLAR);
    });
}

void ParticleEffectEntityItem::setPolarFinish(float polarFinish) {
    withWriteLock([&] {
        _particleProperties.polar.finish = glm::clamp(polarFinish, MINIMUM_POLAR, MAXIMUM_POLAR);
    });
}

void ParticleEffectEntityItem::setAzimuthStart(float azimuthStart) {
    withWriteLock([&] {
        _particleProperties.azimuth.start = glm::clamp(azimuthStart, MINIMUM_AZIMUTH, MAXIMUM_AZIMUTH);
    });
}

void ParticleEffectEntityItem::setAzimuthFinish(float azimuthFinish) {
    withWriteLock([&] {
        _particleProperties.azimuth.finish = glm::clamp(azimuthFinish, MINIMUM_AZIMUTH, MAXIMUM_AZIMUTH);
    });
}

void ParticleEffectEntityItem::setEmitAcceleration(const glm::vec3& emitAcceleration_) {
    auto emitAcceleration = glm::clamp(emitAcceleration_, vec3(MINIMUM_EMIT_ACCELERATION), vec3(MAXIMUM_EMIT_ACCELERATION));
    if (emitAcceleration != _particleProperties.emission.acceleration.target) {
        if (!_particleProperties.valid()) {
            qCWarning(entities) << "Bad particle data";
        }
        withWriteLock([&] {
            _particleProperties.emission.acceleration.target = emitAcceleration;
        });
        if (!_particleProperties.valid()) {
            qCWarning(entities) << "Bad particle data";
        }
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setAccelerationSpread(const glm::vec3& accelerationSpread_){
    auto accelerationSpread = glm::clamp(accelerationSpread_, vec3(MINIMUM_ACCELERATION_SPREAD), vec3(MAXIMUM_ACCELERATION_SPREAD));
    if (accelerationSpread != _particleProperties.emission.acceleration.spread) {
        withWriteLock([&] {
            _particleProperties.emission.acceleration.spread = accelerationSpread;
        });
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setParticleRadius(float particleRadius) {
    withWriteLock([&] {
        _particleProperties.radius.gradient.target = glm::clamp(particleRadius, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS);
    });
}

void ParticleEffectEntityItem::setRadiusStart(float radiusStart) {
    withWriteLock([&] {
        _particleProperties.radius.range.start = glm::clamp(radiusStart, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS);
        _isRadiusStartInitialized = true;
    });
}

void ParticleEffectEntityItem::setRadiusFinish(float radiusFinish) {
    withWriteLock([&] {
        _particleProperties.radius.range.finish = glm::clamp(radiusFinish, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS);
        _isRadiusFinishInitialized = true;
    });
}

void ParticleEffectEntityItem::setRadiusSpread(float radiusSpread) { 
    withWriteLock([&] {
        _particleProperties.radius.gradient.spread = glm::clamp(radiusSpread, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS);
    });
}


void ParticleEffectEntityItem::computeAndUpdateDimensions() {
    particle::Properties particleProperties;
    withReadLock([&] {
        particleProperties = _particleProperties;
    });
    const float time = particleProperties.lifespan * 1.1f; // add 10% extra time to account for incremental timer accumulation error

    glm::vec3 direction = particleProperties.emission.orientation * Vectors::UNIT_Z;
    glm::vec3 velocity = particleProperties.emission.speed.target * direction;
    glm::vec3 velocitySpread = particleProperties.emission.speed.spread * direction;
    glm::vec3 maxVelocity = glm::abs(velocity) + velocitySpread;
    glm::vec3 maxAccleration = glm::abs(_acceleration) + particleProperties.emission.acceleration.spread;
    glm::vec3 maxDistance = 0.5f * particleProperties.emission.dimensions + time * maxVelocity + (0.5f * time * time) * maxAccleration;
    if (isNaN(maxDistance)) {
        qCWarning(entities) << "Bad particle data";
        return;
    }

    float maxDistanceValue = glm::compMax(maxDistance);
    //times 2 because dimensions are diameters not radii
    glm::vec3 dims(2.0f * maxDistanceValue);
    EntityItem::setScaledDimensions(dims);
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

void ParticleEffectEntityItem::setColor(const rgbColor& value) {
    memcpy(_particleColorHack, value, sizeof(rgbColor));
    _particleProperties.color.gradient.target.red = value[RED_INDEX];
    _particleProperties.color.gradient.target.green = value[GREEN_INDEX];
    _particleProperties.color.gradient.target.blue = value[BLUE_INDEX];
}

void ParticleEffectEntityItem::setColor(const xColor& value) {
    _particleProperties.color.gradient.target = value;
    _particleColorHack[RED_INDEX] = value.red;
    _particleColorHack[GREEN_INDEX] = value.green;
    _particleColorHack[BLUE_INDEX] = value.blue;
}

int ParticleEffectEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                               ReadBitstreamToTreeParams& args,
                                                               EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                               bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_EMITTING_PARTICLES, bool, setIsEmitting);
    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, setShapeType);
    READ_ENTITY_PROPERTY(PROP_MAX_PARTICLES, quint32, setMaxParticles);
    READ_ENTITY_PROPERTY(PROP_LIFESPAN, float, setLifespan);
    READ_ENTITY_PROPERTY(PROP_EMIT_RATE, float, setEmitRate);

    READ_ENTITY_PROPERTY(PROP_EMIT_ACCELERATION, glm::vec3, setEmitAcceleration);
    READ_ENTITY_PROPERTY(PROP_ACCELERATION_SPREAD, glm::vec3, setAccelerationSpread);
    READ_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, float, setParticleRadius);
    READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTextures);

    READ_ENTITY_PROPERTY(PROP_RADIUS_SPREAD, float, setRadiusSpread);
    READ_ENTITY_PROPERTY(PROP_RADIUS_START, float, setRadiusStart);
    READ_ENTITY_PROPERTY(PROP_RADIUS_FINISH, float, setRadiusFinish);

    READ_ENTITY_PROPERTY(PROP_COLOR_SPREAD, xColor, setColorSpread);
    READ_ENTITY_PROPERTY(PROP_COLOR_START, xColor, setColorStart);
    READ_ENTITY_PROPERTY(PROP_COLOR_FINISH, xColor, setColorFinish);
    READ_ENTITY_PROPERTY(PROP_ALPHA, float, setAlpha);
    READ_ENTITY_PROPERTY(PROP_ALPHA_SPREAD, float, setAlphaSpread);
    READ_ENTITY_PROPERTY(PROP_ALPHA_START, float, setAlphaStart);
    READ_ENTITY_PROPERTY(PROP_ALPHA_FINISH, float, setAlphaFinish);

    READ_ENTITY_PROPERTY(PROP_EMIT_SPEED, float, setEmitSpeed);
    READ_ENTITY_PROPERTY(PROP_SPEED_SPREAD, float, setSpeedSpread);
    READ_ENTITY_PROPERTY(PROP_EMIT_ORIENTATION, glm::quat, setEmitOrientation);
    READ_ENTITY_PROPERTY(PROP_EMIT_DIMENSIONS, glm::vec3, setEmitDimensions);
    READ_ENTITY_PROPERTY(PROP_EMIT_RADIUS_START, float, setEmitRadiusStart);
    READ_ENTITY_PROPERTY(PROP_POLAR_START, float, setPolarStart);
    READ_ENTITY_PROPERTY(PROP_POLAR_FINISH, float, setPolarFinish);
    READ_ENTITY_PROPERTY(PROP_AZIMUTH_START, float, setAzimuthStart);
    READ_ENTITY_PROPERTY(PROP_AZIMUTH_FINISH, float, setAzimuthFinish);

    READ_ENTITY_PROPERTY(PROP_EMITTER_SHOULD_TRAIL, bool, setEmitterShouldTrail);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
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



void ParticleEffectEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "PA EFFECT EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                  color:" << 
        _particleProperties.color.gradient.target.red << "," << 
        _particleProperties.color.gradient.target.green << "," << 
        _particleProperties.color.gradient.target.blue;
    qCDebug(entities) << "               position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "             dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "          getLastEdited:" << debugTime(getLastEdited(), now);
}

void ParticleEffectEntityItem::setShapeType(ShapeType type) {
    withWriteLock([&] {
        if (type != _shapeType) {
            _shapeType = type;
            _flags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
        }
    });
}

void ParticleEffectEntityItem::setMaxParticles(quint32 maxParticles) {
    _particleProperties.maxParticles = glm::clamp(maxParticles, MINIMUM_MAX_PARTICLES, MAXIMUM_MAX_PARTICLES);
}

QString ParticleEffectEntityItem::getTextures() const { 
    QString result;
    withReadLock([&] {
        result = _particleProperties.textures;
    });
    return result;
}

void ParticleEffectEntityItem::setTextures(const QString& textures) {
    withWriteLock([&] {
        _particleProperties.textures = textures;
    });
}

particle::Properties ParticleEffectEntityItem::getParticleProperties() const {
    particle::Properties result;  
    withReadLock([&] { 
        result = _particleProperties; 
    });  

    // Special case the properties that get treated differently if they're unintialized
    result.color.range.start = getColorStart();
    result.color.range.finish = getColorFinish();
    result.alpha.range.start = getAlphaStart();
    result.alpha.range.finish = getAlphaFinish();
    result.radius.range.start = getRadiusStart();
    result.radius.range.finish = getRadiusFinish();

    if (!result.valid()) {
        qCWarning(entities) << "failed validation";
    }

    return result; 
}


