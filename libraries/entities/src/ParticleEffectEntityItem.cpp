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

#include "ParticleEffectEntityItem.h"

#include <glm/gtx/transform.hpp>
#include <QtCore/QJsonDocument>

#include <ByteCountCoding.h>
#include <GeometryUtil.h>
#include <Interpolate.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"
#include "EntityScriptingInterface.h"

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
        (a.radiusStart == b.radiusStart) &&
        (a.radius == b.radius) &&
        (a.spin == b.spin) &&
        (a.rotateWithEntity == b.rotateWithEntity) &&
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
        (radiusStart == glm::clamp(radiusStart, MINIMUM_EMIT_RADIUS_START, MAXIMUM_EMIT_RADIUS_START)) &&
        (radius.gradient.target == glm::clamp(radius.gradient.target, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS)) &&
        (radius.range.start == glm::clamp(radius.range.start, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS)) &&
        (radius.range.finish == glm::clamp(radius.range.finish, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS)) &&
        (radius.gradient.spread == glm::clamp(radius.gradient.spread, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS)) &&
        (spin.gradient.target == glm::clamp(spin.gradient.target, MINIMUM_PARTICLE_SPIN, MAXIMUM_PARTICLE_SPIN)) &&
        (spin.range.start == glm::clamp(spin.range.start, MINIMUM_PARTICLE_SPIN, MAXIMUM_PARTICLE_SPIN)) &&
        (spin.range.finish == glm::clamp(spin.range.finish, MINIMUM_PARTICLE_SPIN, MAXIMUM_PARTICLE_SPIN)) &&
        (spin.gradient.spread == glm::clamp(spin.gradient.spread, MINIMUM_PARTICLE_SPIN, MAXIMUM_PARTICLE_SPIN)) &&
        (lifespan == glm::clamp(lifespan, MINIMUM_LIFESPAN, MAXIMUM_LIFESPAN)) &&
        (maxParticles == glm::clamp(maxParticles, MINIMUM_MAX_PARTICLES, MAXIMUM_MAX_PARTICLES)) &&
        (emission.rate == glm::clamp(emission.rate, MINIMUM_EMIT_RATE, MAXIMUM_EMIT_RATE)) &&
        (emission.speed.target == glm::clamp(emission.speed.target, MINIMUM_EMIT_SPEED, MAXIMUM_EMIT_SPEED)) &&
        (emission.speed.spread == glm::clamp(emission.speed.spread, MINIMUM_EMIT_SPEED, MAXIMUM_EMIT_SPEED)) &&
        (emission.acceleration.target == glm::clamp(emission.acceleration.target, vec3(MINIMUM_EMIT_ACCELERATION), vec3(MAXIMUM_EMIT_ACCELERATION))) &&
        (emission.acceleration.spread == glm::clamp(emission.acceleration.spread, vec3(MINIMUM_ACCELERATION_SPREAD), vec3(MAXIMUM_ACCELERATION_SPREAD)) &&
        (emission.dimensions == glm::clamp(emission.dimensions, vec3(MINIMUM_EMIT_DIMENSION), vec3(MAXIMUM_EMIT_DIMENSION))) &&
        (polar.start == glm::clamp(polar.start, MINIMUM_POLAR, MAXIMUM_POLAR)) &&
        (polar.finish == glm::clamp(polar.finish, MINIMUM_POLAR, MAXIMUM_POLAR)) &&
        (azimuth.start == glm::clamp(azimuth.start, MINIMUM_AZIMUTH, MAXIMUM_AZIMUTH)) &&
        (azimuth.finish == glm::clamp(azimuth.finish, MINIMUM_AZIMUTH, MAXIMUM_AZIMUTH)));
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

// our non-pure virtual subclass for now...
ParticleEffectEntityItem::ParticleEffectEntityItem(const EntityItemID& entityItemID) :
    EntityItem(entityItemID)
{
    _type = EntityTypes::ParticleEffect;
    _visuallyReady = false;
}

void ParticleEffectEntityItem::setAlpha(float alpha) {
    withWriteLock([&] {
        _particleProperties.alpha.gradient.target  = glm::clamp(alpha, MINIMUM_ALPHA, MAXIMUM_ALPHA);
    });
}

void ParticleEffectEntityItem::setAlphaStart(float alphaStart) {
    withWriteLock([&] {
        _particleProperties.alpha.range.start = glm::isnan(alphaStart) ? alphaStart : glm::clamp(alphaStart, MINIMUM_ALPHA, MAXIMUM_ALPHA);
    });
}

void ParticleEffectEntityItem::setAlphaFinish(float alphaFinish) {
    withWriteLock([&] {
        _particleProperties.alpha.range.finish = glm::isnan(alphaFinish) ? alphaFinish : glm::clamp(alphaFinish, MINIMUM_ALPHA, MAXIMUM_ALPHA);
    });
}

void ParticleEffectEntityItem::setAlphaSpread(float alphaSpread) {
    withWriteLock([&] {
        _particleProperties.alpha.gradient.spread = glm::clamp(alphaSpread, MINIMUM_ALPHA, MAXIMUM_ALPHA);
    });
}

void ParticleEffectEntityItem::setLifespan(float lifespan) {
    lifespan = glm::clamp(lifespan, MINIMUM_LIFESPAN, MAXIMUM_LIFESPAN);
    if (lifespan != _particleProperties.lifespan) {
        withWriteLock([&] {
            _particleProperties.lifespan = lifespan;
        });
        computeAndUpdateDimensions();
    }
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
    if (emitOrientation != _particleProperties.emission.orientation) {
        withWriteLock([&] {
            _particleProperties.emission.orientation = emitOrientation;
        });
        computeAndUpdateDimensions();
    }
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
        withWriteLock([&] {
            _particleProperties.emission.acceleration.target = emitAcceleration;
        });
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
    particleRadius = glm::clamp(particleRadius, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS);
    if (particleRadius != _particleProperties.radius.gradient.target) {
        withWriteLock([&] {
            _particleProperties.radius.gradient.target = particleRadius;
        });
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setRadiusStart(float radiusStart) {
    radiusStart = glm::isnan(radiusStart) ? radiusStart : glm::clamp(radiusStart, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS);
    if (radiusStart != _particleProperties.radius.range.start) {
        withWriteLock([&] {
            _particleProperties.radius.range.start = radiusStart;
        });
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setRadiusFinish(float radiusFinish) {
    radiusFinish = glm::isnan(radiusFinish) ? radiusFinish : glm::clamp(radiusFinish, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS);
    if (radiusFinish != _particleProperties.radius.range.finish) {
        withWriteLock([&] {
            _particleProperties.radius.range.finish = radiusFinish;
        });
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setRadiusSpread(float radiusSpread) {
    radiusSpread = glm::clamp(radiusSpread, MINIMUM_PARTICLE_RADIUS, MAXIMUM_PARTICLE_RADIUS);
    if (radiusSpread != _particleProperties.radius.gradient.spread) {
        withWriteLock([&] {
            _particleProperties.radius.gradient.spread = radiusSpread;
        });
        computeAndUpdateDimensions();
    }
}

void ParticleEffectEntityItem::setParticleSpin(float particleSpin) {
    particleSpin = glm::clamp(particleSpin, MINIMUM_PARTICLE_SPIN, MAXIMUM_PARTICLE_SPIN);
    if (particleSpin != _particleProperties.spin.gradient.target) {
        withWriteLock([&] {
            _particleProperties.spin.gradient.target = particleSpin;
        });
    }
}

void ParticleEffectEntityItem::setSpinStart(float spinStart) {
    spinStart =
        glm::isnan(spinStart) ? spinStart : glm::clamp(spinStart, MINIMUM_PARTICLE_SPIN, MAXIMUM_PARTICLE_SPIN);
    if (spinStart != _particleProperties.spin.range.start) {
        withWriteLock([&] {
            _particleProperties.spin.range.start = spinStart;
        });
    }
}

void ParticleEffectEntityItem::setSpinFinish(float spinFinish) {
    spinFinish =
        glm::isnan(spinFinish) ? spinFinish : glm::clamp(spinFinish, MINIMUM_PARTICLE_SPIN, MAXIMUM_PARTICLE_SPIN);
    if (spinFinish != _particleProperties.spin.range.finish) {
        withWriteLock([&] {
            _particleProperties.spin.range.finish = spinFinish;
        });
    }
}

void ParticleEffectEntityItem::setSpinSpread(float spinSpread) {
    spinSpread = glm::clamp(spinSpread, MINIMUM_PARTICLE_SPIN, MAXIMUM_PARTICLE_SPIN);
    if (spinSpread != _particleProperties.spin.gradient.spread) {
        withWriteLock([&] {
            _particleProperties.spin.gradient.spread = spinSpread;
        });
    }
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
    glm::vec3 maxAccleration = glm::abs(particleProperties.emission.acceleration.target) + particleProperties.emission.acceleration.spread;
    float maxRadius = particleProperties.radius.gradient.target;
    if (!glm::isnan(particleProperties.radius.range.start)) {
        maxRadius = glm::max(maxRadius, particleProperties.radius.range.start);
    }
    if (!glm::isnan(particleProperties.radius.range.finish)) {
        maxRadius = glm::max(maxRadius, particleProperties.radius.range.finish);
    }
    glm::vec3 maxDistance = 0.5f * particleProperties.emission.dimensions + time * maxVelocity + (0.5f * time * time) * maxAccleration + vec3(maxRadius + particleProperties.radius.gradient.spread);
    if (isNaN(maxDistance)) {
        qCWarning(entities) << "Bad particle data";
        return;
    }

    float maxDistanceValue = glm::compMax(maxDistance);
    //times 2 because dimensions are diameters not radii
    glm::vec3 dims(2.0f * maxDistanceValue);
    EntityItem::setScaledDimensions(dims);
}


EntityItemProperties ParticleEffectEntityItem::getProperties(const EntityPropertyFlags& desiredProperties, bool allowEmptyDesiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties, allowEmptyDesiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getColor);
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
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(particleSpin, getParticleSpin);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(spinSpread, getSpinSpread);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(spinStart, getSpinStart);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(spinFinish, getSpinFinish);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(rotateWithEntity, getRotateWithEntity);

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
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(particleSpin, setParticleSpin);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(spinSpread, setSpinSpread);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(spinStart, setSpinStart);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(spinFinish, setSpinFinish);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(rotateWithEntity, setRotateWithEntity);

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

void ParticleEffectEntityItem::setColor(const glm::u8vec3& value) {
    withWriteLock([&] {
        _particleProperties.color.gradient.target = value;
    });
}

int ParticleEffectEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                               ReadBitstreamToTreeParams& args,
                                                               EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                               bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, u8vec3Color, setColor);
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

    READ_ENTITY_PROPERTY(PROP_COLOR_SPREAD, u8vec3Color, setColorSpread);
    READ_ENTITY_PROPERTY(PROP_COLOR_START, vec3Color, setColorStart);
    READ_ENTITY_PROPERTY(PROP_COLOR_FINISH, vec3Color, setColorFinish);
    READ_ENTITY_PROPERTY(PROP_ALPHA, float, setAlpha);
    READ_ENTITY_PROPERTY(PROP_ALPHA_SPREAD, float, setAlphaSpread);
    READ_ENTITY_PROPERTY(PROP_ALPHA_START, float, setAlphaStart);
    READ_ENTITY_PROPERTY(PROP_ALPHA_FINISH, float, setAlphaFinish);

    READ_ENTITY_PROPERTY(PROP_EMIT_SPEED, float, setEmitSpeed);
    READ_ENTITY_PROPERTY(PROP_SPEED_SPREAD, float, setSpeedSpread);
    READ_ENTITY_PROPERTY(PROP_EMIT_ORIENTATION, quat, setEmitOrientation);
    READ_ENTITY_PROPERTY(PROP_EMIT_DIMENSIONS, glm::vec3, setEmitDimensions);
    READ_ENTITY_PROPERTY(PROP_EMIT_RADIUS_START, float, setEmitRadiusStart);
    READ_ENTITY_PROPERTY(PROP_POLAR_START, float, setPolarStart);
    READ_ENTITY_PROPERTY(PROP_POLAR_FINISH, float, setPolarFinish);
    READ_ENTITY_PROPERTY(PROP_AZIMUTH_START, float, setAzimuthStart);
    READ_ENTITY_PROPERTY(PROP_AZIMUTH_FINISH, float, setAzimuthFinish);

    READ_ENTITY_PROPERTY(PROP_EMITTER_SHOULD_TRAIL, bool, setEmitterShouldTrail);

    READ_ENTITY_PROPERTY(PROP_PARTICLE_SPIN, float, setParticleSpin);
    READ_ENTITY_PROPERTY(PROP_SPIN_SPREAD, float, setSpinSpread);
    READ_ENTITY_PROPERTY(PROP_SPIN_START, float, setSpinStart);
    READ_ENTITY_PROPERTY(PROP_SPIN_FINISH, float, setSpinFinish);
    READ_ENTITY_PROPERTY(PROP_PARTICLE_ROTATE_WITH_ENTITY, bool, setRotateWithEntity);

    return bytesRead;
}

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
    requestedProperties += PROP_PARTICLE_SPIN;
    requestedProperties += PROP_SPIN_SPREAD;
    requestedProperties += PROP_SPIN_START;
    requestedProperties += PROP_SPIN_FINISH;
    requestedProperties += PROP_PARTICLE_ROTATE_WITH_ENTITY;

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
    APPEND_ENTITY_PROPERTY(PROP_PARTICLE_SPIN, getParticleSpin());
    APPEND_ENTITY_PROPERTY(PROP_SPIN_SPREAD, getSpinSpread());
    APPEND_ENTITY_PROPERTY(PROP_SPIN_START, getSpinStart());
    APPEND_ENTITY_PROPERTY(PROP_SPIN_FINISH, getSpinFinish());
    APPEND_ENTITY_PROPERTY(PROP_PARTICLE_ROTATE_WITH_ENTITY, getRotateWithEntity());
}



void ParticleEffectEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "PA EFFECT EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                  color:" <<
        _particleProperties.color.gradient.target.r << "," <<
        _particleProperties.color.gradient.target.g << "," <<
        _particleProperties.color.gradient.target.b;
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
    withWriteLock([&] {
        _particleProperties.maxParticles = glm::clamp(maxParticles, MINIMUM_MAX_PARTICLES, MAXIMUM_MAX_PARTICLES);
    });
}

void ParticleEffectEntityItem::setTextures(const QString& textures) {
    withWriteLock([&] {
        _particleProperties.textures = textures;
    });
}

void ParticleEffectEntityItem::setColorStart(const vec3& colorStart) {
    withWriteLock([&] {
        _particleProperties.color.range.start = colorStart;
    });
}

void ParticleEffectEntityItem::setColorFinish(const vec3& colorFinish) {
    withWriteLock([&] {
        _particleProperties.color.range.finish = colorFinish;
    });
}

void ParticleEffectEntityItem::setColorSpread(const glm::u8vec3& value) {
    withWriteLock([&] {
        _particleProperties.color.gradient.spread = value;
    });
}

void ParticleEffectEntityItem::setEmitterShouldTrail(bool emitterShouldTrail) {
    withWriteLock([&] {
        _particleProperties.emission.shouldTrail = emitterShouldTrail;
    });
}

void ParticleEffectEntityItem::setRotateWithEntity(bool rotateWithEntity) {
    withWriteLock([&] {
        _particleProperties.rotateWithEntity = rotateWithEntity;
    });
}

particle::Properties ParticleEffectEntityItem::getParticleProperties() const {
    particle::Properties result;  
    withReadLock([&] {
        result = _particleProperties;

        // Special case the properties that get treated differently if they're unintialized
        if (glm::any(glm::isnan(result.color.range.start))) {
            result.color.range.start = getColor();
        }
        if (glm::any(glm::isnan(result.color.range.finish))) {
            result.color.range.finish = getColor();
        }
        if (glm::isnan(result.alpha.range.start)) {
            result.alpha.range.start = getAlpha();
        }
        if (glm::isnan(result.alpha.range.finish)) {
            result.alpha.range.finish = getAlpha();
        }
        if (glm::isnan(result.radius.range.start)) {
            result.radius.range.start = getParticleRadius();
        }
        if (glm::isnan(result.radius.range.finish)) {
            result.radius.range.finish = getParticleRadius();
        }
        if (glm::isnan(result.spin.range.start)) {
            result.spin.range.start = getParticleSpin();
        }
        if (glm::isnan(result.spin.range.finish)) {
            result.spin.range.finish = getParticleSpin();
        }
    });

    if (!result.valid()) {
        qCWarning(entities) << "failed validation";
    }

    return result; 
}
