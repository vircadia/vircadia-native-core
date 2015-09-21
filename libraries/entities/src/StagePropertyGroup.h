//
//  StagePropertyGroup.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_StagePropertyGroup_h
#define hifi_StagePropertyGroup_h

#include <QtScript/QScriptEngine>

#include "PropertyGroup.h"
#include "EntityItemPropertiesMacros.h"

class EntityItemProperties;
class EncodeBitstreamParams;
class OctreePacketData;
class EntityTreeElementExtraEncodeData;
class ReadBitstreamToTreeParams;

#include <stdint.h>
#include <glm/glm.hpp>


class StagePropertyGroup : public PropertyGroup {
public:
    StagePropertyGroup();
    virtual ~StagePropertyGroup() {}

    // EntityItemProperty related helpers
    virtual void copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const;
    virtual void copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings);
    virtual void debugDump() const;

    virtual bool appentToEditPacket(OctreePacketData* packetData,                                     
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
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData);
                                                
    static const bool DEFAULT_STAGE_SUN_MODEL_ENABLED;
    static const float DEFAULT_STAGE_LATITUDE;
    static const float DEFAULT_STAGE_LONGITUDE;
    static const float DEFAULT_STAGE_ALTITUDE;
    static const quint16 DEFAULT_STAGE_DAY;
    static const float DEFAULT_STAGE_HOUR;
    
    float calculateHour() const;
    uint16_t calculateDay() const;

    DEFINE_PROPERTY(PROP_STAGE_SUN_MODEL_ENABLED, SunModelEnabled, sunModelEnabled, bool);
    DEFINE_PROPERTY(PROP_STAGE_LATITUDE, Latitude, latitude, float);
    DEFINE_PROPERTY(PROP_STAGE_LONGITUDE, Longitude, longitude, float);
    DEFINE_PROPERTY(PROP_STAGE_ALTITUDE, Altitude, altitude, float);
    DEFINE_PROPERTY(PROP_STAGE_DAY, Day, day, uint16_t);
    DEFINE_PROPERTY(PROP_STAGE_HOUR, Hour, hour, float);
    DEFINE_PROPERTY(PROP_STAGE_AUTOMATIC_HOURDAY, AutomaticHourDay, automaticHourDay, bool);
};

#endif // hifi_StagePropertyGroup_h
