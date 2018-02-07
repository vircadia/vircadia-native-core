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
    EntityItemPointer entity(new ModelEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

ModelEntityItem::ModelEntityItem(const EntityItemID& entityItemID) : EntityItem(entityItemID)
{
    _lastAnimated = usecTimestampNow();
    // set the last animated when interface (re)starts
    _type = EntityTypes::Model;
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
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(relayParentJoints, getRelayParentJoints);
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
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(relayParentJoints, setRelayParentJoints);

    bool somethingChangedInAnimations = _animationProperties.setProperties(properties);

    if (somethingChangedInAnimations) {
        _flags |= Simulation::DIRTY_UPDATEABLE;
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
    READ_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
    READ_ENTITY_PROPERTY(PROP_TEXTURES, QString, setTextures);
    READ_ENTITY_PROPERTY(PROP_RELAY_PARENT_JOINTS, bool, setRelayParentJoints);

    int bytesFromAnimation;
    withWriteLock([&] {
        // Note: since we've associated our _animationProperties with our _animationLoop, the readEntitySubclassDataFromBuffer()
        // will automatically read into the animation loop
        bytesFromAnimation = _animationProperties.readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args,
            propertyFlags, overwriteLocalData, animationPropertiesChanged);
    });

    bytesRead += bytesFromAnimation;
    dataAt += bytesFromAnimation;

    READ_ENTITY_PROPERTY(PROP_SHAPE_TYPE, ShapeType, setShapeType);

    if (animationPropertiesChanged) {
        _flags |= Simulation::DIRTY_UPDATEABLE;
        somethingChanged = true;
    }

    READ_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS_SET, QVector<bool>, setJointRotationsSet);
    READ_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS, QVector<glm::quat>, setJointRotations);
    READ_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS_SET, QVector<bool>, setJointTranslationsSet);
    READ_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS, QVector<glm::vec3>, setJointTranslations);

    return bytesRead;
}

// TODO: eventually only include properties changed since the params.nodeData->getLastTimeBagEmpty() time
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
    requestedProperties += PROP_RELAY_PARENT_JOINTS;

    return requestedProperties;
}


void ModelEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount, OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_MODEL_URL, getModelURL());
    APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, getCompoundShapeURL());
    APPEND_ENTITY_PROPERTY(PROP_TEXTURES, getTextures());
    APPEND_ENTITY_PROPERTY(PROP_RELAY_PARENT_JOINTS, getRelayParentJoints());

    withReadLock([&] {
        _animationProperties.appendSubclassData(packetData, params, entityTreeElementExtraEncodeData, requestedProperties,
            propertyFlags, propertiesDidntFit, propertyCount, appendState);
    });

    APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)getShapeType());

    APPEND_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS_SET, getJointRotationsSet());
    APPEND_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS, getJointRotations());
    APPEND_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS_SET, getJointTranslationsSet());
    APPEND_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS, getJointTranslations());
}



// added update function back for property fix
void ModelEntityItem::update(const quint64& now) {

    {
        auto currentAnimationProperties = this->getAnimationProperties();

        if (_previousAnimationProperties != currentAnimationProperties) {
            withWriteLock([&] {
                // if we hit start animation or change the first or last frame then restart the animation
                if ((currentAnimationProperties.getFirstFrame() != _previousAnimationProperties.getFirstFrame()) || 
                    (currentAnimationProperties.getLastFrame() != _previousAnimationProperties.getLastFrame()) || 
                    (currentAnimationProperties.getRunning() && !_previousAnimationProperties.getRunning())) {

                    // when we start interface and the property is are set then the current frame is initialized to -1
                    if (_currentFrame < 0) {
                        // don't reset _lastAnimated here because we need the timestamp from the ModelEntityItem constructor for when the properties were set
                        _currentFrame = currentAnimationProperties.getCurrentFrame();
                        setAnimationCurrentFrame(_currentFrame);
                    } else {
                        _lastAnimated =  usecTimestampNow();
                        _currentFrame = currentAnimationProperties.getFirstFrame();
                        setAnimationCurrentFrame(currentAnimationProperties.getFirstFrame());
                    }
                } else if (!currentAnimationProperties.getRunning() && _previousAnimationProperties.getRunning()) {
                    _currentFrame = currentAnimationProperties.getFirstFrame();
                    setAnimationCurrentFrame(_currentFrame);
                } else if (currentAnimationProperties.getCurrentFrame() != _previousAnimationProperties.getCurrentFrame()) {
                    // don't reset _lastAnimated here because the currentFrame was set with the previous setting of _lastAnimated
                    _currentFrame = currentAnimationProperties.getCurrentFrame();
                }
                
            });
            _previousAnimationProperties = this->getAnimationProperties();

        }
        
        if (isAnimatingSomething()) {
            if (!(getAnimationFirstFrame() < 0) && !(getAnimationFirstFrame() > getAnimationLastFrame())) {
                updateFrameCount();
            }
        }
    }

    EntityItem::update(now);
}

bool ModelEntityItem::needsToCallUpdate() const {

     return true;
}

void ModelEntityItem::updateFrameCount() {

    if (_currentFrame < 0.0f) {
        return;
    }

    if (!_lastAnimated) {
        _lastAnimated = usecTimestampNow();
        return;
    }

    auto now = usecTimestampNow();

    // update the interval since the last animation.
    auto interval = now - _lastAnimated;
    _lastAnimated = now;

    // if fps is negative then increment timestamp and return.
    if (getAnimationFPS() < 0.0f) {
        return;
    }

    int updatedFrameCount = getAnimationLastFrame() - getAnimationFirstFrame() + 1;

    if (!getAnimationHold() && getAnimationIsPlaying()) {
        float deltaTime = (float)interval / (float)USECS_PER_SECOND;
        _currentFrame += (deltaTime * getAnimationFPS());
        if (_currentFrame > getAnimationLastFrame() + 1) {
            if (getAnimationLoop() && getAnimationFirstFrame() != getAnimationLastFrame()) {
                _currentFrame = getAnimationFirstFrame() + (int)(_currentFrame - getAnimationFirstFrame()) % updatedFrameCount;
            } else {
                _currentFrame = getAnimationLastFrame();
            }
        } else if (_currentFrame < getAnimationFirstFrame()) {
            if (getAnimationFirstFrame() < 0) {
                _currentFrame = 0;
            } else {
                _currentFrame = getAnimationFirstFrame();
            }
        }
        // qCDebug(entities)  << "in update frame " << _currentFrame;
        setAnimationCurrentFrame(_currentFrame);
    }
}

void ModelEntityItem::debugDump() const {
    qCDebug(entities) << "ModelEntityItem id:" << getEntityItemID();
    qCDebug(entities) << "    edited ago:" << getEditedAgo();
    qCDebug(entities) << "    position:" << getWorldPosition();
    qCDebug(entities) << "    dimensions:" << getScaledDimensions();
    qCDebug(entities) << "    model URL:" << getModelURL();
    qCDebug(entities) << "    compound shape URL:" << getCompoundShapeURL();
}

void ModelEntityItem::setShapeType(ShapeType type) {
    withWriteLock([&] {
        if (type != _shapeType) {
            if (type == SHAPE_TYPE_STATIC_MESH && _dynamic) {
                // dynamic and STATIC_MESH are incompatible
                // since the shape is being set here we clear the dynamic bit
                _dynamic = false;
                _flags |= Simulation::DIRTY_MOTION_TYPE;
            }
            _shapeType = type;
            _flags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
        }
    });
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
    withWriteLock([&] {
        if (_modelURL != url) {
            _modelURL = url;
            if (_shapeType == SHAPE_TYPE_STATIC_MESH) {
                _flags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
            }
        }
    });
}

void ModelEntityItem::setCompoundShapeURL(const QString& url) {
    withWriteLock([&] {
        if (_compoundShapeURL.get() != url) {
            ShapeType oldType = computeTrueShapeType();
            _compoundShapeURL.set(url);
            if (oldType != computeTrueShapeType()) {
                _flags |= Simulation::DIRTY_SHAPE | Simulation::DIRTY_MASS;
            }
        }
    });
}

void ModelEntityItem::setAnimationURL(const QString& url) {
    _flags |= Simulation::DIRTY_UPDATEABLE;
    withWriteLock([&] {
        _animationProperties.setURL(url);
    });
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
        if (!getAnimationURL().isEmpty()) {
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

    if (settingsMap.contains("allowTranslation")) {
        bool allowTranslation = settingsMap["allowTranslation"].toBool();
        setAnimationAllowTranslation(allowTranslation);
    }
    _flags |= Simulation::DIRTY_UPDATEABLE;
}

void ModelEntityItem::setAnimationIsPlaying(bool value) {
    _flags |= Simulation::DIRTY_UPDATEABLE;
    _animationProperties.setRunning(value);
}

void ModelEntityItem::setAnimationFPS(float value) {
    _flags |= Simulation::DIRTY_UPDATEABLE;
    _animationProperties.setFPS(value);
}

// virtual
bool ModelEntityItem::shouldBePhysical() const {
    return !isDead() && getShapeType() != SHAPE_TYPE_NONE;
}

void ModelEntityItem::resizeJointArrays(int newSize) {
    if (newSize < 0) {
        return;
    }

    _jointDataLock.withWriteLock([&] {
        if (newSize > _localJointData.size()) {
            _localJointData.resize(newSize);
        }
    });
}

void ModelEntityItem::setAnimationJointsData(const QVector<EntityJointData>& jointsData) {
    resizeJointArrays(jointsData.size());
    _jointDataLock.withWriteLock([&] {
        for (auto index = 0; index < jointsData.size(); ++index) {
            const auto& newJointData = jointsData[index];
            auto& localJointData = _localJointData[index];
            if (newJointData.translationSet) {
                localJointData.joint.translation = newJointData.translation;
                localJointData.translationDirty = true;
            }
            if (newJointData.rotationSet) {
                localJointData.joint.rotation = newJointData.rotation;
                localJointData.rotationDirty = true;
            }
        }
    });
}

void ModelEntityItem::setJointRotations(const QVector<glm::quat>& rotations) {
    resizeJointArrays(rotations.size());
    _jointDataLock.withWriteLock([&] {
        _jointRotationsExplicitlySet = rotations.size() > 0;
        for (int index = 0; index < rotations.size(); index++) {
            auto& jointData = _localJointData[index];
            if (jointData.joint.rotationSet) {
                jointData.joint.rotation = rotations[index];
                jointData.rotationDirty = true;
            }
        }
    });
}

void ModelEntityItem::setJointRotationsSet(const QVector<bool>& rotationsSet) {
    resizeJointArrays(rotationsSet.size());
    _jointDataLock.withWriteLock([&] {
        _jointRotationsExplicitlySet = rotationsSet.size() > 0;
        for (int index = 0; index < rotationsSet.size(); index++) {
            _localJointData[index].joint.rotationSet = rotationsSet[index];
        }
    });
}

void ModelEntityItem::setJointTranslations(const QVector<glm::vec3>& translations) {
    resizeJointArrays(translations.size());
    _jointDataLock.withWriteLock([&] {
        _jointTranslationsExplicitlySet = translations.size() > 0;
        for (int index = 0; index < translations.size(); index++) {
            auto& jointData = _localJointData[index];
            if (jointData.joint.translationSet) {
                jointData.joint.translation = translations[index];
                jointData.translationDirty = true;
            }
        }
    });
}

void ModelEntityItem::setJointTranslationsSet(const QVector<bool>& translationsSet) {
    resizeJointArrays(translationsSet.size());
    _jointDataLock.withWriteLock([&] {
        _jointTranslationsExplicitlySet = translationsSet.size() > 0;
        for (int index = 0; index < translationsSet.size(); index++) {
            _localJointData[index].joint.translationSet = translationsSet[index];
        }
    });
}

QVector<glm::quat> ModelEntityItem::getJointRotations() const {
    QVector<glm::quat> result;
    _jointDataLock.withReadLock([&] {
        if (_jointRotationsExplicitlySet) {
            result.resize(_localJointData.size());
            for (auto i = 0; i < _localJointData.size(); ++i) {
                result[i] = _localJointData[i].joint.rotation;
            }
        }
    });
    return result;
}

QVector<bool> ModelEntityItem::getJointRotationsSet() const {
    QVector<bool> result;
    _jointDataLock.withReadLock([&] {
        if (_jointRotationsExplicitlySet) {
            result.resize(_localJointData.size());
            for (auto i = 0; i < _localJointData.size(); ++i) {
                result[i] = _localJointData[i].joint.rotationSet;
            }
        }
    });

    return result;
}

QVector<glm::vec3> ModelEntityItem::getJointTranslations() const {
    QVector<glm::vec3> result;
    _jointDataLock.withReadLock([&] {
        if (_jointTranslationsExplicitlySet) {
            result.resize(_localJointData.size());
            for (auto i = 0; i < _localJointData.size(); ++i) {
                result[i] = _localJointData[i].joint.translation;
            }
        }
    });
    return result;
}

QVector<bool> ModelEntityItem::getJointTranslationsSet() const {
    QVector<bool> result;
    _jointDataLock.withReadLock([&] {
        if (_jointTranslationsExplicitlySet) {
            result.resize(_localJointData.size());
            for (auto i = 0; i < _localJointData.size(); ++i) {
                result[i] = _localJointData[i].joint.translationSet;
            }
        }
    });
    return result;
}


xColor ModelEntityItem::getXColor() const { 
    xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; 
}
bool ModelEntityItem::hasModel() const { 
    return resultWithReadLock<bool>([&] {
        return !_modelURL.isEmpty();
    });
}
bool ModelEntityItem::hasCompoundShapeURL() const {
    return !_compoundShapeURL.get().isEmpty();
}

QString ModelEntityItem::getModelURL() const {
    return resultWithReadLock<QString>([&] {
        return _modelURL;
    });
}

void ModelEntityItem::setRelayParentJoints(bool relayJoints) {
    withWriteLock([&] {
        _relayParentJoints = relayJoints;
    });
}

bool ModelEntityItem::getRelayParentJoints() const {
    return resultWithReadLock<bool>([&] {
        return _relayParentJoints;
    });
}

QString ModelEntityItem::getCompoundShapeURL() const {
    return _compoundShapeURL.get();
}

void ModelEntityItem::setColor(const rgbColor& value) { 
    withWriteLock([&] {
        memcpy(_color, value, sizeof(_color));
    });
}

void ModelEntityItem::setColor(const xColor& value) {
    withWriteLock([&] {
        _color[RED_INDEX] = value.red;
        _color[GREEN_INDEX] = value.green;
        _color[BLUE_INDEX] = value.blue;
    });
}

// Animation related items...
AnimationPropertyGroup ModelEntityItem::getAnimationProperties() const { 
    AnimationPropertyGroup result;
    withReadLock([&] {
        result = _animationProperties;
    });
    return result; 
}

bool ModelEntityItem::hasAnimation() const { 
    return resultWithReadLock<bool>([&] { 
        return !_animationProperties.getURL().isEmpty();
    });
}

QString ModelEntityItem::getAnimationURL() const { 
    return resultWithReadLock<QString>([&] {
        return _animationProperties.getURL();
    });
}

void ModelEntityItem::setAnimationCurrentFrame(float value) {
    withWriteLock([&] {
        _animationProperties.setCurrentFrame(value);
    });
}

void ModelEntityItem::setAnimationLoop(bool loop) { 
    withWriteLock([&] {
        _animationProperties.setLoop(loop);
    });
}

bool ModelEntityItem::getAnimationLoop() const {
    return resultWithReadLock<bool>([&] {
        return _animationProperties.getLoop();
    });
}


void ModelEntityItem::setAnimationHold(bool hold) { 
    withWriteLock([&] {
        _animationProperties.setHold(hold);
    });
}

bool ModelEntityItem::getAnimationHold() const { 
    return resultWithReadLock<bool>([&] {
        return _animationProperties.getHold();
    });
}

void ModelEntityItem::setAnimationFirstFrame(float firstFrame) { 
    withWriteLock([&] {
        _animationProperties.setFirstFrame(firstFrame);
    });
}

float ModelEntityItem::getAnimationFirstFrame() const { 
    return resultWithReadLock<float>([&] {
        return _animationProperties.getFirstFrame();
    });
}

void ModelEntityItem::setAnimationLastFrame(float lastFrame) { 
    withWriteLock([&] {
        _animationProperties.setLastFrame(lastFrame);
    });
}

float ModelEntityItem::getAnimationLastFrame() const { 
    return resultWithReadLock<float>([&] {
        return _animationProperties.getLastFrame();
    });
}

bool ModelEntityItem::getAnimationIsPlaying() const { 
    return resultWithReadLock<bool>([&] {
        return _animationProperties.getRunning();
    });
}

float ModelEntityItem::getAnimationCurrentFrame() const { 
    return resultWithReadLock<float>([&] {
        return _animationProperties.getCurrentFrame();
    });
}

float ModelEntityItem::getAnimationFPS() const {
    return resultWithReadLock<float>([&] {
        return _animationProperties.getFPS();
    });
}


bool ModelEntityItem::isAnimatingSomething() const {
    return resultWithReadLock<bool>([&] {
        return !_animationProperties.getURL().isEmpty() &&
            _animationProperties.getRunning() &&
            (_animationProperties.getFPS() != 0.0f);
        });
}
