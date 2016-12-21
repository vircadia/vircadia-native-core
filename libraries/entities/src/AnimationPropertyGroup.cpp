//
//  AnimationPropertyGroup.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QJsonDocument>
#include <OctreePacketData.h>

#include <AnimationLoop.h>

#include "AnimationPropertyGroup.h"
#include "EntityItemProperties.h"
#include "EntityItemPropertiesMacros.h"

void AnimationPropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_URL, Animation, animation, URL, url);

    if (_animationLoop) {
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_FPS, Animation, animation, FPS, fps, _animationLoop->getFPS);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_FRAME_INDEX, Animation, animation, CurrentFrame, currentFrame, _animationLoop->getCurrentFrame);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_PLAYING, Animation, animation, Running, running, _animationLoop->getRunning);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_LOOP, Animation, animation, Loop, loop, _animationLoop->getLoop);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_FIRST_FRAME, Animation, animation, FirstFrame, firstFrame, _animationLoop->getFirstFrame);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_LAST_FRAME, Animation, animation, LastFrame, lastFrame, _animationLoop->getLastFrame);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_HOLD, Animation, animation, Hold, hold, _animationLoop->getHold);
    } else {
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FPS, Animation, animation, FPS, fps);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FRAME_INDEX, Animation, animation, CurrentFrame, currentFrame);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_PLAYING, Animation, animation, Running, running);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_LOOP, Animation, animation, Loop, loop);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FIRST_FRAME, Animation, animation, FirstFrame, firstFrame);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_LAST_FRAME, Animation, animation, LastFrame, lastFrame);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_HOLD, Animation, animation, Hold, hold);
    }
}

void AnimationPropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {

    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, url, QString, setURL);

    // legacy property support
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(animationURL, QString, setURL, getURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_NOCHECK(animationSettings, QString, setFromOldAnimationSettings);

    if (_animationLoop) {
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, fps, float, _animationLoop->setFPS);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, currentFrame, float, _animationLoop->setCurrentFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, running, bool, _animationLoop->setRunning);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, loop, bool, _animationLoop->setLoop);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, firstFrame, float, _animationLoop->setFirstFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, lastFrame, float, _animationLoop->setLastFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, hold, bool, _animationLoop->setHold);

        // legacy property support
        COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(animationFPS, float, _animationLoop->setFPS, _animationLoop->getFPS);
        COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(animationIsPlaying, bool, _animationLoop->setRunning, _animationLoop->getRunning);
        COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(animationFrameIndex, float, _animationLoop->setCurrentFrame, _animationLoop->getCurrentFrame);

    } else {
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, fps, float, setFPS);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, currentFrame, float, setCurrentFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, running, bool, setRunning);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, loop, bool, setLoop);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, firstFrame, float, setFirstFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, lastFrame, float, setLastFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, hold, bool, setHold);

        // legacy property support
        COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(animationFPS, float, setFPS, getFPS);
        COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(animationIsPlaying, bool, setRunning, getRunning);
        COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(animationFrameIndex, float, setCurrentFrame, getCurrentFrame);
    }

}

void AnimationPropertyGroup::merge(const AnimationPropertyGroup& other) {
    COPY_PROPERTY_IF_CHANGED(url);
    if (_animationLoop) {
        _fps = _animationLoop->getFPS();
        _currentFrame = _animationLoop->getCurrentFrame();
        _running = _animationLoop->getRunning();
        _loop = _animationLoop->getLoop();
        _firstFrame = _animationLoop->getFirstFrame();
        _lastFrame = _animationLoop->getLastFrame();
        _hold = _animationLoop->getHold();
    } else {
        COPY_PROPERTY_IF_CHANGED(fps);
        COPY_PROPERTY_IF_CHANGED(currentFrame);
        COPY_PROPERTY_IF_CHANGED(running);
        COPY_PROPERTY_IF_CHANGED(loop);
        COPY_PROPERTY_IF_CHANGED(firstFrame);
        COPY_PROPERTY_IF_CHANGED(lastFrame);
        COPY_PROPERTY_IF_CHANGED(hold);
    }
}

void AnimationPropertyGroup::setFromOldAnimationSettings(const QString& value) {
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, currentFrame, or running, those values will be parsed out and
    // will over ride the regular animation settings

    float fps = _animationLoop ? _animationLoop->getFPS() : getFPS();
    float currentFrame = _animationLoop ? _animationLoop->getCurrentFrame() : getCurrentFrame();
    bool running = _animationLoop ? _animationLoop->getRunning() : getRunning();
    float firstFrame = _animationLoop ? _animationLoop->getFirstFrame() : getFirstFrame();
    float lastFrame = _animationLoop ? _animationLoop->getLastFrame() : getLastFrame();
    bool loop = _animationLoop ? _animationLoop->getLoop() : getLoop();
    bool hold = _animationLoop ? _animationLoop->getHold() : getHold();

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();

    if (settingsMap.contains("fps")) {
        fps = settingsMap["fps"].toFloat();
    }

    // old settings had frameIndex
    if (settingsMap.contains("frameIndex")) {
        currentFrame = settingsMap["frameIndex"].toFloat();
    }

    if (settingsMap.contains("running")) {
        running = settingsMap["running"].toBool();
    }

    if (settingsMap.contains("firstFrame")) {
        firstFrame = settingsMap["firstFrame"].toFloat();
    }

    if (settingsMap.contains("lastFrame")) {
        lastFrame = settingsMap["lastFrame"].toFloat();
    }

    if (settingsMap.contains("loop")) {
        running = settingsMap["loop"].toBool();
    }

    if (settingsMap.contains("hold")) {
        running = settingsMap["hold"].toBool();
    }

    if (_animationLoop) {
        _animationLoop->setFPS(fps);
        _animationLoop->setCurrentFrame(currentFrame);
        _animationLoop->setRunning(running);
        _animationLoop->setFirstFrame(firstFrame);
        _animationLoop->setLastFrame(lastFrame);
        _animationLoop->setLoop(loop);
        _animationLoop->setHold(hold);
    } else {
        setFPS(fps);
        setCurrentFrame(currentFrame);
        setRunning(running);
        setFirstFrame(firstFrame);
        setLastFrame(lastFrame);
        setLoop(loop);
        setHold(hold);
    }
}


void AnimationPropertyGroup::debugDump() const {
    qCDebug(entities) << "   AnimationPropertyGroup: ---------------------------------------------";
    qCDebug(entities) << "       url:" << getURL() << " has changed:" << urlChanged();
    qCDebug(entities) << "       fps:" << getFPS() << " has changed:" << fpsChanged();
    qCDebug(entities) << "currentFrame:" << getCurrentFrame() << " has changed:" << currentFrameChanged();
}

void AnimationPropertyGroup::listChangedProperties(QList<QString>& out) {
    if (urlChanged()) {
        out << "animation-url";
    }
    if (fpsChanged()) {
        out << "animation-fps";
    }
    if (currentFrameChanged()) {
        out << "animation-currentFrame";
    }
}


bool AnimationPropertyGroup::appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_URL, getURL());
    if (_animationLoop) {
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, _animationLoop->getFPS());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, _animationLoop->getCurrentFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, _animationLoop->getRunning());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, _animationLoop->getLoop());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, _animationLoop->getFirstFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, _animationLoop->getLastFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, _animationLoop->getHold());
    } else {
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, getFPS());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getCurrentFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, getRunning());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, getLoop());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, getFirstFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, getLastFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, getHold());
    }

    return true;
}


bool AnimationPropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;
    bool somethingChanged = false;

    READ_ENTITY_PROPERTY(PROP_ANIMATION_URL, QString, setURL);

    if (_animationLoop) {
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, _animationLoop->setFPS);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, _animationLoop->setCurrentFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, _animationLoop->setRunning);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, _animationLoop->setLoop);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, _animationLoop->setFirstFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, _animationLoop->setLastFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, _animationLoop->setHold);
    } else {
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setFPS);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setCurrentFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setRunning);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, setLoop);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, setFirstFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, setLastFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, setHold);
    }

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_URL, URL);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_FPS, FPS);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_FRAME_INDEX, CurrentFrame);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_PLAYING, Running);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_LOOP, Loop);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_FIRST_FRAME, FirstFrame);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_LAST_FRAME, LastFrame);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_HOLD, Hold);

    processedBytes += bytesRead;

    Q_UNUSED(somethingChanged);

    return true;
}

void AnimationPropertyGroup::markAllChanged() {
    _urlChanged = true;
    _fpsChanged = true;
    _currentFrameChanged = true;
    _runningChanged = true;
    _loopChanged = true;
    _firstFrameChanged = true;
    _lastFrameChanged = true;
    _holdChanged = true;
}

EntityPropertyFlags AnimationPropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;

    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_URL, url);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_FPS, fps);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_FRAME_INDEX, currentFrame);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_PLAYING, running);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_LOOP, loop);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_FIRST_FRAME, firstFrame);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_LAST_FRAME, lastFrame);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_HOLD, hold);

    return changedProperties;
}

void AnimationPropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, URL, getURL);
    if (_animationLoop) {
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, FPS, _animationLoop->getFPS);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, CurrentFrame, _animationLoop->getCurrentFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, Running, _animationLoop->getRunning);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, Loop, _animationLoop->getLoop);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, FirstFrame, _animationLoop->getFirstFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, LastFrame, _animationLoop->getLastFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, Hold, _animationLoop->getHold);
    } else {
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, FPS, getFPS);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, CurrentFrame, getCurrentFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, Running, getRunning);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, Loop, getLoop);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, FirstFrame, getFirstFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, LastFrame, getLastFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, Hold, getHold);
    }
}

bool AnimationPropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, URL, url, setURL);
    if (_animationLoop) {
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, FPS, fps, _animationLoop->setFPS);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, CurrentFrame, currentFrame, _animationLoop->setCurrentFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, Running, running, _animationLoop->setRunning);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, Loop, loop, _animationLoop->setLoop);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, FirstFrame, firstFrame, _animationLoop->setFirstFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, LastFrame, lastFrame, _animationLoop->setLastFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, Hold, hold, _animationLoop->setHold);
    } else {
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, FPS, fps, setFPS);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, CurrentFrame, currentFrame, setCurrentFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, Running, running, setRunning);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, Loop, loop, setLoop);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, FirstFrame, firstFrame, setFirstFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, LastFrame, lastFrame, setLastFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, Hold, hold, setHold);
    }

    return somethingChanged;
}

EntityPropertyFlags AnimationPropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_ANIMATION_URL;
    requestedProperties += PROP_ANIMATION_FPS;
    requestedProperties += PROP_ANIMATION_FRAME_INDEX;
    requestedProperties += PROP_ANIMATION_PLAYING;
    requestedProperties += PROP_ANIMATION_LOOP;
    requestedProperties += PROP_ANIMATION_FIRST_FRAME;
    requestedProperties += PROP_ANIMATION_LAST_FRAME;
    requestedProperties += PROP_ANIMATION_HOLD;

    return requestedProperties;
}

void AnimationPropertyGroup::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount,
                                OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_URL, getURL());
    if (_animationLoop) {
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, _animationLoop->getFPS());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, _animationLoop->getCurrentFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, _animationLoop->getRunning());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, _animationLoop->getLoop());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, _animationLoop->getFirstFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, _animationLoop->getLastFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, _animationLoop->getHold());
    } else {
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, getFPS());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getCurrentFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, getRunning());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, getLoop());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, getFirstFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, getLastFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, getHold());
    }
}

int AnimationPropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                            bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_ANIMATION_URL, QString, setURL);

    if (_animationLoop) {
        // apply new properties to our associated AnimationLoop
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, _animationLoop->setFPS);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, _animationLoop->setCurrentFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, _animationLoop->setRunning);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, _animationLoop->setLoop);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, _animationLoop->setFirstFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, _animationLoop->setLastFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, _animationLoop->setHold);
    } else {
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setFPS);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setCurrentFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setRunning);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, setLoop);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, setFirstFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, setLastFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, setHold);
    }

    return bytesRead;
}
