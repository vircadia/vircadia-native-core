//
//  PropertyGroup.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PropertyGroup_h
#define hifi_PropertyGroup_h

#include <QtScript/QScriptEngine>

//#include "EntityItemProperties.h"
#include "EntityPropertyFlags.h"

class EntityItemProperties;
class EncodeBitstreamParams;
class OctreePacketData;
class EntityTreeElementExtraEncodeData;
class ReadBitstreamToTreeParams;

#include <OctreeElement.h>

/*
#include <stdint.h>

#include <glm/glm.hpp>
#include <glm/gtx/extented_min_max.hpp>

#include <QtCore/QObject>
#include <QVector>
#include <QString>

#include <AACube.h>
#include <FBXReader.h> // for SittingPoint
#include <PropertyFlags.h>
#include <OctreeConstants.h>
#include <ShapeInfo.h>

#include "EntityItemID.h"
#include "PropertyGroupMacros.h"
#include "EntityTypes.h"
*/

//typedef PropertyFlags<EntityPropertyList> EntityPropertyFlags;


class PropertyGroup {
public:
    virtual ~PropertyGroup() = default;

    // EntityItemProperty related helpers
    virtual void copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties, QScriptEngine* engine, bool skipDefaults, EntityItemProperties& defaultEntityProperties) const = 0;
    virtual void copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) = 0;
    virtual void debugDump() const { }
    virtual void listChangedProperties(QList<QString>& out) { }

    virtual bool appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const = 0;

    virtual bool decodeFromEditPacket(EntityPropertyFlags& propertyFlags, const unsigned char*& dataAt , int& processedBytes) = 0;
    virtual void markAllChanged() = 0;
    virtual EntityPropertyFlags getChangedProperties() const = 0;

    // EntityItem related helpers
    // methods for getting/setting all properties of an entity
    virtual void getProperties(EntityItemProperties& propertiesOut) const = 0;
    
    /// returns true if something changed
    virtual bool setProperties(const EntityItemProperties& properties) = 0;

    /// Override this in your derived class if you'd like to be informed when something about the state of the entity
    /// has changed. This will be called with properties change or when new data is loaded from a stream
    virtual void somethingChangedNotification() { }

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const = 0;
        
    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                    EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount, 
                                    OctreeElement::AppendState& appendState) const = 0;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead, 
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) = 0;
};

#endif // hifi_PropertyGroup_h
