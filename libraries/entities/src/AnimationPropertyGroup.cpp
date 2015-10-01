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

#include "AnimationPropertyGroup.h"
#include "EntityItemProperties.h"
#include "EntityItemPropertiesMacros.h"

AnimationPropertyGroup::AnimationPropertyGroup() {
    //_url = QString();
}

void AnimationPropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_URL, Animation, animation, URL, url);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FPS, Animation, animation, FPS, fps);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FRAME_INDEX, Animation, animation, FrameIndex, frameIndex);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FRAME_INDEX, Animation, animation, Running, running);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_LOOP, Animation, animation, Loop, loop);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_FIRST_FRAME, Animation, animation, FirstFrame, firstFrame);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_LAST_FRAME, Animation, animation, LastFrame, lastFrame);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_HOLD, Animation, animation, Hold, hold);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_ANIMATION_START_AUTOMATICALLY, Animation, animation, StartAutomatically, startAutomatically);
}

void AnimationPropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, url, QString, setURL);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, fps, float, setFPS);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, frameIndex, float, setFrameIndex);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, running, bool, setRunning);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, loop, bool, setLoop);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, firstFrame, float, setFirstFrame);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, lastFrame, float, setLastFrame);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, hold, bool, setHold);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(animation, startAutomatically, bool, setStartAutomatically);
}

void AnimationPropertyGroup::debugDump() const {
    qDebug() << "   AnimationPropertyGroup: ---------------------------------------------";
    qDebug() << "       URL:" << getURL() << " has changed:" << urlChanged();
}

bool AnimationPropertyGroup::appentToEditPacket(OctreePacketData* packetData,                                     
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_URL, getURL());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, getFPS());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getFrameIndex());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getRunning());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, getLoop());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, getFirstFrame());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, getLastFrame());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, getHold());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, getStartAutomatically());

    return true;
}


bool AnimationPropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;

    READ_ENTITY_PROPERTY(PROP_ANIMATION_URL, QString, setURL);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setFPS);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setFrameIndex);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setRunning);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, setLoop);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, setFirstFrame);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, setLastFrame);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, setHold);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, bool, setStartAutomatically);

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
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, URL, getURL);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, FPS, getFPS);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, FrameIndex, getFrameIndex);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, Running, getRunning);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, Loop, getLoop);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, FirstFrame, getFirstFrame);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, LastFrame, getLastFrame);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, Hold, getHold);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Animation, StartAutomatically, getStartAutomatically);
}

bool AnimationPropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, URL, url, setURL);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, FPS, fps, setFPS);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, FrameIndex, frameIndex, setFrameIndex);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, Running, running, setRunning);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, Loop, loop, setLoop);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, FirstFrame, firstFrame, setFirstFrame);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, LastFrame, lastFrame, setLastFrame);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, Hold, hold, setHold);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Animation, StartAutomatically, startAutomatically, setStartAutomatically);

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
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, getFPS());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, getFrameIndex());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, getRunning());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, getLoop());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, getFirstFrame());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, getLastFrame());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, getHold());
    APPEND_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, getStartAutomatically());
}

int AnimationPropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                            ReadBitstreamToTreeParams& args,
                                            EntityPropertyFlags& propertyFlags, bool overwriteLocalData) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_ANIMATION_URL, QString, setURL);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FPS, float, setFPS);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, float, setFrameIndex);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, bool, setRunning);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_LOOP, bool, setLoop);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_FIRST_FRAME, float, setFirstFrame);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_LAST_FRAME, float, setLastFrame);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_HOLD, bool, setHold);
    READ_ENTITY_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, bool, setStartAutomatically);

    return bytesRead;
}
