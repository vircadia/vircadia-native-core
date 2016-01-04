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

EntityItemProperties ModelEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(modelURL, getModelURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(compoundShapeURL, getCompoundShapeURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(glowLevel, getGlowLevel);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textures, getTextures);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(shapeType, getShapeType);
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
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(shapeType, updateShapeType);

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

    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, updateShapeType);

    if (animationPropertiesChanged) {
        _dirtyFlags |= Simulation::DIRTY_UPDATEABLE;
        somethingChanged = true;
    }

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
}


QMap<QString, AnimationPointer> ModelEntityItem::_loadedAnimations; // TODO: improve cleanup by leveraging the AnimationPointer(s)

void ModelEntityItem::cleanupLoadedAnimations() {
    foreach(AnimationPointer animation, _loadedAnimations) {
        animation.clear();
    }
    _loadedAnimations.clear();
}

AnimationPointer ModelEntityItem::getAnimation(const QString& url) {
    AnimationPointer animation;

    // if we don't already have this model then create it and initialize it
    if (_loadedAnimations.find(url) == _loadedAnimations.end()) {
        animation = DependencyManager::get<AnimationCache>()->getAnimation(url);
        _loadedAnimations[url] = animation;
    } else {
        animation = _loadedAnimations[url];
    }
    return animation;
}

void ModelEntityItem::mapJoints(const QStringList& modelJointNames) {
    // if we don't have animation, or we're already joint mapped then bail early
    if (!hasAnimation() || jointsMapped()) {
        return;
    }

    AnimationPointer myAnimation = getAnimation(_animationProperties.getURL());
    if (myAnimation && myAnimation->isLoaded()) {
        QStringList animationJointNames = myAnimation->getJointNames();

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

void ModelEntityItem::getAnimationFrame(bool& newFrame,
                                        QVector<glm::quat>& rotationsResult, QVector<glm::vec3>& translationsResult) {
    newFrame = false;

    if (!hasAnimation() || !_jointMappingCompleted) {
        rotationsResult = _lastKnownFrameDataRotations;
        translationsResult = _lastKnownFrameDataTranslations;
    }
    AnimationPointer myAnimation = getAnimation(_animationProperties.getURL()); // FIXME: this could be optimized
    if (myAnimation && myAnimation->isLoaded()) {

        const QVector<FBXAnimationFrame>&  frames = myAnimation->getFramesReference(); // NOTE: getFrames() is too heavy
        auto& fbxJoints = myAnimation->getGeometry().joints;

        int frameCount = frames.size();
        if (frameCount > 0) {
            int animationCurrentFrame = (int)(glm::floor(getAnimationCurrentFrame())) % frameCount;
            if (animationCurrentFrame < 0 || animationCurrentFrame > frameCount) {
                animationCurrentFrame = 0;
            }

            if (animationCurrentFrame != _lastKnownCurrentFrame) {
                _lastKnownCurrentFrame = animationCurrentFrame;
                newFrame = true;

                const QVector<glm::quat>& rotations = frames[animationCurrentFrame].rotations;
                const QVector<glm::vec3>& translations = frames[animationCurrentFrame].translations;

                _lastKnownFrameDataRotations.resize(_jointMapping.size());
                _lastKnownFrameDataTranslations.resize(_jointMapping.size());

                for (int j = 0; j < _jointMapping.size(); j++) {
                    int index = _jointMapping[j];
                    if (index >= 0) {
                        glm::mat4 translationMat;
                        if (index < translations.size()) {
                            translationMat = glm::translate(translations[index]);
                        }
                        glm::mat4 rotationMat(glm::mat4::_null);
                        if (index < rotations.size()) {
                            rotationMat = glm::mat4_cast(fbxJoints[index].preRotation * rotations[index] * fbxJoints[index].postRotation);
                        } else {
                            rotationMat = glm::mat4_cast(fbxJoints[index].preRotation * fbxJoints[index].postRotation);
                        }
                        glm::mat4 finalMat = (translationMat * fbxJoints[index].preTransform *
                                              rotationMat * fbxJoints[index].postTransform);
                        _lastKnownFrameDataTranslations[j] = extractTranslation(finalMat);
                        _lastKnownFrameDataRotations[j] = glmExtractRotation(finalMat);
                    }
                }
            }
        }
    }

    rotationsResult = _lastKnownFrameDataRotations;
    translationsResult = _lastKnownFrameDataTranslations;
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

void ModelEntityItem::updateShapeType(ShapeType type) {
    // BEGIN_TEMPORARY_WORKAROUND
    // we have allowed inconsistent ShapeType's to be stored in SVO files in the past (this was a bug)
    // but we are now enforcing the entity properties to be consistent.  To make the possible we're
    // introducing a temporary workaround: we will ignore ShapeType updates that conflict with the
    // _compoundShapeURL.
    if (hasCompoundShapeURL()) {
        type = SHAPE_TYPE_COMPOUND;
    }
    // END_TEMPORARY_WORKAROUND

    if (type != _shapeType) {
        _shapeType = type;
        _dirtyFlags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
    }
}

// virtual
ShapeType ModelEntityItem::getShapeType() const {
    if (_shapeType == SHAPE_TYPE_COMPOUND) {
        return hasCompoundShapeURL() ? SHAPE_TYPE_COMPOUND : SHAPE_TYPE_NONE;
    } else {
        return _shapeType;
    }
}

void ModelEntityItem::setCompoundShapeURL(const QString& url) {
    if (_compoundShapeURL != url) {
        _compoundShapeURL = url;
        _dirtyFlags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
        _shapeType = _compoundShapeURL.isEmpty() ? SHAPE_TYPE_NONE : SHAPE_TYPE_COMPOUND;
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
    return getShapeType() != SHAPE_TYPE_NONE;
}
