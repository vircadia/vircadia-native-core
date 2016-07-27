//
//  ModelEntityItem.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QJsonDocument>

#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <glm/gtx/transform.hpp>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "ResourceCache.h"
#include "ModelEntityItem.h"

const QString ModelEntityItem::DEFAULT_MODEL_URL = QString("");
const QString ModelEntityItem::DEFAULT_COMPOUND_SHAPE_URL = QString("");

EntityItemPointer ModelEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity { new ModelEntityItem(entityID) };
    entity->setProperties(properties);
    return entity;
}

ModelEntityItem::ModelEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID)
{
    _animationProperties.associateWithAnimationLoop(&_animationLoop);
    _animationLoop.setResetOnRunning(false);

    _type = EntityTypes::Model;
    _jointMappingCompleted = false;
    _lastKnownCurrentFrame = -1;
    _color[0] = _color[1] = _color[2] = 0;
}

const QString ModelEntityItem::getTextures() const {
    QReadLocker locker(&_texturesLock);
    auto textures = _textures;
    return textures;
}

void ModelEntityItem::setTextures(const QString& textures) {
    QWriteLocker locker(&_texturesLock);
    _textures = textures;
}

EntityItemProperties ModelEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(modelURL, getModelURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(compoundShapeURL, getCompoundShapeURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textures, getTextures);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeType);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(jointRotationsSet, getJointRotationsSet);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(jointRotations, getJointRotations);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(jointTranslationsSet, getJointTranslationsSet);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(jointTranslations, getJointTranslations);

    _animationProperties.getProperties(properties);
    return properties;
}

bool ModelEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(modelURL, setModelURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(compoundShapeURL, setCompoundShapeURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textures, setTextures);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, setShapeType);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(jointRotationsSet, setJointRotationsSet);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(jointRotations, setJointRotations);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(jointTranslationsSet, setJointTranslationsSet);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(jointTranslations, setJointTranslations);

    bool somethingChangedInAnimations = _animationProperties.setProperties(properties);

    if (somethingChangedInAnimations) {
        _dirtyFlags |= Simulation::DIRTY_UPDATEABLE;
    }
    somethingChanged = somethingChanged || somethingChangedInAnimations;

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "ModelEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }

    return somethingChanged;
}

int ModelEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;
    bool animationPropertiesChanged = false;

    READ_ENTITY_PROPERTY(PROP_COLOR, rgbColor, setColor);
    READ_ENTITY_PROPERTY(PROP_MODEL_URL, QString, setModelURL);
    if (args.bitstreamVersion < VERSION_ENTITIES_HAS_COLLISION_MODEL) {
        setCompoundShapeURL("");
    } else {
        READ_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
    }

    // Because we're using AnimationLoop which will reset the frame index if you change it's running state
    // we want to read these values in the order they appear in the buffer, but call our setters in an
    // order that allows AnimationLoop to preserve the correct frame rate.
    if (args.bitstreamVersion < VERSION_ENTITIES_ANIMATION_PROPERTIES_GROUP) {
        READ_ENTITY_PROPERTY(PROP_ANIMATION_URL, QString, setAnimationURL);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setAnimationFPS);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setAnimationCurrentFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setAnimationIsPlaying);
    }

    READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTextures);

    if (args.bitstreamVersion < VERSION_ENTITIES_ANIMATION_PROPERTIES_GROUP) {
        READ_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, QString, setAnimationSettings);
    } else {
        // Note: since we've associated our _animationProperties with our _animationLoop, the readEntitySubclassDataFromBuffer()
        // will automatically read into the animation loop
        int bytesFromAnimation = _animationProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
                                                                        propertyFlags, overwriteLocalData, animationPropertiesChanged);

        bytesRead += bytesFromAnimation;
        dataAt += bytesFromAnimation;
    }

    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, setShapeType);

    if (animationPropertiesChanged) {
        _dirtyFlags |= Simulation::DIRTY_UPDATEABLE;
        somethingChanged = true;
    }

    READ_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS_SET, QVector<bool>, setJointRotationsSet);
    READ_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS, QVector<glm::quat>, setJointRotations);
    READ_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS_SET, QVector<bool>, setJointTranslationsSet);
    READ_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS, QVector<glm::vec3>, setJointTranslations);

    return bytesRead;
}

// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ModelEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_MODEL_URL;
    requestedProperties += PROP_COMPOUND_SHAPE_URL;
    requestedProperties += PROP_TEXTURES;
    requestedProperties += PROP_SHAPE_TYPE;
    requestedProperties += _animationProperties.getEntityProperties(params);
    requestedProperties += PROP_JOINT_ROTATIONS_SET;
    requestedProperties += PROP_JOINT_ROTATIONS;
    requestedProperties += PROP_JOINT_TRANSLATIONS_SET;
    requestedProperties += PROP_JOINT_TRANSLATIONS;

    return requestedProperties;
}


void ModelEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount, OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_MODEL_URL, getModelURL());
    APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, getCompoundShapeURL());
    APPEND_ENTITY_PROPERTY(PROP_TEXTURES, getTextures());

    _animationProperties.appendSubclassData(packetData, params, entityTreeElementExtraEncodeData, requestedProperties,
        propertyFlags, propertiesDidntFit, propertyCount, appendState);

    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeType());

    APPEND_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS_SET, getJointRotationsSet());
    APPEND_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS, getJointRotations());
    APPEND_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS_SET, getJointTranslationsSet());
    APPEND_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS, getJointTranslations());
}


void ModelEntityItem::mapJoints(const QStringList& modelJointNames) {
    // if we don't have animation, or we're already joint mapped then bail early
    if (!hasAnimation() || jointsMapped()) {
        return;
    }

    if (!_animation || _animation->getURL().toString() != getAnimationURL()) {
        _animation = DependencyManager::get<AnimationCache>()->getAnimation(getAnimationURL());
    }

    if (_animation && _animation->isLoaded()) {
        QStringList animationJointNames = _animation->getJointNames();

        if (modelJointNames.size() > 0 && animationJointNames.size() > 0) {
            _jointMapping.resize(modelJointNames.size());
            for (int i = 0; i < modelJointNames.size(); i++) {
                _jointMapping[i] = animationJointNames.indexOf(modelJointNames[i]);
            }
            _jointMappingCompleted = true;
            _jointMappingURL = _animationProperties.getURL();
        }
    }
}

bool ModelEntityItem::isAnimatingSomething() const {
    return getAnimationIsPlaying() &&
            getAnimationFPS() != 0.0f &&
            !getAnimationURL().isEmpty();
}

bool ModelEntityItem::needsToCallUpdate() const {
    return isAnimatingSomething() || EntityItem::needsToCallUpdate();
}

void ModelEntityItem::update(const quint64& now) {

    // only advance the frame index if we're playing
    if (getAnimationIsPlaying()) {
        _animationLoop.simulateAtTime(now);
    }

    EntityItem::update(now); // let our base class handle it's updates...
}

void ModelEntityItem::debugDump() const {
    qCDebug(entities) << "ModelEntityItem id:" << getEntityItemID();
    qCDebug(entities) << "    edited ago:" << getEditedAgo();
    qCDebug(entities) << "    position:" << getPosition();
    qCDebug(entities) << "    dimensions:" << getDimensions();
    qCDebug(entities) << "    model URL:" << getModelURL();
    qCDebug(entities) << "    compound shape URL:" << getCompoundShapeURL();
}

void ModelEntityItem::setShapeType(ShapeType type) {
    if (type != _shapeType) {
        if (type == SHAPE_TYPE_STATIC_MESH && _dynamic) {
            // dynamic and STATIC_MESH are incompatible
            // since the shape is being set here we clear the dynamic bit
            _dynamic = false;
            _dirtyFlags |= Simulation::DIRTY_MOTION_TYPE;
        }
        _shapeType = type;
        _dirtyFlags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
    }
}

ShapeType ModelEntityItem::getShapeType() const {
    return computeTrueShapeType();
}

ShapeType ModelEntityItem::computeTrueShapeType() const {
    ShapeType type = _shapeType;
    if (type == SHAPE_TYPE_STATIC_MESH && _dynamic) {
        // dynamic is incompatible with STATIC_MESH
        // shouldn't fall in here but just in case --> fall back to COMPOUND
        type = SHAPE_TYPE_COMPOUND;
    }
    if (type == SHAPE_TYPE_COMPOUND && !hasCompoundShapeURL()) {
        // no compoundURL set --> fall back to SIMPLE_COMPOUND
        type = SHAPE_TYPE_SIMPLE_COMPOUND;
    }
    return type;
}

void ModelEntityItem::setModelURL(const QString& url) {
    if (_modelURL != url) {
        _modelURL = url;
        _parsedModelURL = QUrl(url);
        if (_shapeType == SHAPE_TYPE_STATIC_MESH) {
            _dirtyFlags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
        }
    }
}

void ModelEntityItem::setCompoundShapeURL(const QString& url) {
    if (_compoundShapeURL != url) {
        ShapeType oldType = computeTrueShapeType();
        _compoundShapeURL = url;
        if (oldType != computeTrueShapeType()) {
            _dirtyFlags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
        }
    }
}

void ModelEntityItem::setAnimationURL(const QString& url) {
    _dirtyFlags |= Simulation::DIRTY_UPDATEABLE;
    _animationProperties.setURL(url);
}

void ModelEntityItem::setAnimationSettings(const QString& value) {
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, currentFrame, or running, those values will be parsed out and
    // will over ride the regular animation settings

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();
    if (settingsMap.contains("fps")) {
        float fps = settingsMap["fps"].toFloat();
        setAnimationFPS(fps);
    }

    // old settings used frameIndex
    if (settingsMap.contains("frameIndex")) {
        float currentFrame = settingsMap["frameIndex"].toFloat();
#ifdef WANT_DEBUG
        if (isAnimatingSomething()) {
            qCDebug(entities) << "ModelEntityItem::setAnimationSettings() calling setAnimationFrameIndex()...";
            qCDebug(entities) << "    model URL:" << getModelURL();
            qCDebug(entities) << "    animation URL:" << getAnimationURL();
            qCDebug(entities) << "    settings:" << value;
            qCDebug(entities) << "    settingsMap[frameIndex]:" << settingsMap["frameIndex"];
            qCDebug(entities"    currentFrame: %20.5f", currentFrame);
        }
#endif

        setAnimationCurrentFrame(currentFrame);
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

    _dirtyFlags |= Simulation::DIRTY_UPDATEABLE;
}

void ModelEntityItem::setAnimationIsPlaying(bool value) {
    _dirtyFlags |= Simulation::DIRTY_UPDATEABLE;
    _animationLoop.setRunning(value);
}

void ModelEntityItem::setAnimationFPS(float value) {
    _dirtyFlags |= Simulation::DIRTY_UPDATEABLE;
    _animationLoop.setFPS(value);
}

// virtual
bool ModelEntityItem::shouldBePhysical() const {
    return !isDead() && getShapeType() != SHAPE_TYPE_NONE;
}

void ModelEntityItem::resizeJointArrays(int newSize) {
    if (newSize >= 0 && newSize > _absoluteJointRotationsInObjectFrame.size()) {
        _absoluteJointRotationsInObjectFrame.resize(newSize);
        _absoluteJointRotationsInObjectFrameSet.resize(newSize);
        _absoluteJointRotationsInObjectFrameDirty.resize(newSize);
        _absoluteJointTranslationsInObjectFrame.resize(newSize);
        _absoluteJointTranslationsInObjectFrameSet.resize(newSize);
        _absoluteJointTranslationsInObjectFrameDirty.resize(newSize);
    }
}

void ModelEntityItem::setJointRotations(const QVector<glm::quat>& rotations) {
    _jointDataLock.withWriteLock([&] {
        _jointRotationsExplicitlySet = rotations.size() > 0;
        resizeJointArrays(rotations.size());
        for (int index = 0; index < rotations.size(); index++) {
            if (_absoluteJointRotationsInObjectFrameSet[index]) {
                _absoluteJointRotationsInObjectFrame[index] = rotations[index];
                _absoluteJointRotationsInObjectFrameDirty[index] = true;
            }
        }
    });
}

void ModelEntityItem::setJointRotationsSet(const QVector<bool>& rotationsSet) {
    _jointDataLock.withWriteLock([&] {
        _jointRotationsExplicitlySet = rotationsSet.size() > 0;
        resizeJointArrays(rotationsSet.size());
        for (int index = 0; index < rotationsSet.size(); index++) {
            _absoluteJointRotationsInObjectFrameSet[index] = rotationsSet[index];
        }
    });
}

void ModelEntityItem::setJointTranslations(const QVector<glm::vec3>& translations) {
    _jointDataLock.withWriteLock([&] {
        _jointTranslationsExplicitlySet = translations.size() > 0;
        resizeJointArrays(translations.size());
        for (int index = 0; index < translations.size(); index++) {
            if (_absoluteJointTranslationsInObjectFrameSet[index]) {
                _absoluteJointTranslationsInObjectFrame[index] = translations[index];
                _absoluteJointTranslationsInObjectFrameSet[index] = true;
            }
        }
    });
}

void ModelEntityItem::setJointTranslationsSet(const QVector<bool>& translationsSet) {
    _jointDataLock.withWriteLock([&] {
        _jointTranslationsExplicitlySet = translationsSet.size() > 0;
        resizeJointArrays(translationsSet.size());
        for (int index = 0; index < translationsSet.size(); index++) {
            _absoluteJointTranslationsInObjectFrameSet[index] = translationsSet[index];
        }
    });
}

QVector<glm::quat> ModelEntityItem::getJointRotations() const {
    QVector<glm::quat> result;
    _jointDataLock.withReadLock([&] {
        if (_jointRotationsExplicitlySet) {
            result = _absoluteJointRotationsInObjectFrame;
        }
    });
    return result;
}

QVector<bool> ModelEntityItem::getJointRotationsSet() const {
    QVector<bool> result;
    _jointDataLock.withReadLock([&] {
        if (_jointRotationsExplicitlySet) {
            result = _absoluteJointRotationsInObjectFrameSet;
        }
    });

    return result;
}

QVector<glm::vec3> ModelEntityItem::getJointTranslations() const {
    QVector<glm::vec3> result;
    _jointDataLock.withReadLock([&] {
        if (_jointTranslationsExplicitlySet) {
            result = _absoluteJointTranslationsInObjectFrame;
        }
    });
    return result;
}

QVector<bool> ModelEntityItem::getJointTranslationsSet() const {
    QVector<bool> result;
    _jointDataLock.withReadLock([&] {
        if (_jointTranslationsExplicitlySet) {
            result = _absoluteJointTranslationsInObjectFrameSet;
        }
    });
    return result;
}
