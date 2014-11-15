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
    PROP_DIFFUSE_COLOR,
    PROP_AMBIENT_COLOR,
    PROP_SPECULAR_COLOR,
    PROP_CONSTANT_ATTENUATION,
    PROP_LINEAR_ATTENUATION,
    PROP_QUADRATIC_ATTENUATION,
    PROP_EXPONENT,
    PROP_CUTOFF,

    // available to all entities
    PROP_LOCKED,
    
    // used by Model entities
    PROP_TEXTURES,
    PROP_ANIMATION_SETTINGS,
    PROP_USER_DATA,

    PROP_LAST_ITEM = PROP_USER_DATA,

    // These properties of TextEntity piggy back off of properties of ModelEntities, the type doesn't matter
    // since the derived class knows how to interpret it's own properties and knows the types it expects
    PROP_TEXT_COLOR = PROP_COLOR,
    PROP_TEXT = PROP_MODEL_URL,
    PROP_LINE_HEIGHT = PROP_ANIMATION_URL,
    PROP_BACKGROUND_COLOR = PROP_ANIMATION_FPS,
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
    friend class TextEntityItem; // TODO: consider removing this friend relationship and use public methods
private:
    QUuid _id;
    bool _idSet;
    quint64 _lastEdited;
    quint64 _created;
    EntityTypes::EntityType _type;
    void setType(const QString& typeName) { _type = EntityTypes::getEntityTypeFromName(typeName); }


public:
    EntityItemProperties();
    virtual ~EntityItemProperties();

    EntityTypes::EntityType getType() const { return _type; }
    void setType(EntityTypes::EntityType type) { _type = type; }

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
    void setLastEdited(quint64 usecTime) { _lastEdited = usecTime; }

    DEFINE_PROPERTY(PROP_VISIBLE, Visible, visible, bool);
    DEFINE_PROPERTY_REF_WITH_SETTER(PROP_POSITION, Position, position, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_DIMENSIONS, Dimensions, dimensions, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_ROTATION, Rotation, rotation, glm::quat);
    DEFINE_PROPERTY(PROP_MASS, Mass, mass, float);
    DEFINE_PROPERTY_REF(PROP_VELOCITY, Velocity, velocity, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_GRAVITY, Gravity, gravity, glm::vec3);
    DEFINE_PROPERTY(PROP_DAMPING, Damping, damping, float);
    DEFINE_PROPERTY(PROP_LIFETIME, Lifetime, lifetime, float);
    DEFINE_PROPERTY_REF(PROP_SCRIPT, Script, script, QString);
    DEFINE_PROPERTY_REF(PROP_COLOR, Color, color, xColor);
    DEFINE_PROPERTY_REF(PROP_MODEL_URL, ModelURL, modelURL, QString);
    DEFINE_PROPERTY_REF(PROP_ANIMATION_URL, AnimationURL, animationURL, QString);
    DEFINE_PROPERTY(PROP_ANIMATION_FPS, AnimationFPS, animationFPS, float);
    DEFINE_PROPERTY(PROP_ANIMATION_FRAME_INDEX, AnimationFrameIndex, animationFrameIndex, float);
    DEFINE_PROPERTY(PROP_ANIMATION_PLAYING, AnimationIsPlaying, animationIsPlaying, bool);
    DEFINE_PROPERTY_REF(PROP_REGISTRATION_POINT, RegistrationPoint, registrationPoint, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_ANGULAR_VELOCITY, AngularVelocity, angularVelocity, glm::vec3);
    DEFINE_PROPERTY(PROP_ANGULAR_DAMPING, AngularDamping, angularDamping, float);
    DEFINE_PROPERTY(PROP_IGNORE_FOR_COLLISIONS, IgnoreForCollisions, ignoreForCollisions, bool);
    DEFINE_PROPERTY(PROP_COLLISIONS_WILL_MOVE, CollisionsWillMove, collisionsWillMove, bool);
    DEFINE_PROPERTY(PROP_IS_SPOTLIGHT, IsSpotlight, isSpotlight, bool);
    DEFINE_PROPERTY_REF(PROP_DIFFUSE_COLOR, DiffuseColor, diffuseColor, xColor);
    DEFINE_PROPERTY_REF(PROP_AMBIENT_COLOR, AmbientColor, ambientColor, xColor);
    DEFINE_PROPERTY_REF(PROP_SPECULAR_COLOR, SpecularColor, specularColor, xColor);
    DEFINE_PROPERTY(PROP_CONSTANT_ATTENUATION, ConstantAttenuation, constantAttenuation, float);
    DEFINE_PROPERTY(PROP_LINEAR_ATTENUATION, LinearAttenuation, linearAttenuation, float);
    DEFINE_PROPERTY(PROP_QUADRATIC_ATTENUATION, QuadraticAttenuation, quadraticAttenuation, float);
    DEFINE_PROPERTY(PROP_EXPONENT, Exponent, exponent, float);
    DEFINE_PROPERTY(PROP_CUTOFF, Cutoff, cutoff, float);
    DEFINE_PROPERTY(PROP_LOCKED, Locked, locked, bool);
    DEFINE_PROPERTY_REF(PROP_TEXTURES, Textures, textures, QString);
    DEFINE_PROPERTY_REF_WITH_SETTER_AND_GETTER(PROP_ANIMATION_SETTINGS, AnimationSettings, animationSettings, QString);
    DEFINE_PROPERTY_REF(PROP_USER_DATA, UserData, userData, QString);
    DEFINE_PROPERTY_REF(PROP_TEXT, Text, text, QString);
    DEFINE_PROPERTY(PROP_LINE_HEIGHT, LineHeight, lineHeight, float);
    DEFINE_PROPERTY_REF(PROP_TEXT_COLOR, TextColor, textColor, xColor);
    DEFINE_PROPERTY_REF(PROP_BACKGROUND_COLOR, BackgroundColor, backgroundColor, xColor);

public:
    float getMaxDimension() const { return glm::max(_dimensions.x, _dimensions.y, _dimensions.z); }

    float getAge() const { return (float)(usecTimestampNow() - _created) / (float)USECS_PER_SECOND; }
    quint64 getCreated() const { return _created; }
    void setCreated(quint64 usecTime) { _created = usecTime; }
    bool hasCreatedTime() const { return (_created != UNKNOWN_CREATED_TIME); }

    bool containsBoundsProperties() const { return (_positionChanged || _dimensionsChanged); }
    bool containsPositionChange() const { return _positionChanged; }
    bool containsDimensionsChange() const { return _dimensionsChanged; }

    float getGlowLevel() const { return _glowLevel; }
    float getLocalRenderAlpha() const { return _localRenderAlpha; }
    void setGlowLevel(float value) { _glowLevel = value; _glowLevelChanged = true; }
    void setLocalRenderAlpha(float value) { _localRenderAlpha = value; _localRenderAlphaChanged = true; }

    static bool encodeEntityEditPacket(PacketType command, EntityItemID id, const EntityItemProperties& properties,
        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static bool encodeEraseEntityMessage(const EntityItemID& entityItemID, 
                                            unsigned char* outputBuffer, size_t maxLength, size_t& outputLength);


    static bool decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes,
                        EntityItemID& entityID, EntityItemProperties& properties);

    bool glowLevelChanged() const { return _glowLevelChanged; }
    bool localRenderAlphaChanged() const { return _localRenderAlphaChanged; }

    void clearID() { _id = UNKNOWN_ENTITY_ID; _idSet = false; }
    void markAllChanged();

    void setSittingPoints(const QVector<SittingPoint>& sittingPoints);

    const glm::vec3& getNaturalDimensions() const { return _naturalDimensions; }
    void setNaturalDimensions(const glm::vec3& value) { _naturalDimensions = value; }

    const QStringList& getTextureNames() const { return _textureNames; }
    void setTextureNames(const QStringList& value) { _textureNames = value; }


private:
    float _glowLevel;
    float _localRenderAlpha;
    bool _glowLevelChanged;
    bool _localRenderAlphaChanged;
    bool _defaultSettings;

    // NOTE: The following are pseudo client only properties. They are only used in clients which can access
    // properties of model geometry. But these properties are not serialized like other properties.
    QVector<SittingPoint> _sittingPoints;
    QStringList _textureNames;
    glm::vec3 _naturalDimensions;
};
Q_DECLARE_METATYPE(EntityItemProperties);
QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties);


// define these inline here so the macros work
inline void EntityItemProperties::setPosition(const glm::vec3& value) 
                    { _position = glm::clamp(value, 0.0f, (float)TREE_SCALE); _positionChanged = true; }


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
