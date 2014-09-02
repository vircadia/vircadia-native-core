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


// TODO: should these be static members of EntityItem or EntityItemProperties?
const float ENTITY_DEFAULT_RADIUS = 0.1f / TREE_SCALE;
const float ENTITY_MINIMUM_ELEMENT_SIZE = (1.0f / 100000.0f) / TREE_SCALE; // smallest size container
const QString ENTITY_DEFAULT_MODEL_URL("");
const glm::quat ENTITY_DEFAULT_ROTATION;
const QString ENTITY_DEFAULT_ANIMATION_URL("");
const float ENTITY_DEFAULT_ANIMATION_FPS = 30.0f;

const quint64 UNKNOWN_CREATED_TIME = (quint64)(-1);
const quint64 USE_EXISTING_CREATED_TIME = (quint64)(-2);

// PropertyFlags support
enum EntityPropertyList {
    PROP_PAGED_PROPERTY,
    PROP_CUSTOM_PROPERTIES_INCLUDED,
    
    // these properties are supported by the EntityItem base class
    PROP_VISIBLE,
    PROP_POSITION,
    PROP_RADIUS,
    PROP_ROTATION,
    PROP_MASS,
    PROP_VELOCITY,
    PROP_GRAVITY,
    PROP_DAMPING,
    PROP_LIFETIME,
    PROP_SCRIPT,

    // these properties are supported by some derived classes
    PROP_COLOR,
    PROP_MODEL_URL,
    PROP_ANIMATION_URL,
    PROP_ANIMATION_FPS,
    PROP_ANIMATION_FRAME_INDEX,
    PROP_ANIMATION_PLAYING,

    PROP_LAST_ITEM = PROP_ANIMATION_PLAYING
};

typedef PropertyFlags<EntityPropertyList> EntityPropertyFlags;


/// A collection of properties of an entity item used in the scripting API. Translates between the actual properties of an
/// entity and a JavaScript style hash/QScriptValue storing a set of properties. Used in scripting to set/get the complete
/// set of entity item properties via JavaScript hashes/QScriptValues
/// all units for position, radius, etc are in meter units
class EntityItemProperties {
    friend class EntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class ModelEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class BoxEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class SphereEntityItem; // TODO: consider removing this friend relationship and use public methods
public:
    EntityItemProperties();
    virtual ~EntityItemProperties() { };

    virtual QScriptValue copyToScriptValue(QScriptEngine* engine) const;
    virtual void copyFromScriptValue(const QScriptValue& object);

    // editing related features supported by all entities
    quint64 getLastEdited() const { return _lastEdited; }
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
        return AACube(getMinimumPointMeters() / (float)TREE_SCALE, getMaxDimension() / (float)TREE_SCALE); 
    }

    void debugDump() const;

    // properties of all entities
    EntityTypes::EntityType getType() const { return _type; }
    const glm::vec3& getPosition() const { return _position; }
    float getRadius() const { return _radius; }
    float getMaxDimension() const { return _radius * 2.0f; }
    glm::vec3 getDimensions() const { return glm::vec3(_radius, _radius, _radius) * 2.0f; }
    const glm::quat& getRotation() const { return _rotation; }

    void setType(EntityTypes::EntityType type) { _type = type; }
    /// set position in meter units, will be clamped to domain bounds
    void setPosition(const glm::vec3& value) { _position = glm::clamp(value, 0.0f, (float)TREE_SCALE); _positionChanged = true; }
    void setRadius(float value) { _radius = value; _radiusChanged = true; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; _rotationChanged = true; }

    float getMass() const { return _mass; }
    void setMass(float value) { _mass = value; _massChanged = true; }

    /// velocity in domain scale units (0.0-1.0) per second
    const glm::vec3& getVelocity() const { return _velocity; }
    /// velocity in domain scale units (0.0-1.0) per second
    void setVelocity(const glm::vec3& value) { _velocity = value; _velocityChanged = true; }

    /// gravity in domain scale units (0.0-1.0) per second squared
    const glm::vec3& getGravity() const { return _gravity; }
    /// gravity in domain scale units (0.0-1.0) per second squared
    void setGravity(const glm::vec3& value) { _gravity = value; _gravityChanged = true; }

    float getDamping() const { return _damping; }
    void setDamping(float value) { _damping = value; _dampingChanged = true; }

    float getLifetime() const { return _lifetime; } /// get the lifetime in seconds for the entity
    void setLifetime(float value) { _lifetime = value; _lifetimeChanged = true; } /// set the lifetime in seconds for the entity
    float getAge() const { return (float)(usecTimestampNow() - _created) / (float)USECS_PER_SECOND; }
    quint64 getCreated() const { return _created; }
    void setCreated(quint64 usecTime) { _created = usecTime; }
    bool hasCreatedTime() const { return (_created != UNKNOWN_CREATED_TIME); }

    
    // NOTE: how do we handle _defaultSettings???
    bool containsBoundsProperties() const { return (_positionChanged || _radiusChanged); }
    bool containsPositionChange() const { return _positionChanged; }
    bool containsRadiusChange() const { return _radiusChanged; }

    // TODO: this need to be more generic. for now, we're going to have the properties class support these as
    // named getter/setters, but we want to move them to generic types...
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


    static bool encodeEntityEditPacket(PacketType command, EntityItemID id, const EntityItemProperties& properties,
        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static bool encodeEraseEntityMessage(const EntityItemID& entityItemID, 
                                            unsigned char* outputBuffer, size_t maxLength, size_t& outputLength);


    static bool decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes,
                        EntityItemID& entityID, EntityItemProperties& properties);

    bool positionChanged() const { return _positionChanged; }
    bool radiusChanged() const { return _radiusChanged; }
    bool rotationChanged() const { return _rotationChanged; }
    bool massChanged() const { return _massChanged; }
    bool velocityChanged() const { return _velocityChanged; }
    bool gravityChanged() const { return _gravityChanged; }
    bool dampingChanged() const { return _dampingChanged; }
    bool lifetimeChanged() const { return _lifetimeChanged; }
    bool scriptChanged() const { return _scriptChanged; }
    bool colorChanged() const { return _colorChanged; }
    bool modelURLChanged() const { return _modelURLChanged; }
    bool animationURLChanged() const { return _animationURLChanged; }
    bool animationIsPlayingChanged() const { return _animationIsPlayingChanged; }
    bool animationFrameIndexChanged() const { return _animationFrameIndexChanged; }
    bool animationFPSChanged() const { return _animationFPSChanged; }
    bool glowLevelChanged() const { return _glowLevelChanged; }

    void clearID() { _id = UNKNOWN_ENTITY_ID; _idSet = false; }
    void markAllChanged();

private:
    void setLastEdited(quint64 usecTime) { _lastEdited = usecTime; }

    QUuid _id;
    bool _idSet;
    quint64 _lastEdited;
    quint64 _created;

    EntityTypes::EntityType _type;
    glm::vec3 _position;
    float _radius;
    glm::quat _rotation;
    float _mass;
    glm::vec3 _velocity;
    glm::vec3 _gravity;
    float _damping;
    float _lifetime;
    QString _script;

    bool _positionChanged;
    bool _radiusChanged;
    bool _rotationChanged;
    bool _massChanged;
    bool _velocityChanged;
    bool _gravityChanged;
    bool _dampingChanged;
    bool _lifetimeChanged;
    bool _scriptChanged;
    
    // TODO: this need to be more generic. for now, we're going to have the properties class support these as
    // named getter/setters, but we want to move them to generic types...
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

    bool _defaultSettings;
};
Q_DECLARE_METATYPE(EntityItemProperties);
QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties);

#define APPEND_ENTITY_PROPERTY(P,O,V) \
        if (requestedProperties.getHasProperty(P)) {                \
            LevelDetails propertyLevel = packetData->startLevel();  \
            successPropertyFits = packetData->O(V);                 \
            if (successPropertyFits) {                              \
                propertyFlags |= P;                                 \
                propertiesDidntFit -= P;                            \
                propertyCount++;                                    \
                packetData->endLevel(propertyLevel);                \
            } else {                                                \
                packetData->discardLevel(propertyLevel);            \
                appendState = OctreeElement::PARTIAL;               \
            }                                                       \
        } else {                                                    \
            propertiesDidntFit -= P;                                \
        }

#define READ_ENTITY_PROPERTY(P,T,M)                             \
        if (propertyFlags.getHasProperty(P)) {                  \
            T fromBuffer;                                       \
            memcpy(&fromBuffer, dataAt, sizeof(fromBuffer));    \
            dataAt += sizeof(fromBuffer);                       \
            bytesRead += sizeof(fromBuffer);                    \
            if (overwriteLocalData) {                           \
                M = fromBuffer;                                 \
            }                                                   \
        }

#define READ_ENTITY_PROPERTY_QUAT(P,M)                                      \
        if (propertyFlags.getHasProperty(P)) {                              \
            glm::quat fromBuffer;                                           \
            int bytes = unpackOrientationQuatFromBytes(dataAt, fromBuffer); \
            dataAt += bytes;                                                \
            bytesRead += bytes;                                             \
            if (overwriteLocalData) {                                       \
                M = fromBuffer;                                             \
            }                                                               \
        }

#define READ_ENTITY_PROPERTY_STRING(P,O)                \
        if (propertyFlags.getHasProperty(P)) {          \
            uint16_t length;                            \
            memcpy(&length, dataAt, sizeof(length));    \
            dataAt += sizeof(length);                   \
            bytesRead += sizeof(length);                \
            QString value((const char*)dataAt);         \
            dataAt += length;                           \
            bytesRead += length;                        \
            if (overwriteLocalData) {                   \
                O(value);                               \
            }                                           \
        }

#define READ_ENTITY_PROPERTY_COLOR(P,M)         \
        if (propertyFlags.getHasProperty(P)) {  \
            if (overwriteLocalData) {           \
                memcpy(M, dataAt, sizeof(M));   \
            }                                   \
            dataAt += sizeof(rgbColor);         \
            bytesRead += sizeof(rgbColor);      \
        }

#define READ_ENTITY_PROPERTY_TO_PROPERTIES(P,T,O)               \
        if (propertyFlags.getHasProperty(P)) {                  \
            T fromBuffer;                                       \
            memcpy(&fromBuffer, dataAt, sizeof(fromBuffer));    \
            dataAt += sizeof(fromBuffer);                       \
            processedBytes += sizeof(fromBuffer);               \
            properties.O(fromBuffer);                           \
        }

#define READ_ENTITY_PROPERTY_QUAT_TO_PROPERTIES(P,O)                        \
        if (propertyFlags.getHasProperty(P)) {                              \
            glm::quat fromBuffer;                                           \
            int bytes = unpackOrientationQuatFromBytes(dataAt, fromBuffer); \
            dataAt += bytes;                                                \
            processedBytes += bytes;                                        \
            properties.O(fromBuffer);                                       \
        }

#define READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(P,O)  \
        if (propertyFlags.getHasProperty(P)) {          \
            uint16_t length;                            \
            memcpy(&length, dataAt, sizeof(length));    \
            dataAt += sizeof(length);                   \
            processedBytes += sizeof(length);           \
            QString value((const char*)dataAt);         \
            dataAt += length;                           \
            processedBytes += length;                   \
            properties.O(value);                        \
        }

#define READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(P,O)   \
        if (propertyFlags.getHasProperty(P)) {          \
            xColor color;                               \
            memcpy(&color, dataAt, sizeof(color));      \
            dataAt += sizeof(color);                    \
            processedBytes += sizeof(color);            \
            properties.O(color);                        \
        }



#endif // hifi_EntityItemProperties_h
