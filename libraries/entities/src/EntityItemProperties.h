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
#include <glm/gtx/extented_min_max.hpp>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>
#include <QVector>
#include <QString>

#include <AACube.h>
#include <FBXReader.h> // for SittingPoint
#include <PropertyFlags.h>
#include <OctreeConstants.h>


#include "EntityItemID.h"
#include "EntityItemPropertiesMacros.h"
#include "EntityTypes.h"

// PropertyFlags support
enum EntityPropertyList {
    PROP_PAGED_PROPERTY,
    PROP_CUSTOM_PROPERTIES_INCLUDED,
    
    // these properties are supported by the EntityItem base class
    PROP_VISIBLE,
    PROP_POSITION,
    PROP_RADIUS, // NOTE: PROP_RADIUS is obsolete and only included in old format streams
    PROP_DIMENSIONS = PROP_RADIUS,
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

    // these properties are supported by the EntityItem base class
    PROP_REGISTRATION_POINT,
    PROP_ANGULAR_VELOCITY,
    PROP_ANGULAR_DAMPING,
    PROP_IGNORE_FOR_COLLISIONS,
    PROP_COLLISIONS_WILL_MOVE,

    // property used by Light entity
    PROP_IS_SPOTLIGHT,

    PROP_LAST_ITEM = PROP_IS_SPOTLIGHT
};

typedef PropertyFlags<EntityPropertyList> EntityPropertyFlags;

const quint64 UNKNOWN_CREATED_TIME = (quint64)(-1);
const quint64 USE_EXISTING_CREATED_TIME = (quint64)(-2);


/// A collection of properties of an entity item used in the scripting API. Translates between the actual properties of an
/// entity and a JavaScript style hash/QScriptValue storing a set of properties. Used in scripting to set/get the complete
/// set of entity item properties via JavaScript hashes/QScriptValues
/// all units for position, dimensions, etc are in meter units
class EntityItemProperties {
    friend class EntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class ModelEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class BoxEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class SphereEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class LightEntityItem; // TODO: consider removing this friend relationship and use public methods
public:
    EntityItemProperties();
    virtual ~EntityItemProperties() { };

    virtual QScriptValue copyToScriptValue(QScriptEngine* engine) const;
    virtual void copyFromScriptValue(const QScriptValue& object);

    // editing related features supported by all entities
    quint64 getLastEdited() const { return _lastEdited; }
    float getEditedAgo() const /// Elapsed seconds since this entity was last edited
        { return (float)(usecTimestampNow() - getLastEdited()) / (float)USECS_PER_SECOND; }
    EntityPropertyFlags getChangedProperties() const;

    /// used by EntityScriptingInterface to return EntityItemProperties for unknown models
    void setIsUnknownID() { _id = UNKNOWN_ENTITY_ID; _idSet = true; }
    
    AACube getMaximumAACubeInTreeUnits() const;
    AACube getMaximumAACubeInMeters() const;
    AABox getAABoxInMeters() const;

    void debugDump() const;


    // properties of all entities
    EntityTypes::EntityType getType() const { return _type; }

    void setType(EntityTypes::EntityType type) { _type = type; }

    const glm::vec3& getPosition() const { return _position; }
    /// set position in meter units, will be clamped to domain bounds
    void setPosition(const glm::vec3& value) { _position = glm::clamp(value, 0.0f, (float)TREE_SCALE); _positionChanged = true; }


    const glm::vec3& getDimensions() const { return _dimensions; }
    void setDimensions(const glm::vec3& value) { _dimensions = value; _dimensionsChanged = true; }
    float getMaxDimension() const { return glm::max(_dimensions.x, _dimensions.y, _dimensions.z); }

    const glm::quat& getRotation() const { return _rotation; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; _rotationChanged = true; }

    float getMass() const { return _mass; }
    void setMass(float value) { _mass = value; _massChanged = true; }

    /// velocity in meters (0.0-1.0) per second
    const glm::vec3& getVelocity() const { return _velocity; }
    /// velocity in meters (0.0-1.0) per second
    void setVelocity(const glm::vec3& value) { _velocity = value; _velocityChanged = true; }

    /// gravity in meters (0.0-TREE_SCALE) per second squared
    const glm::vec3& getGravity() const { return _gravity; }
    /// gravity in meters (0.0-TREE_SCALE) per second squared
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
    bool containsBoundsProperties() const { return (_positionChanged || _dimensionsChanged); }
    bool containsPositionChange() const { return _positionChanged; }
    bool containsDimensionsChange() const { return _dimensionsChanged; }

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
    float getLocalRenderAlpha() const { return _localRenderAlpha; }
    const QString& getScript() const { return _script; }

    // model related properties
    void setColor(const xColor& value) { _color = value; _colorChanged = true; }
    void setModelURL(const QString& url) { _modelURL = url; _modelURLChanged = true; }
    void setAnimationURL(const QString& url) { _animationURL = url; _animationURLChanged = true; }
    void setAnimationFrameIndex(float value) { _animationFrameIndex = value; _animationFrameIndexChanged = true; }
    void setAnimationIsPlaying(bool value) { _animationIsPlaying = value; _animationIsPlayingChanged = true;  }
    void setAnimationFPS(float value) { _animationFPS = value; _animationFPSChanged = true; }
    void setGlowLevel(float value) { _glowLevel = value; _glowLevelChanged = true; }
    void setLocalRenderAlpha(float value) { _localRenderAlpha = value; _localRenderAlphaChanged = true; }
    void setScript(const QString& value) { _script = value; _scriptChanged = true; }


    static bool encodeEntityEditPacket(PacketType command, EntityItemID id, const EntityItemProperties& properties,
        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static bool encodeEraseEntityMessage(const EntityItemID& entityItemID, 
                                            unsigned char* outputBuffer, size_t maxLength, size_t& outputLength);


    static bool decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes,
                        EntityItemID& entityID, EntityItemProperties& properties);

    bool positionChanged() const { return _positionChanged; }
    bool rotationChanged() const { return _rotationChanged; }
    bool massChanged() const { return _massChanged; }
    bool velocityChanged() const { return _velocityChanged; }
    bool gravityChanged() const { return _gravityChanged; }
    bool dampingChanged() const { return _dampingChanged; }
    bool lifetimeChanged() const { return _lifetimeChanged; }
    bool scriptChanged() const { return _scriptChanged; }
    bool dimensionsChanged() const { return _dimensionsChanged; }
    bool registrationPointChanged() const { return _registrationPointChanged; }
    bool colorChanged() const { return _colorChanged; }
    bool modelURLChanged() const { return _modelURLChanged; }
    bool animationURLChanged() const { return _animationURLChanged; }
    bool animationIsPlayingChanged() const { return _animationIsPlayingChanged; }
    bool animationFrameIndexChanged() const { return _animationFrameIndexChanged; }
    bool animationFPSChanged() const { return _animationFPSChanged; }
    bool glowLevelChanged() const { return _glowLevelChanged; }
    bool localRenderAlphaChanged() const { return _localRenderAlphaChanged; }

    void clearID() { _id = UNKNOWN_ENTITY_ID; _idSet = false; }
    void markAllChanged();

    QVector<SittingPoint> getSittingPoints() const { return _sittingPoints; }
    void setSittingPoints(QVector<SittingPoint> sittingPoints) { _sittingPoints = sittingPoints; }
    
    const glm::vec3& getNaturalDimensions() const { return _naturalDimensions; }
    void setNaturalDimensions(const glm::vec3& value) { _naturalDimensions = value; }

    const glm::vec3& getRegistrationPoint() const { return _registrationPoint; }
    void setRegistrationPoint(const glm::vec3& value) { _registrationPoint = value; _registrationPointChanged = true; }

    const glm::vec3& getAngularVelocity() const { return _angularVelocity; }
    void setAngularVelocity(const glm::vec3& value) { _angularVelocity = value; _angularVelocityChanged = true; }

    float getAngularDamping() const { return _angularDamping; }
    void setAngularDamping(float value) { _angularDamping = value; _angularDampingChanged = true; }

    bool getVisible() const { return _visible; }
    void setVisible(bool value) { _visible = value; _visibleChanged = true; }

    bool getIgnoreForCollisions() const { return _ignoreForCollisions; }
    void setIgnoreForCollisions(bool value) { _ignoreForCollisions = value; _ignoreForCollisionsChanged = true; }

    bool getCollisionsWillMove() const { return _collisionsWillMove; }
    void setCollisionsWillMove(bool value) { _collisionsWillMove = value; _collisionsWillMoveChanged = true; }

    bool getIsSpotlight() const { return _isSpotlight; }
    void setIsSpotlight(bool value) { _isSpotlight = value; _isSpotlightChanged = true; }

    void setLastEdited(quint64 usecTime) { _lastEdited = usecTime; }

private:

    QUuid _id;
    bool _idSet;
    quint64 _lastEdited;
    quint64 _created;

    EntityTypes::EntityType _type;
 
    void setType(const QString& typeName) { _type = EntityTypes::getEntityTypeFromName(typeName); }
    
    glm::vec3 _position;
    glm::vec3 _dimensions;
    glm::quat _rotation;
    float _mass;
    glm::vec3 _velocity;
    glm::vec3 _gravity;
    float _damping;
    float _lifetime;
    QString _script;
    glm::vec3 _registrationPoint;
    glm::vec3 _angularVelocity;
    float _angularDamping;
    bool _visible;
    bool _ignoreForCollisions;
    bool _collisionsWillMove;

    bool _positionChanged;
    bool _dimensionsChanged;
    bool _rotationChanged;
    bool _massChanged;
    bool _velocityChanged;
    bool _gravityChanged;
    bool _dampingChanged;
    bool _lifetimeChanged;
    bool _scriptChanged;
    bool _registrationPointChanged;
    bool _angularVelocityChanged;
    bool _angularDampingChanged;
    bool _visibleChanged;
    bool _ignoreForCollisionsChanged;
    bool _collisionsWillMoveChanged;
    
    // TODO: this need to be more generic. for now, we're going to have the properties class support these as
    // named getter/setters, but we want to move them to generic types...
    xColor _color;
    QString _modelURL;
    QString _animationURL;
    bool _animationIsPlaying;
    float _animationFrameIndex;
    float _animationFPS;
    float _glowLevel;
    float _localRenderAlpha;
    bool _isSpotlight;
    QVector<SittingPoint> _sittingPoints;
    glm::vec3 _naturalDimensions;
    

    bool _colorChanged;
    bool _modelURLChanged;
    bool _animationURLChanged;
    bool _animationIsPlayingChanged;
    bool _animationFrameIndexChanged;
    bool _animationFPSChanged;
    bool _glowLevelChanged;
    bool _localRenderAlphaChanged;
    bool _isSpotlightChanged;

    bool _defaultSettings;
};
Q_DECLARE_METATYPE(EntityItemProperties);
QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties);


inline QDebug operator<<(QDebug debug, const EntityItemProperties& properties) {
    debug << "EntityItemProperties[" << "\n"
        << "  position:" << properties.getPosition() << "in meters" << "\n"
        << "  velocity:" << properties.getVelocity() << "in meters" << "\n"
        << "  last edited:" << properties.getLastEdited() << "\n"
        << "  edited ago:" << properties.getEditedAgo() << "\n"
        << "]";

    return debug;
}

#endif // hifi_EntityItemProperties_h
