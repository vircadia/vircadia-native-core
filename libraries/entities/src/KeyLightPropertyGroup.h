//
//  KeyLightPropertyGroup.h
//  libraries/entities/src
//
//  Created by Sam Gateau on 2015/10/23.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_KeyLightPropertyGroup_h
#define hifi_KeyLightPropertyGroup_h

#include <stdint.h>

#include <glm/glm.hpp>

#include <QtScript/QScriptEngine>
#include "EntityItemPropertiesMacros.h"
#include "PropertyGroup.h"

class EntityItemProperties;
class EncodeBitstreamParams;
class OctreePacketData;
class EntityTreeElementExtraEncodeData;
class ReadBitstreamToTreeParams;

class KeyLightPropertyGroup : public PropertyGroup {
public:
    // EntityItemProperty related helpers
    virtual void copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const;
    virtual void copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings);
    virtual void debugDump() const;
    virtual void listChangedProperties(QList<QString>& out);

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
  
    static const xColor DEFAULT_KEYLIGHT_COLOR;
    static const float DEFAULT_KEYLIGHT_INTENSITY;
    static const float DEFAULT_KEYLIGHT_AMBIENT_INTENSITY;
    static const glm::vec3 DEFAULT_KEYLIGHT_DIRECTION;
    
    DEFINE_PROPERTY_REF(PROP_KEYLIGHT_COLOR, Color, color, xColor, DEFAULT_KEYLIGHT_COLOR);
    DEFINE_PROPERTY(PROP_KEYLIGHT_INTENSITY, Intensity, intensity, float, DEFAULT_KEYLIGHT_INTENSITY);
    DEFINE_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, AmbientIntensity, ambientIntensity, float, DEFAULT_KEYLIGHT_AMBIENT_INTENSITY);
    DEFINE_PROPERTY_REF(PROP_KEYLIGHT_DIRECTION, Direction, direction, glm::vec3, DEFAULT_KEYLIGHT_DIRECTION);
    DEFINE_PROPERTY_REF(PROP_KEYLIGHT_AMBIENT_URL, AmbientURL, ambientURL, QString, "");
    
protected:
    
};

#endif // hifi_KeyLightPropertyGroup_h
