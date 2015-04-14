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
//  - For simplicity, I'm currently just rendering each particle as a cross of four axis-aligned quads.  Really, we'd
//    want multiple render modes, including (the most important) textured billboards (always facing camera).  Also, these
//    should support animated textures.
//  - There's no synchronization of the simulation across clients at all.  In fact, it's using rand() under the hood, so
//    there's no gaurantee that different clients will see simulations that look anything like the other.
//  - MORE?
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


EntityItem* ParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new ParticleEffectEntityItem(entityID, properties);
}

// our non-pure virtual subclass for now...
ParticleEffectEntityItem::ParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
    EntityItem(entityItemID, properties) {
    _type = EntityTypes::ParticleEffect;
    _maxParticles = DEFAULT_MAX_PARTICLES;
    _lifespan = DEFAULT_LIFESPAN;
    _emitRate = DEFAULT_EMIT_RATE;
    _emitDirection = DEFAULT_EMIT_DIRECTION;
    _emitStrength = DEFAULT_EMIT_STRENGTH;
    _localGravity = DEFAULT_LOCAL_GRAVITY;
    _particleRadius = DEFAULT_PARTICLE_RADIUS;
    setProperties(properties);
    // this is a pretty dumb thing to do, and it should probably be changed to use a more dynamic
    // data structure in the future.  I'm just trying to get some code out the door for now (and it's
    // at least time efficient (though not space efficient).
    // Also, this being a real-time application, it's doubtful we'll ever have millions of particles
    // to keep track of, so this really isn't all that bad.
    _paLife = new float[_maxParticles];
    _paPosition = new float[_maxParticles * XYZ_STRIDE]; // x,y,z
    _paVelocity = new float[_maxParticles * XYZ_STRIDE]; // x,y,z
    _paXmax = _paYmax = _paZmax = 1.0f;
    _paXmin = _paYmin = _paZmin = -1.0f;
    _randSeed = (unsigned int) glm::abs(_lifespan + _emitRate + _localGravity + getPosition().x + getPosition().y + getPosition().z);
    resetSimulation();
    _lastAnimated = usecTimestampNow();
}

ParticleEffectEntityItem::~ParticleEffectEntityItem() {
    delete [] _paLife;
    delete [] _paPosition;
    delete [] _paVelocity;
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

    READ_ENTITY_PROPERTY_COLOR(PROP_COLOR, _color);

    // Because we're using AnimationLoop which will reset the frame index if you change it's running state
    // we want to read these values in the order they appear in the buffer, but call our setters in an
    // order that allows AnimationLoop to preserve the correct frame rate.
    float animationFPS = getAnimationFPS();
    float animationFrameIndex = getAnimationFrameIndex();
    bool animationIsPlaying = getAnimationIsPlaying();
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, animationFPS);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, animationFrameIndex);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, animationIsPlaying);

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

    READ_ENTITY_PROPERTY_STRING(PROP_ANIMATION_SETTINGS, setAnimationSettings);
    READ_ENTITY_PROPERTY_SETTER(PROP_SHAPE_TYPE, ShapeType, updateShapeType);
    READ_ENTITY_PROPERTY(PROP_MAX_PARTICLES, quint32, _maxParticles);
    READ_ENTITY_PROPERTY(PROP_LIFESPAN, float, _lifespan);
    READ_ENTITY_PROPERTY(PROP_EMIT_RATE, float, _emitRate);
    READ_ENTITY_PROPERTY_SETTER(PROP_EMIT_DIRECTION, glm::vec3, setEmitDirection);
    READ_ENTITY_PROPERTY(PROP_EMIT_STRENGTH, float, _emitStrength);
    READ_ENTITY_PROPERTY(PROP_LOCAL_GRAVITY, float, _localGravity);
    READ_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, float, _particleRadius);

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
    APPEND_ENTITY_PROPERTY(PROP_COLOR, appendColor, getColor());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, appendValue, getAnimationFPS());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, appendValue, getAnimationFrameIndex());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, appendValue, getAnimationIsPlaying());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, appendValue, getAnimationSettings());
    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, appendValue, (uint32_t)getShapeType());
    APPEND_ENTITY_PROPERTY(PROP_MAX_PARTICLES, appendValue, getMaxParticles());
    APPEND_ENTITY_PROPERTY(PROP_LIFESPAN, appendValue, getLifespan());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_RATE, appendValue, getEmitRate());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_DIRECTION, appendValue, getEmitDirection());
    APPEND_ENTITY_PROPERTY(PROP_EMIT_STRENGTH, appendValue, getEmitStrength());
    APPEND_ENTITY_PROPERTY(PROP_LOCAL_GRAVITY, appendValue, getLocalGravity());
    APPEND_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, appendValue, getParticleRadius());
}

bool ParticleEffectEntityItem::isAnimatingSomething() const {
    return getAnimationIsPlaying() &&
        getAnimationFPS() != 0.0f;
}

bool ParticleEffectEntityItem::needsToCallUpdate() const {
    return isAnimatingSomething() ? true : EntityItem::needsToCallUpdate();
}

void ParticleEffectEntityItem::update(const quint64& now) {
    // only advance the frame index if we're playing
    if (getAnimationIsPlaying()) {
        float deltaTime = (float)(now - _lastAnimated) / (float)USECS_PER_SECOND;
        _lastAnimated = now;
        float lastFrame = _animationLoop.getFrameIndex();
        _animationLoop.simulate(deltaTime);
        float curFrame = _animationLoop.getFrameIndex();
        if (curFrame > lastFrame) {
            stepSimulation(deltaTime);
        }
        else if (curFrame < lastFrame) {
            // we looped around, so restart the sim and only sim up to the point
            // since the beginning of the frame range.
            resetSimulation();
            stepSimulation((curFrame - _animationLoop.getFirstFrame()) / _animationLoop.getFPS());
        }
    }
    else {
        _lastAnimated = now;
    }

    // update the dimensions
    glm::vec3 dims;
    dims.x = glm::max(glm::abs(_paXmin), glm::abs(_paXmax)) * 2.0f;
    dims.y = glm::max(glm::abs(_paYmin), glm::abs(_paYmax)) * 2.0f;
    dims.z = glm::max(glm::abs(_paZmin), glm::abs(_paZmax)) * 2.0f;
    setDimensions(dims);

    EntityItem::update(now); // let our base class handle it's updates...
}

void ParticleEffectEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "PA EFFECT EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "                  color:" << _color[0] << "," << _color[1] << "," << _color[2];
    qCDebug(entities) << "               position:" << debugTreeVector(_position);
    qCDebug(entities) << "             dimensions:" << debugTreeVector(_dimensions);
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

void ParticleEffectEntityItem::stepSimulation(float deltaTime) {
    _paXmin = _paYmin = _paZmin = -1.0f;
    _paXmax = _paYmax = _paZmax = 1.0f;

    // update particles
    quint32 updateIter = _paHead;
    while (_paLife[updateIter] > 0.0f) {
        _paLife[updateIter] -= deltaTime;
        if (_paLife[updateIter] <= 0.0f) {
            _paLife[updateIter] = -1.0f;
            _paHead = (_paHead + 1) % _maxParticles;
            _paCount--;
        }
        else {
            // DUMB FORWARD EULER just to get it done
            int j = updateIter * XYZ_STRIDE;
            _paPosition[j] += _paVelocity[j] * deltaTime;
            _paPosition[j+1] += _paVelocity[j+1] * deltaTime;
            _paPosition[j+2] += _paVelocity[j+2] * deltaTime;

            _paXmin = glm::min(_paXmin, _paPosition[j]);
            _paYmin = glm::min(_paYmin, _paPosition[j+1]);
            _paZmin = glm::min(_paZmin, _paPosition[j+2]);
            _paXmax = glm::max(_paXmax, _paPosition[j]);
            _paYmax = glm::max(_paYmax, _paPosition[j + 1]);
            _paZmax = glm::max(_paZmax, _paPosition[j + 2]);

            // massless particles
            _paVelocity[j + 1] += deltaTime * _localGravity;
        }
        updateIter = (updateIter + 1) % _maxParticles;
    }

    // emit new particles
    quint32 emitIdx = updateIter;
    _partialEmit += ((float)_emitRate) * deltaTime;
    quint32 birthed = (quint32)_partialEmit;
    _partialEmit -= (float)birthed;
    glm::vec3 randOffset;

    for (quint32 i = 0; i < birthed; i++) {
        if (_paLife[emitIdx] < 0.0f) {
            int j = emitIdx * XYZ_STRIDE;
            _paLife[emitIdx] = _lifespan;
            randOffset.x = (((double)rand() / (double)RAND_MAX) - 0.5) * 0.25 * _emitStrength;
            randOffset.y = (((double)rand() / (double)RAND_MAX) - 0.5) * 0.25 * _emitStrength;
            randOffset.z = (((double)rand() / (double)RAND_MAX) - 0.5) * 0.25 * _emitStrength;
            _paVelocity[j] = (_emitDirection.x * _emitStrength) + randOffset.x;
            _paVelocity[j + 1] = (_emitDirection.y * _emitStrength) + randOffset.y;
            _paVelocity[j + 2] = (_emitDirection.z * _emitStrength) + randOffset.z;

            // DUMB FORWARD EULER just to get it done
            _paPosition[j] += _paVelocity[j] * deltaTime;
            _paPosition[j + 1] += _paVelocity[j + 1] * deltaTime;
            _paPosition[j + 2] += _paVelocity[j + 2] * deltaTime;

            _paXmin = glm::min(_paXmin, _paPosition[j]);
            _paYmin = glm::min(_paYmin, _paPosition[j + 1]);
            _paZmin = glm::min(_paZmin, _paPosition[j + 2]);
            _paXmax = glm::max(_paXmax, _paPosition[j]);
            _paYmax = glm::max(_paYmax, _paPosition[j + 1]);
            _paZmax = glm::max(_paZmax, _paPosition[j + 2]);

            // massless particles
            // and simple gravity down
            _paVelocity[j + 1] += deltaTime * _localGravity;

            emitIdx = (emitIdx + 1) % _maxParticles;
            _paCount++;
        }
        else
            break;
    }
}

void ParticleEffectEntityItem::resetSimulation() {
    for (quint32 i = 0; i < _maxParticles; i++) {
        quint32 j = i * XYZ_STRIDE;
        _paLife[i] = -1.0f;
        _paPosition[j] = 0.0f;
        _paPosition[j+1] = 0.0f;
        _paPosition[j+2] = 0.0f;
        _paVelocity[j] = 0.0f;
        _paVelocity[j+1] = 0.0f;
        _paVelocity[j+2] = 0.0f;
    }
    _paCount = 0;
    _paHead = 0;
    _partialEmit = 0.0f;

    srand(_randSeed);
}

