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

#include <OctreePacketData.h>

#include <AnimationLoop.h>

#include "AnimationPropertyGroup.h"
#include "EntityItemProperties.h"
#include "EntityItemPropertiesMacros.h"

AnimationPropertyGroup::AnimationPropertyGroup() {
    //_url = QString();
}

void AnimationPropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_URL, AnimationSettings, animationSettings, URL, url);

    if (_animationLoop) {
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_FPS, AnimationSettings, animationSettings, FPS, fps, _animationLoop->getFPS);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_FRAME_INDEX, AnimationSettings, animationSettings, FrameIndex, frameIndex, _animationLoop->getFPS);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_FRAME_INDEX, AnimationSettings, animationSettings, Running, running, _animationLoop->getFPS);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_LOOP, AnimationSettings, animationSettings, Loop, loop, _animationLoop->getFPS);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_FIRST_FRAME, AnimationSettings, animationSettings, FirstFrame, firstFrame, _animationLoop->getFPS);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_LAST_FRAME, AnimationSettings, animationSettings, LastFrame, lastFrame, _animationLoop->getFPS);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_HOLD, AnimationSettings, animationSettings, Hold, hold, _animationLoop->getFPS);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ANIMATION_START_AUTOMATICALLY, AnimationSettings, animationSettings, StartAutomatically, startAutomatically, _animationLoop->getFPS);
    } else {
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FPS, AnimationSettings, animationSettings, FPS, fps);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FRAME_INDEX, AnimationSettings, animationSettings, FrameIndex, frameIndex);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FRAME_INDEX, AnimationSettings, animationSettings, Running, running);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_LOOP, AnimationSettings, animationSettings, Loop, loop);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FIRST_FRAME, AnimationSettings, animationSettings, FirstFrame, firstFrame);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_LAST_FRAME, AnimationSettings, animationSettings, LastFrame, lastFrame);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_HOLD, AnimationSettings, animationSettings, Hold, hold);
        COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_START_AUTOMATICALLY, AnimationSettings, animationSettings, StartAutomatically, startAutomatically);
    }
}

void AnimationPropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, url, QString, setURL);

    if (_animationLoop) {
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, fps, float, _animationLoop->setFPS);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, frameIndex, float, _animationLoop->setFrameIndex);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, running, bool, _animationLoop->setRunning);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, loop, bool, _animationLoop->setLoop);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, firstFrame, float, _animationLoop->setFirstFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, lastFrame, float, _animationLoop->setLastFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, hold, bool, _animationLoop->setHold);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, startAutomatically, bool, _animationLoop->setStartAutomatically);
    } else {
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, fps, float, setFPS);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, frameIndex, float, setFrameIndex);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, running, bool, setRunning);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, loop, bool, setLoop);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, firstFrame, float, setFirstFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, lastFrame, float, setLastFrame);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, hold, bool, setHold);
        COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, startAutomatically, bool, setStartAutomatically);
    }
}

void AnimationPropertyGroup::debugDump() const {
    qDebug() << "   AnimationPropertyGroup: ---------------------------------------------";
    qDebug() << "       URL:" << getURL() << " has changed:" << urlChanged();
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
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, _animationLoop->getFrameIndex());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, _animationLoop->getRunning());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, _animationLoop->getLoop());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, _animationLoop->getFirstFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, _animationLoop->getLastFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, _animationLoop->getHold());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, _animationLoop->getStartAutomatically());
    } else {
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, getFPS());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getFrameIndex());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getRunning());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, getLoop());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, getFirstFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, getLastFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, getHold());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, getStartAutomatically());
    }

    return true;
}


bool AnimationPropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;

    READ_ENTITY_PROPERTY(PROP_ANIMATION_URL, QString, setURL);
    if (_animationLoop) {
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, _animationLoop->setFPS);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, _animationLoop->setFrameIndex);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, _animationLoop->setRunning);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, _animationLoop->setLoop);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, _animationLoop->setFirstFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, _animationLoop->setLastFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, _animationLoop->setHold);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, bool, _animationLoop->setStartAutomatically);
    } else {
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setFPS);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setFrameIndex);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setRunning);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, setLoop);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, setFirstFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, setLastFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, setHold);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, bool, setStartAutomatically);
    }

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_URL, URL);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_FPS, FPS);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_FRAME_INDEX, FrameIndex);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_PLAYING, Running);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_LOOP, Loop);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_FIRST_FRAME, FirstFrame);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_LAST_FRAME, LastFrame);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_HOLD, Hold);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_ANIMATION_START_AUTOMATICALLY, StartAutomatically);

    processedBytes += bytesRead;

    return true;
}

void AnimationPropertyGroup::markAllChanged() {
    _urlChanged = true;
    _fpsChanged = true;
    _frameIndexChanged = true;
    _runningChanged = true;
}

EntityPropertyFlags AnimationPropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;
    
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_URL, url);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_FPS, fps);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_FRAME_INDEX, frameIndex);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_PLAYING, running);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_LOOP, loop);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_FIRST_FRAME, firstFrame);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_LAST_FRAME, lastFrame);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_HOLD, hold);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_START_AUTOMATICALLY, startAutomatically);

    return changedProperties;
}

void AnimationPropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, URL, getURL);

    if (_animationLoop) {
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, FPS, _animationLoop->getFPS);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, FrameIndex, _animationLoop->getFrameIndex);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, Running, _animationLoop->getRunning);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, Loop, _animationLoop->getLoop);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, FirstFrame, _animationLoop->getFirstFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, LastFrame, _animationLoop->getLastFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, Hold, _animationLoop->getHold);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, StartAutomatically, _animationLoop->getStartAutomatically);
    } else {
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, FPS, getFPS);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, FrameIndex, getFrameIndex);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, Running, getRunning);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, Loop, getLoop);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, FirstFrame, getFirstFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, LastFrame, getLastFrame);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, Hold, getHold);
        COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(AnimationSettings, StartAutomatically, getStartAutomatically);
    }
}

bool AnimationPropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, URL, url, setURL);

    if (_animationLoop) {
        qDebug() << "AnimationPropertyGroup::setProperties() -- apply new properties to our associated AnimationLoop";

        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, FPS, fps, _animationLoop->setFPS);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, FrameIndex, frameIndex, _animationLoop->setFrameIndex);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, Running, running, _animationLoop->setRunning);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, Loop, loop, _animationLoop->setLoop);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, FirstFrame, firstFrame, _animationLoop->setFirstFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, LastFrame, lastFrame, _animationLoop->setLastFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, Hold, hold, _animationLoop->setHold);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, StartAutomatically, startAutomatically, _animationLoop->setStartAutomatically);
    } else {
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, FPS, fps, setFPS);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, FrameIndex, frameIndex, setFrameIndex);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, Running, running, setRunning);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, Loop, loop, setLoop);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, FirstFrame, firstFrame, setFirstFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, LastFrame, lastFrame, setLastFrame);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, Hold, hold, setHold);
        SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(AnimationSettings, StartAutomatically, startAutomatically, setStartAutomatically);
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
    requestedProperties += PROP_ANIMATION_START_AUTOMATICALLY;

    return requestedProperties;
}
    
void AnimationPropertyGroup::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                EntityPropertyFlags& requestedProperties,
                                EntityPropertyFlags& propertyFlags,
                                EntityPropertyFlags& propertiesDidntFit,
                                int& propertyCount, 
                                OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_URL, getURL());
    if (_animationLoop) {
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, getFPS());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getFrameIndex());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, getRunning());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, getLoop());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, getFirstFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, getLastFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, getHold());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, getStartAutomatically());
    } else {
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, _animationLoop->getFPS());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, _animationLoop->getFrameIndex());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, _animationLoop->getRunning());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, _animationLoop->getLoop());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, _animationLoop->getFirstFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, _animationLoop->getLastFrame());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, _animationLoop->getHold());
        APPEND_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, _animationLoop->getStartAutomatically());
    }
}

int AnimationPropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_ANIMATION_URL, QString, setURL);
    if (_animationLoop) {
        // apply new properties to our associated AnimationLoop
        qDebug() << "AnimationPropertyGroup::readEntitySubclassDataFromBuffer() -- apply new properties to our associated AnimationLoop";
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, _animationLoop->setFPS);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, _animationLoop->setFrameIndex);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, _animationLoop->setRunning);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, _animationLoop->setLoop);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, _animationLoop->setFirstFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, _animationLoop->setLastFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, _animationLoop->setHold);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, bool, _animationLoop->setStartAutomatically);
    } else {
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setFPS);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setFrameIndex);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setRunning);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, setLoop);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, setFirstFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, setLastFrame);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, setHold);
        READ_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, bool, setStartAutomatically);
    }


    return bytesRead;
}
