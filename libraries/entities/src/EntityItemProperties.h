//
//  EntityItemProperties.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityItemProperties_h
#define hifi_EntityItemProperties_h

#define HIDE_SUBCLASS_METHODS 1

#include <stdint.h>

#include <glm/glm.hpp>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>
#include <QVector>
#include <QString>

#include <AACube.h>
#include <FBXReader.h> // for SittingPoint
#include <PropertyFlags.h>
#include <OctreeConstants.h>


#include "EntityItemID.h"
#include "EntityTypes.h"


const uint16_t ENTITY_PACKET_CONTAINS_RADIUS = 1;
const uint16_t ENTITY_PACKET_CONTAINS_POSITION = 2;
const uint16_t ENTITY_PACKET_CONTAINS_COLOR = 4;
const uint16_t ENTITY_PACKET_CONTAINS_SHOULDDIE = 8;
const uint16_t ENTITY_PACKET_CONTAINS_MODEL_URL = 16;
const uint16_t ENTITY_PACKET_CONTAINS_ROTATION = 32;
const uint16_t ENTITY_PACKET_CONTAINS_ANIMATION_URL = 64;
const uint16_t ENTITY_PACKET_CONTAINS_ANIMATION_PLAYING = 128;
const uint16_t ENTITY_PACKET_CONTAINS_ANIMATION_FRAME = 256;
const uint16_t ENTITY_PACKET_CONTAINS_ANIMATION_FPS = 512;

const float ENTITY_DEFAULT_RADIUS = 0.1f / TREE_SCALE;
const float ENTITY_MINIMUM_ELEMENT_SIZE = (1.0f / 100000.0f) / TREE_SCALE; // smallest size container
const QString ENTITY_DEFAULT_MODEL_URL("");
const glm::quat ENTITY_DEFAULT_ROTATION;
const QString ENTITY_DEFAULT_ANIMATION_URL("");
const float ENTITY_DEFAULT_ANIMATION_FPS = 30.0f;

// PropertyFlags support
enum EntityPropertyList {
    PROP_PAGED_PROPERTY,
    PROP_CUSTOM_PROPERTIES_INCLUDED,
    PROP_VISIBLE,
    PROP_POSITION,
    PROP_RADIUS,
    PROP_ROTATION,
    PROP_SCRIPT,
    PROP_MODEL_URL,
    PROP_COLOR,
    PROP_ANIMATION_URL,
    PROP_ANIMATION_FPS,
    PROP_ANIMATION_FRAME_INDEX,
    PROP_ANIMATION_PLAYING,
    PROP_SHOULD_BE_DELETED,
    PROP_LAST_ITEM = PROP_SHOULD_BE_DELETED
};

typedef PropertyFlags<EntityPropertyList> EntityPropertyFlags;


/// A collection of properties of an entity item used in the scripting API. Translates between the actual properties of an
/// entity and a JavaScript style hash/QScriptValue storing a set of properties. Used in scripting to set/get the complete
/// set of entity item properties via JavaScript hashes/QScriptValues
/// all units for position, radius, etc are in meter units
class EntityItemProperties {
    friend class EntityItem; // TODO: consider removing this friend relationship and have EntityItem use public methods
public:
    EntityItemProperties();
    virtual ~EntityItemProperties() { };

    virtual QScriptValue copyToScriptValue(QScriptEngine* engine) const;
    virtual void copyFromScriptValue(const QScriptValue& object);

    // editing related features supported by all entities
    quint64 getLastEdited() const { return _lastEdited; }
    uint16_t getChangedBits() const;
    EntityPropertyFlags getChangedProperties() const;

    /// used by EntityScriptingInterface to return EntityItemProperties for unknown models
    void setIsUnknownID() { _id = UNKNOWN_ENTITY_ID; _idSet = true; }
    
    glm::vec3 getMinimumPointMeters() const { return _position - glm::vec3(_radius, _radius, _radius); }
    glm::vec3 getMaximumPointMeters() const { return _position + glm::vec3(_radius, _radius, _radius); }
    AACube getAACubeMeters() const { return AACube(getMinimumPointMeters(), getMaxDimension()); } /// AACube in meter units

    glm::vec3 getMinimumPointTreeUnits() const { return getMinimumPointMeters() / (float)TREE_SCALE; }
    glm::vec3 getMaximumPointTreeUnits() const { return getMaximumPointMeters() / (float)TREE_SCALE; }
    /// AACube in domain scale units (0.0 - 1.0)
    AACube getAACubeTreeUnits() const { 
        glm::vec3 cornerInTreeUnits = getMinimumPointMeters()/(float)TREE_SCALE;
        float dimensionInTreeUnits = getMaxDimension()/(float)TREE_SCALE;
    
    qDebug() << "getAACubeTreeUnits()";
    qDebug() << "   corner in meters=" << getMinimumPointMeters().x << "," << getMinimumPointMeters().y << "," << getMinimumPointMeters().z;
    qDebug() << "   dimension in meters=" << getMaxDimension();
    qDebug() << "   corner in tree units=" << cornerInTreeUnits.x << "," << cornerInTreeUnits.y << "," << cornerInTreeUnits.z;
    qDebug() << "   dimension in tree units=" << dimensionInTreeUnits;

        return AACube(cornerInTreeUnits, dimensionInTreeUnits); 
    }


    void debugDump() const;

    // properties of all entities
    EntityTypes::EntityType_t getType() const { return _type; }
    const glm::vec3& getPosition() const { return _position; }
    float getRadius() const { return _radius; }
    float getMaxDimension() const { return _radius * 2.0f; }
    glm::vec3 getDimensions() const { return glm::vec3(_radius, _radius, _radius) * 2.0f; }
    const glm::quat& getRotation() const { return _rotation; }
    bool getShouldBeDeleted() const { return _shouldBeDeleted; }

    void setType(EntityTypes::EntityType_t type) { _type = type; }
    /// set position in meter units
    void setPosition(const glm::vec3& value) { _position = value; _positionChanged = true; }
    void setRadius(float value) { _radius = value; _radiusChanged = true; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; _rotationChanged = true; }
    void setShouldBeDeleted(bool shouldBeDeleted) { _shouldBeDeleted = shouldBeDeleted; _shouldBeDeletedChanged = true;  }
    
    // NOTE: how do we handle _defaultSettings???
    bool containsBoundsProperties() const { return (_positionChanged || _radiusChanged); }
    bool containsPositionChange() const { return _positionChanged; }
    bool containsRadiusChange() const { return _radiusChanged; }

#ifdef HIDE_SUBCLASS_METHODS
    // properties we want to move to just models and particles
    xColor getColor() const { return _color; }
    const QString& getModelURL() const { return _modelURL; }
    const QString& getAnimationURL() const { return _animationURL; }
    float getAnimationFrameIndex() const { return _animationFrameIndex; }
    bool getAnimationIsPlaying() const { return _animationIsPlaying;  }
    float getAnimationFPS() const { return _animationFPS; }
    float getGlowLevel() const { return _glowLevel; }

    // model related properties
    void setColor(const xColor& value) { _color = value; _colorChanged = true; }
    void setModelURL(const QString& url) { _modelURL = url; _modelURLChanged = true; }
    void setAnimationURL(const QString& url) { _animationURL = url; _animationURLChanged = true; }
    void setAnimationFrameIndex(float value) { _animationFrameIndex = value; _animationFrameIndexChanged = true; }
    void setAnimationIsPlaying(bool value) { _animationIsPlaying = value; _animationIsPlayingChanged = true;  }
    void setAnimationFPS(float value) { _animationFPS = value; _animationFPSChanged = true; }
    void setGlowLevel(float value) { _glowLevel = value; _glowLevelChanged = true; }
#endif

private:
    quint32 _id;
    bool _idSet;
    quint64 _lastEdited;

    EntityTypes::EntityType_t _type;
    glm::vec3 _position;
    float _radius;
    glm::quat _rotation;
    bool _shouldBeDeleted;

    bool _positionChanged;
    bool _radiusChanged;
    bool _rotationChanged;
    bool _shouldBeDeletedChanged;
    
#ifdef HIDE_SUBCLASS_METHODS
    xColor _color;
    QString _modelURL;
    QString _animationURL;
    bool _animationIsPlaying;
    float _animationFrameIndex;
    float _animationFPS;
    float _glowLevel;
    QVector<SittingPoint> _sittingPoints;

    bool _colorChanged;
    bool _modelURLChanged;
    bool _animationURLChanged;
    bool _animationIsPlayingChanged;
    bool _animationFrameIndexChanged;
    bool _animationFPSChanged;
    bool _glowLevelChanged;
#endif

    bool _defaultSettings;
};
Q_DECLARE_METATYPE(EntityItemProperties);
QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties);


#endif // hifi_EntityItemProperties_h
