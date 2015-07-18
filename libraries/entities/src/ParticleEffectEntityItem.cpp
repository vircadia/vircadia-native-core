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
//  - Add controls for spread (which is currently hard-coded) and varying emission strength (not currently implemented).
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

#include <QDebug>

#include <ByteCountCoding.h>
#include <GeometryUtil.h>

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "EntitiesLogging.h"
#include "ParticleEffectEntityItem.h"

const xColor ParticleEffectEntityItem::DEFAULT_COLOR = { 255, 255, 255 };
const float ParticleEffectEntityItem::DEFAULT_ANIMATION_FRAME_INDEX = 0.0f;
const bool ParticleEffectEntityItem::DEFAULT_ANIMATION_IS_PLAYING = false;
const float ParticleEffectEntityItem::DEFAULT_ANIMATION_FPS = 30.0f;
const quint32 ParticleEffectEntityItem::DEFAULT_MAX_PARTICLES = 1000;
const float ParticleEffectEntityItem::DEFAULT_LIFESPAN = 3.0f;
const float ParticleEffectEntityItem::DEFAULT_EMIT_RATE = 15.0f;
const glm::vec3 ParticleEffectEntityItem::DEFAULT_EMIT_DIRECTION(0.0f, 1.0f, 0.0f);
const float ParticleEffectEntityItem::DEFAULT_EMIT_STRENGTH = 25.0f;
const float ParticleEffectEntityItem::DEFAULT_LOCAL_GRAVITY = -9.8f;
const float ParticleEffectEntityItem::DEFAULT_PARTICLE_RADIUS = 0.025f;
const QString ParticleEffectEntityItem::DEFAULT_TEXTURES = "";


EntityItemPointer ParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return std::make_shared<ParticleEffectEntityItem>(entityID, properties);
}

// our non-pure virtual subclass for now...
ParticleEffectEntityItem::ParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    EntityItem(entityItemID),
    _maxParticles(DEFAULT_MAX_PARTICLES),
    _lifespan(DEFAULT_LIFESPAN),
    _emitRate(DEFAULT_EMIT_RATE),
    _emitDirection(DEFAULT_EMIT_DIRECTION),
    _emitStrength(DEFAULT_EMIT_STRENGTH),
    _localGravity(DEFAULT_LOCAL_GRAVITY),
    _particleRadius(DEFAULT_PARTICLE_RADIUS),
    _lastAnimated(usecTimestampNow()),
    _animationLoop(),
    _animationSettings(),
    _textures(DEFAULT_TEXTURES),
    _texturesChangedFlag(false),
    _shapeType(SHAPE_TYPE_NONE),
    _particleLifetimes(DEFAULT_MAX_PARTICLES, 0.0f),
    _particlePositions(DEFAULT_MAX_PARTICLES, glm::vec3(0.0f, 0.0f, 0.0f)),
    _particleVelocities(DEFAULT_MAX_PARTICLES, glm::vec3(0.0f, 0.0f, 0.0f)),
    _timeUntilNextEmit(0.0f),
    _particleHeadIndex(0),
    _particleTailIndex(0),
    _particleMaxBound(glm::vec3(1.0f, 1.0f, 1.0f)),
    _particleMinBound(glm::vec3(-1.0f, -1.0f, -1.0f)) {

    _type = EntityTypes::ParticleEffect;
    setColor(DEFAULT_COLOR);
    setProperties(properties);
}

ParticleEffectEntityItem::~ParticleEffectEntityItem() {
}

EntityItemProperties ParticleEffectEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationIsPlaying, getAnimationIsPlaying);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationFrameIndex, getAnimationFrameIndex);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationFPS, getAnimationFPS);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(glowLevel, getGlowLevel);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationSettings, getAnimationSettings);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeType);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(maxParticles, getMaxParticles);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lifespan, getLifespan);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitRate, getEmitRate);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitDirection, getEmitDirection);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(emitStrength, getEmitStrength);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(localGravity, getLocalGravity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(particleRadius, getParticleRadius);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textures, getTextures);

    return properties;
}

bool ParticleEffectEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationIsPlaying, setAnimationIsPlaying);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationFrameIndex, setAnimationFrameIndex);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationFPS, setAnimationFPS);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(glowLevel, setGlowLevel);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationSettings, setAnimationSettings);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, updateShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(maxParticles, setMaxParticles);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifespan, setLifespan);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitRate, setEmitRate);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitDirection, setEmitDirection);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(emitStrength, setEmitStrength);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(localGravity, setLocalGravity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(particleRadius, setParticleRadius);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textures, setTextures);

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
                                                               EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);

    // Because we're using AnimationLoop which will reset the frame index if you change it's running state
    // we want to read these values in the order they appear in the buffer, but call our setters in an
    // order that allows AnimationLoop to preserve the correct frame rate.
    float animationFPS = getAnimationFPS();
    float animationFrameIndex = getAnimationFrameIndex();
    bool animationIsPlaying = getAnimationIsPlaying();
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setAnimationFPS);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setAnimationFrameIndex);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setAnimationIsPlaying);

    if (propertyFlags.getHasProperty(PROP_ANIMATION_PLAYING)) {
        if (animationIsPlaying != getAnimationIsPlaying()) {
            setAnimationIsPlaying(animationIsPlaying);
        }
    }
    if (propertyFlags.getHasProperty(PROP_ANIMATION_FPS)) {
        setAnimationFPS(animationFPS);
    }
    if (propertyFlags.getHasProperty(PROP_ANIMATION_FRAME_INDEX)) {
        setAnimationFrameIndex(animationFrameIndex);
    }

    READ_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, QString, setAnimationSettings);
    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, updateShapeType);
    READ_ENTITY_PROPERTY(PROP_MAX_PARTICLES, quint32, setMaxParticles);
    READ_ENTITY_PROPERTY(PROP_LIFESPAN, float, setLifespan);
    READ_ENTITY_PROPERTY(PROP_EMIT_RATE, float, setEmitRate);
    READ_ENTITY_PROPERTY(PROP_EMIT_DIRECTION, glm::vec3, setEmitDirection);
    READ_ENTITY_PROPERTY(PROP_EMIT_STRENGTH, float, setEmitStrength);
    READ_ENTITY_PROPERTY(PROP_LOCAL_GRAVITY, float, setLocalGravity);
    READ_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, float, setParticleRadius);
    READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTextures);

    return bytesRead;
}


// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ParticleEffectEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_ANIMATION_FPS;
    requestedProperties += PROP_ANIMATION_FRAME_INDEX;
    requestedProperties += PROP_ANIMATION_PLAYING;
    requestedProperties += PROP_ANIMATION_SETTINGS;
    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += PROP_MAX_PARTICLES;
    requestedProperties += PROP_LIFESPAN;
    requestedProperties += PROP_EMIT_RATE;
    requestedProperties += PROP_EMIT_DIRECTION;
    requestedProperties += PROP_EMIT_STRENGTH;
    requestedProperties += PROP_LOCAL_GRAVITY;
    requestedProperties += PROP_PARTICLE_RADIUS;
    requestedProperties += PROP_TEXTURES;

    return requestedProperties;
}

void ParticleEffectEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                                  EntityTreeElementExtraEncodeData* modelTreeElementExtraEncodeData,
                                                  EntityPropertyFlags& requestedProperties,
                                                  EntityPropertyFlags& propertyFlags,
                                                  EntityPropertyFlags& propertiesDidntFit,
                                                  int& propertyCount,
                                                  OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;
    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, getAnimationFPS());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getAnimationFrameIndex());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, getAnimationIsPlaying());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, getAnimationSettings());
    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeType());
    APPEND_ENTITY_PROPERTY(PROP_MAX_PARTICLES, getMaxParticles());
    APPEND_ENTITY_PROPERTY(PROP_LIFESPAN, getLifespan());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_RATE, getEmitRate());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_DIRECTION, getEmitDirection());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_STRENGTH, getEmitStrength());
    APPEND_ENTITY_PROPERTY(PROP_LOCAL_GRAVITY, getLocalGravity());
    APPEND_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, getParticleRadius());
    APPEND_ENTITY_PROPERTY(PROP_TEXTURES, getTextures());
}

bool ParticleEffectEntityItem::isAnimatingSomething() const {
    // keep animating if there are particles still alive.
    return (getAnimationIsPlaying() || getLivingParticleCount() > 0) && getAnimationFPS() != 0.0f;
}

bool ParticleEffectEntityItem::needsToCallUpdate() const {
    return isAnimatingSomething() ? true : EntityItem::needsToCallUpdate();
}

void ParticleEffectEntityItem::update(const quint64& now) {

    float deltaTime = (float)(now - _lastAnimated) / (float)USECS_PER_SECOND;
     _lastAnimated = now;

    // only advance the frame index if we're playing
    if (getAnimationIsPlaying()) {
        _animationLoop.simulate(deltaTime);
    }

    if (isAnimatingSomething()) {
        stepSimulation(deltaTime);

        // update the dimensions
        glm::vec3 dims;
        dims.x = glm::max(glm::abs(_particleMinBound.x), glm::abs(_particleMaxBound.x)) * 2.0f;
        dims.y = glm::max(glm::abs(_particleMinBound.y), glm::abs(_particleMaxBound.y)) * 2.0f;
        dims.z = glm::max(glm::abs(_particleMinBound.z), glm::abs(_particleMaxBound.z)) * 2.0f;
        setDimensions(dims);
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

void ParticleEffectEntityItem::updateShapeType(ShapeType type) {
    if (type != _shapeType) {
        _shapeType = type;
        _dirtyFlags |= EntityItem::DIRTY_SHAPE | EntityItem::DIRTY_MASS;
    }
}

void ParticleEffectEntityItem::setAnimationFrameIndex(float value) {
#ifdef WANT_DEBUG
    if (isAnimatingSomething()) {
        qCDebug(entities) << "ParticleEffectEntityItem::setAnimationFrameIndex()";
        qCDebug(entities) << "    value:" << value;
        qCDebug(entities) << "    was:" << _animationLoop.getFrameIndex();
    }
#endif
    _animationLoop.setFrameIndex(value);
}

void ParticleEffectEntityItem::setAnimationSettings(const QString& value) {
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, frameIndex, or running, those values will be parsed out and
    // will over ride the regular animation settings

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();
    if (settingsMap.contains("fps")) {
        float fps = settingsMap["fps"].toFloat();
        setAnimationFPS(fps);
    }

    if (settingsMap.contains("frameIndex")) {
        float frameIndex = settingsMap["frameIndex"].toFloat();
#ifdef WANT_DEBUG
        if (isAnimatingSomething()) {
            qCDebug(entities) << "ParticleEffectEntityItem::setAnimationSettings() calling setAnimationFrameIndex()...";
            qCDebug(entities) << "    settings:" << value;
            qCDebug(entities) << "    settingsMap[frameIndex]:" << settingsMap["frameIndex"];
            qCDebug(entities"    frameIndex: %20.5f", frameIndex);
        }
#endif

        setAnimationFrameIndex(frameIndex);
    }

    if (settingsMap.contains("running")) {
        bool running = settingsMap["running"].toBool();
        if (running != getAnimationIsPlaying()) {
            setAnimationIsPlaying(running);
        }
    }

    if (settingsMap.contains("firstFrame")) {
        float firstFrame = settingsMap["firstFrame"].toFloat();
        setAnimationFirstFrame(firstFrame);
    }

    if (settingsMap.contains("lastFrame")) {
        float lastFrame = settingsMap["lastFrame"].toFloat();
        setAnimationLastFrame(lastFrame);
    }

    if (settingsMap.contains("loop")) {
        bool loop = settingsMap["loop"].toBool();
        setAnimationLoop(loop);
    }

    if (settingsMap.contains("hold")) {
        bool hold = settingsMap["hold"].toBool();
        setAnimationHold(hold);
    }

    if (settingsMap.contains("startAutomatically")) {
        bool startAutomatically = settingsMap["startAutomatically"].toBool();
        setAnimationStartAutomatically(startAutomatically);
    }

    _animationSettings = value;
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
}

void ParticleEffectEntityItem::setAnimationIsPlaying(bool value) {
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationLoop.setRunning(value);
}

void ParticleEffectEntityItem::setAnimationFPS(float value) {
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationLoop.setFPS(value);
}

QString ParticleEffectEntityItem::getAnimationSettings() const {
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, frameIndex, or running, those values will be parsed out and
    // will over ride the regular animation settings
    QString value = _animationSettings;

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();

    QVariant fpsValue(getAnimationFPS());
    settingsMap["fps"] = fpsValue;

    QVariant frameIndexValue(getAnimationFrameIndex());
    settingsMap["frameIndex"] = frameIndexValue;

    QVariant runningValue(getAnimationIsPlaying());
    settingsMap["running"] = runningValue;

    QVariant firstFrameValue(getAnimationFirstFrame());
    settingsMap["firstFrame"] = firstFrameValue;

    QVariant lastFrameValue(getAnimationLastFrame());
    settingsMap["lastFrame"] = lastFrameValue;

    QVariant loopValue(getAnimationLoop());
    settingsMap["loop"] = loopValue;

    QVariant holdValue(getAnimationHold());
    settingsMap["hold"] = holdValue;

    QVariant startAutomaticallyValue(getAnimationStartAutomatically());
    settingsMap["startAutomatically"] = startAutomaticallyValue;

    settingsAsJsonObject = QJsonObject::fromVariantMap(settingsMap);
    QJsonDocument newDocument(settingsAsJsonObject);
    QByteArray jsonByteArray = newDocument.toJson(QJsonDocument::Compact);
    QString jsonByteString(jsonByteArray);
    return jsonByteString;
}

void ParticleEffectEntityItem::extendBounds(const glm::vec3& point) {
    _particleMinBound.x = glm::min(_particleMinBound.x, point.x);
    _particleMinBound.y = glm::min(_particleMinBound.y, point.y);
    _particleMinBound.z = glm::min(_particleMinBound.z, point.z);
    _particleMaxBound.x = glm::max(_particleMaxBound.x, point.x);
    _particleMaxBound.y = glm::max(_particleMaxBound.y, point.y);
    _particleMaxBound.z = glm::max(_particleMaxBound.z, point.z);
}

void ParticleEffectEntityItem::integrateParticle(quint32 index, float deltaTime) {
    glm::vec3 atSquared(0.0f, 0.5f * _localGravity * deltaTime * deltaTime, 0.0f);
    glm::vec3 at(0.0f, _localGravity * deltaTime, 0.0f);
    _particlePositions[index] += _particleVelocities[index] * deltaTime + atSquared;
    _particleVelocities[index] += at;
}

void ParticleEffectEntityItem::stepSimulation(float deltaTime) {

    _particleMinBound = glm::vec3(-1.0f, -1.0f, -1.0f);
    _particleMaxBound = glm::vec3(1.0f, 1.0f, 1.0f);

    // update particles between head and tail
    for (quint32 i = _particleHeadIndex; i != _particleTailIndex; i = (i + 1) % _maxParticles) {
        _particleLifetimes[i] -= deltaTime;

        // if particle has died.
        if (_particleLifetimes[i] <= 0.0f) {
            // move head forward
            _particleHeadIndex = (_particleHeadIndex + 1) % _maxParticles;
        }
        else {
            integrateParticle(i, deltaTime);
            extendBounds(_particlePositions[i]);
        }
    }

    // emit new particles, but only if animaiton is playing
    if (getAnimationIsPlaying()) {

        float timeLeftInFrame = deltaTime;
        while (_timeUntilNextEmit < timeLeftInFrame) {

            timeLeftInFrame -= _timeUntilNextEmit;
            _timeUntilNextEmit = 1.0f / _emitRate;

            // emit a new particle at tail index.
            quint32 i = _particleTailIndex;
            _particleLifetimes[i] = _lifespan;

            // jitter the _emitDirection by a random offset
            glm::vec3 randOffset;
            randOffset.x = (randFloat() - 0.5f) * 0.25f * _emitStrength;
            randOffset.y = (randFloat() - 0.5f) * 0.25f * _emitStrength;
            randOffset.z = (randFloat() - 0.5f) * 0.25f * _emitStrength;

            // set initial conditions
            _particlePositions[i] = glm::vec3(0.0f, 0.0f, 0.0f);
            _particleVelocities[i] = _emitDirection * _emitStrength + randOffset;

            integrateParticle(i, timeLeftInFrame);
            extendBounds(_particlePositions[i]);

            _particleTailIndex = (_particleTailIndex + 1) % _maxParticles;

            // overflow! move head forward by one.
            // because the case of head == tail indicates an empty array, not a full one.
            // This can drop an existing older particle, but this is by design, newer particles are a higher priority.
            if (_particleTailIndex == _particleHeadIndex) {
                _particleHeadIndex = (_particleHeadIndex + 1) % _maxParticles;
            }
        }

        _timeUntilNextEmit -= timeLeftInFrame;
    }
}

void ParticleEffectEntityItem::setMaxParticles(quint32 maxParticles) {
    if (_maxParticles != maxParticles) {
        _maxParticles = maxParticles;

        // TODO: try to do something smart here and preserve the state of existing particles.

        // resize vectors
        _particleLifetimes.resize(_maxParticles);
        _particlePositions.resize(_maxParticles);
        _particleVelocities.resize(_maxParticles);

        // effectivly clear all particles and start emitting new ones from scratch.
        _particleHeadIndex = 0;
        _particleTailIndex = 0;
        _timeUntilNextEmit = 0.0f;
    }
}

// because particles are in a ring buffer, this isn't trivial
quint32 ParticleEffectEntityItem::getLivingParticleCount() const {
    if (_particleTailIndex >= _particleHeadIndex) {
        return _particleTailIndex - _particleHeadIndex;
    } else {
        return (_maxParticles - _particleHeadIndex) + _particleTailIndex;
    }
}
