//
//  AnimationPropertyGroup.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 2015/9/30.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_AnimationPropertyGroup_h
#define hifi_AnimationPropertyGroup_h

#include <QtScript/QScriptEngine>

#include "PropertyGroup.h"
#include "EntityItemPropertiesMacros.h"

class EntityItemProperties;
class EncodeBitstreamParams;
class OctreePacketData;
class EntityTreeElementExtraEncodeData;
class ReadBitstreamToTreeParams;
class AnimationLoop;

#include <stdint.h>
#include <glm/glm.hpp>


class AnimationPropertyGroup : public PropertyGroup {
public:
    AnimationPropertyGroup();
    virtual ~AnimationPropertyGroup() {}
    void associateWithAnimationLoop(AnimationLoop* animationLoop) { _animationLoop = animationLoop; }

    // EntityItemProperty related helpers
    virtual void copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const;
    virtual void copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings);
    virtual void debugDump() const;

    virtual bool appendToEditPacket(OctreePacketData* packetData,                                     
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const;

    virtual bool decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes);
    virtual void markAllChanged();
    virtual EntityPropertyFlags getChangedProperties() const;

    // EntityItem related helpers
    // methods for getting/setting all properties of an entity
    virtual void getProperties(EntityItemProperties& propertiesOut) const;
    
    /// returns true if something changed
    virtual bool setProperties(const EntityItemProperties& properties);

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const;
        
    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged);
                                                
    DEFINE_PROPERTY_REF(PROP_ANIMATION_URL, URL, url, QString);
    DEFINE_PROPERTY(PROP_ANIMATION_FPS, FPS, fps, float);
    DEFINE_PROPERTY(PROP_ANIMATION_FRAME_INDEX, CurrentFrame, currentFrame, float);
    DEFINE_PROPERTY(PROP_ANIMATION_PLAYING, Running, running, bool); // was animationIsPlaying
    DEFINE_PROPERTY(PROP_ANIMATION_LOOP, Loop, loop, bool); // was animationSettings.loop
    DEFINE_PROPERTY(PROP_ANIMATION_FIRST_FRAME, FirstFrame, firstFrame, float); // was animationSettings.firstFrame
    DEFINE_PROPERTY(PROP_ANIMATION_LAST_FRAME, LastFrame, lastFrame, float); // was animationSettings.lastFrame
    DEFINE_PROPERTY(PROP_ANIMATION_HOLD, Hold, hold, bool); // was animationSettings.hold
    DEFINE_PROPERTY(PROP_ANIMATION_START_AUTOMATICALLY, StartAutomatically, startAutomatically, bool); // was animationSettings.startAutomatically

protected:
    void setFromOldAnimationSettings(const QString& value);

    AnimationLoop* _animationLoop = nullptr;
};

#endif // hifi_AnimationPropertyGroup_h
