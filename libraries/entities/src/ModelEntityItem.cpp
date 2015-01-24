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

#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "ModelEntityItem.h"

const QString ModelEntityItem::DEFAULT_MODEL_URL = QString("");
const QString ModelEntityItem::DEFAULT_ANIMATION_URL = QString("");
const float ModelEntityItem::DEFAULT_ANIMATION_FRAME_INDEX = 0.0f;
const bool ModelEntityItem::DEFAULT_ANIMATION_IS_PLAYING = false;
const float ModelEntityItem::DEFAULT_ANIMATION_FPS = 30.0f;


EntityItem* ModelEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    return new ModelEntityItem(entityID, properties);
}

ModelEntityItem::ModelEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        EntityItem(entityItemID, properties) 
{ 
    _type = EntityTypes::Model;
    setProperties(properties);
    _lastAnimated = usecTimestampNow();
    _jointMappingCompleted = false;
    _color[0] = _color[1] = _color[2] = 0;
}

EntityItemProperties ModelEntityItem::getProperties() const {
    EntityItemProperties properties = EntityItem::getProperties(); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getXColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(modelURL, getModelURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationURL, getAnimationURL);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationIsPlaying, getAnimationIsPlaying);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationFrameIndex, getAnimationFrameIndex);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationFPS, getAnimationFPS);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(glowLevel, getGlowLevel);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(textures, getTextures);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(animationSettings, getAnimationSettings);
    return properties;
}

bool ModelEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(modelURL, setModelURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationURL, setAnimationURL);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationIsPlaying, setAnimationIsPlaying);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationFrameIndex, setAnimationFrameIndex);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationFPS, setAnimationFPS);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(textures, setTextures);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(animationSettings, setAnimationSettings);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qDebug() << "ModelEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    
    return somethingChanged;
}

int ModelEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {
    
    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY_COLOR(PROP_COLOR, _color);
    READ_ENTITY_PROPERTY_STRING(PROP_MODEL_URL, setModelURL);
    READ_ENTITY_PROPERTY_STRING(PROP_ANIMATION_URL, setAnimationURL);

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

    READ_ENTITY_PROPERTY_STRING(PROP_TEXTURES, setTextures);
    READ_ENTITY_PROPERTY_STRING(PROP_ANIMATION_SETTINGS, setAnimationSettings);

    return bytesRead;
}

// TODO: eventually only include properties changed since the params.lastViewFrustumSent time
EntityPropertyFlags ModelEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);

    requestedProperties += PROP_MODEL_URL;
    requestedProperties += PROP_ANIMATION_URL;
    requestedProperties += PROP_ANIMATION_FPS;
    requestedProperties += PROP_ANIMATION_FRAME_INDEX;
    requestedProperties += PROP_ANIMATION_PLAYING;
    requestedProperties += PROP_ANIMATION_SETTINGS;
    requestedProperties += PROP_TEXTURES;
    
    return requestedProperties;
}


void ModelEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount, OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, appendColor, getColor());
    APPEND_ENTITY_PROPERTY(PROP_MODEL_URL, appendValue, getModelURL());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_URL, appendValue, getAnimationURL());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, appendValue, getAnimationFPS());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, appendValue, getAnimationFrameIndex());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, appendValue, getAnimationIsPlaying());
    APPEND_ENTITY_PROPERTY(PROP_TEXTURES, appendValue, getTextures());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, appendValue, getAnimationSettings());
}


QMap<QString, AnimationPointer> ModelEntityItem::_loadedAnimations; // TODO: improve cleanup by leveraging the AnimationPointer(s)

// This class/instance will cleanup the animations once unloaded.
class EntityAnimationsBookkeeper {
public:
    ~EntityAnimationsBookkeeper() {
        ModelEntityItem::cleanupLoadedAnimations();
    }
};

EntityAnimationsBookkeeper modelAnimationsBookkeeperInstance;

void ModelEntityItem::cleanupLoadedAnimations() {
    foreach(AnimationPointer animation, _loadedAnimations) {
        animation.clear();
    }
    _loadedAnimations.clear();
}

Animation* ModelEntityItem::getAnimation(const QString& url) {
    AnimationPointer animation;
    
    // if we don't already have this model then create it and initialize it
    if (_loadedAnimations.find(url) == _loadedAnimations.end()) {
        animation = DependencyManager::get<AnimationCache>()->getAnimation(url);
        _loadedAnimations[url] = animation;
    } else {
        animation = _loadedAnimations[url];
    }
    return animation.data();
}

void ModelEntityItem::mapJoints(const QStringList& modelJointNames) {
    // if we don't have animation, or we're already joint mapped then bail early
    if (!hasAnimation() || _jointMappingCompleted) {
        return;
    }

    Animation* myAnimation = getAnimation(_animationURL);
    
    if (!_jointMappingCompleted) {
        QStringList animationJointNames = myAnimation->getJointNames();

        if (modelJointNames.size() > 0 && animationJointNames.size() > 0) {
            _jointMapping.resize(modelJointNames.size());
            for (int i = 0; i < modelJointNames.size(); i++) {
                _jointMapping[i] = animationJointNames.indexOf(modelJointNames[i]);
            }
            _jointMappingCompleted = true;
        }
    }
}

QVector<glm::quat> ModelEntityItem::getAnimationFrame() {
    QVector<glm::quat> frameData;
    if (hasAnimation() && _jointMappingCompleted) {
        Animation* myAnimation = getAnimation(_animationURL);
        QVector<FBXAnimationFrame> frames = myAnimation->getFrames();
        int frameCount = frames.size();
        if (frameCount > 0) {
            int animationFrameIndex = (int)(glm::floor(getAnimationFrameIndex())) % frameCount;
            if (animationFrameIndex < 0 || animationFrameIndex > frameCount) {
                animationFrameIndex = 0;
            }
            
            QVector<glm::quat> rotations = frames[animationFrameIndex].rotations;

            frameData.resize(_jointMapping.size());
            for (int j = 0; j < _jointMapping.size(); j++) {
                int rotationIndex = _jointMapping[j];
                if (rotationIndex != -1 && rotationIndex < rotations.size()) {
                    frameData[j] = rotations[rotationIndex];
                }
            }
        }
    }
    return frameData;
}

bool ModelEntityItem::isAnimatingSomething() const { 
    return getAnimationIsPlaying() && 
            getAnimationFPS() != 0.0f &&
            !getAnimationURL().isEmpty();
}

bool ModelEntityItem::needsToCallUpdate() const {
    return isAnimatingSomething() ?  true : EntityItem::needsToCallUpdate();
}

void ModelEntityItem::update(const quint64& now) {
    // only advance the frame index if we're playing
    if (getAnimationIsPlaying()) {
        float deltaTime = (float)(now - _lastAnimated) / (float)USECS_PER_SECOND;
        _lastAnimated = now;
        _animationLoop.simulate(deltaTime);
    } else {
        _lastAnimated = now;
    }
    EntityItem::update(now); // let our base class handle it's updates...
}

void ModelEntityItem::debugDump() const {
    qDebug() << "ModelEntityItem id:" << getEntityItemID();
    qDebug() << "    edited ago:" << getEditedAgo();
    qDebug() << "    position:" << getPosition() * (float)TREE_SCALE;
    qDebug() << "    dimensions:" << getDimensions() * (float)TREE_SCALE;
    qDebug() << "    model URL:" << getModelURL();
}

void ModelEntityItem::setAnimationURL(const QString& url) { 
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationURL = url; 
}

void ModelEntityItem::setAnimationFrameIndex(float value) {
#ifdef WANT_DEBUG
    if (isAnimatingSomething()) {
        qDebug() << "ModelEntityItem::setAnimationFrameIndex()";
        qDebug() << "    value:" << value;
        qDebug() << "    was:" << _animationLoop.getFrameIndex();
        qDebug() << "    model URL:" << getModelURL();
        qDebug() << "    animation URL:" << getAnimationURL();
    }
#endif
    _animationLoop.setFrameIndex(value);
}

void ModelEntityItem::setAnimationSettings(const QString& value) { 
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
            qDebug() << "ModelEntityItem::setAnimationSettings() calling setAnimationFrameIndex()...";
            qDebug() << "    model URL:" << getModelURL();
            qDebug() << "    animation URL:" << getAnimationURL();
            qDebug() << "    settings:" << value;
            qDebug() << "    settingsMap[frameIndex]:" << settingsMap["frameIndex"];
            qDebug("    frameIndex: %20.5f", frameIndex);
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

void ModelEntityItem::setAnimationIsPlaying(bool value) {
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationLoop.setRunning(value); 
}

void ModelEntityItem::setAnimationFPS(float value) {
    _dirtyFlags |= EntityItem::DIRTY_UPDATEABLE;
    _animationLoop.setFPS(value); 
}

QString ModelEntityItem::getAnimationSettings() const { 
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
