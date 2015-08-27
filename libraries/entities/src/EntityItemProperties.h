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
#include <NumericalConstants.h>
#include <PropertyFlags.h>
#include <OctreeConstants.h>
#include <ShapeInfo.h>

#include "AtmospherePropertyGroup.h"
#include "EntityItemID.h"
#include "EntityItemPropertiesMacros.h"
#include "EntityTypes.h"
#include "EntityPropertyFlags.h"
#include "SimulationOwner.h"
#include "SkyboxPropertyGroup.h"
#include "StagePropertyGroup.h"

const quint64 UNKNOWN_CREATED_TIME = 0;

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
    friend class ParticleEffectEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class ZoneEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class WebEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class LineEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class PolyVoxEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class PolyLineEntityItem; // TODO: consider removing this friend relationship and use public methods
public:
    EntityItemProperties();
    virtual ~EntityItemProperties();

    EntityTypes::EntityType getType() const { return _type; }
    void setType(EntityTypes::EntityType type) { _type = type; }

    virtual QScriptValue copyToScriptValue(QScriptEngine* engine, bool skipDefaults) const;
    virtual void copyFromScriptValue(const QScriptValue& object, bool honorReadOnly);

    // editing related features supported by all entities
    quint64 getLastEdited() const { return _lastEdited; }
    float getEditedAgo() const /// Elapsed seconds since this entity was last edited
        { return (float)(usecTimestampNow() - getLastEdited()) / (float)USECS_PER_SECOND; }
    EntityPropertyFlags getChangedProperties() const;

    AACube getMaximumAACube() const;
    AABox getAABox() const;

    void debugDump() const;
    void setLastEdited(quint64 usecTime);

    // Note:  DEFINE_PROPERTY(PROP_FOO, Foo, foo, type) creates the following methods and variables:
    // type getFoo() const;
    // void setFoo(type);
    // bool fooChanged() const;
    // type _foo;
    // bool _fooChanged;

    DEFINE_PROPERTY(PROP_VISIBLE, Visible, visible, bool);
    DEFINE_PROPERTY_REF_WITH_SETTER(PROP_POSITION, Position, position, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_DIMENSIONS, Dimensions, dimensions, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_ROTATION, Rotation, rotation, glm::quat);
    DEFINE_PROPERTY(PROP_DENSITY, Density, density, float);
    DEFINE_PROPERTY_REF(PROP_VELOCITY, Velocity, velocity, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_GRAVITY, Gravity, gravity, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_ACCELERATION, Acceleration, acceleration, glm::vec3);
    DEFINE_PROPERTY(PROP_DAMPING, Damping, damping, float);
    DEFINE_PROPERTY(PROP_RESTITUTION, Restitution, restitution, float);
    DEFINE_PROPERTY(PROP_FRICTION, Friction, friction, float);
    DEFINE_PROPERTY(PROP_LIFETIME, Lifetime, lifetime, float);
    DEFINE_PROPERTY(PROP_CREATED, Created, created, quint64);
    DEFINE_PROPERTY_REF(PROP_SCRIPT, Script, script, QString);
    DEFINE_PROPERTY(PROP_SCRIPT_TIMESTAMP, ScriptTimestamp, scriptTimestamp, quint64);
    DEFINE_PROPERTY_REF(PROP_COLLISION_SOUND_URL, CollisionSoundURL, collisionSoundURL, QString);
    DEFINE_PROPERTY_REF(PROP_COLOR, Color, color, xColor);
    DEFINE_PROPERTY_REF(PROP_MODEL_URL, ModelURL, modelURL, QString);
    DEFINE_PROPERTY_REF(PROP_COMPOUND_SHAPE_URL, CompoundShapeURL, compoundShapeURL, QString);
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
    DEFINE_PROPERTY(PROP_INTENSITY, Intensity, intensity, float);
    DEFINE_PROPERTY(PROP_EXPONENT, Exponent, exponent, float);
    DEFINE_PROPERTY(PROP_CUTOFF, Cutoff, cutoff, float);
    DEFINE_PROPERTY(PROP_LOCKED, Locked, locked, bool);
    DEFINE_PROPERTY_REF(PROP_TEXTURES, Textures, textures, QString);
    DEFINE_PROPERTY_REF_WITH_SETTER_AND_GETTER(PROP_ANIMATION_SETTINGS, AnimationSettings, animationSettings, QString);
    DEFINE_PROPERTY_REF(PROP_USER_DATA, UserData, userData, QString);
    DEFINE_PROPERTY_REF(PROP_SIMULATION_OWNER, SimulationOwner, simulationOwner, SimulationOwner);
    DEFINE_PROPERTY_REF(PROP_TEXT, Text, text, QString);
    DEFINE_PROPERTY(PROP_LINE_HEIGHT, LineHeight, lineHeight, float);
    DEFINE_PROPERTY_REF(PROP_TEXT_COLOR, TextColor, textColor, xColor);
    DEFINE_PROPERTY_REF(PROP_BACKGROUND_COLOR, BackgroundColor, backgroundColor, xColor);
    DEFINE_PROPERTY_REF_ENUM(PROP_SHAPE_TYPE, ShapeType, shapeType, ShapeType);
    DEFINE_PROPERTY(PROP_MAX_PARTICLES, MaxParticles, maxParticles, quint32);
    DEFINE_PROPERTY(PROP_LIFESPAN, Lifespan, lifespan, float);
    DEFINE_PROPERTY(PROP_EMIT_RATE, EmitRate, emitRate, float);
    DEFINE_PROPERTY_REF(PROP_EMIT_VELOCITY, EmitVelocity, emitVelocity, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_VELOCITY_SPREAD, VelocitySpread, velocitySpread, glm::vec3);
    DEFINE_PROPERTY(PROP_EMIT_ACCELERATION, EmitAcceleration, emitAcceleration, glm::vec3);
    DEFINE_PROPERTY(PROP_ACCELERATION_SPREAD, AccelerationSpread, accelerationSpread, glm::vec3);
    DEFINE_PROPERTY(PROP_PARTICLE_RADIUS, ParticleRadius, particleRadius, float);
    DEFINE_PROPERTY_REF(PROP_MARKETPLACE_ID, MarketplaceID, marketplaceID, QString);
    DEFINE_PROPERTY_REF(PROP_KEYLIGHT_COLOR, KeyLightColor, keyLightColor, xColor);
    DEFINE_PROPERTY(PROP_KEYLIGHT_INTENSITY, KeyLightIntensity, keyLightIntensity, float);
    DEFINE_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, KeyLightAmbientIntensity, keyLightAmbientIntensity, float);
    DEFINE_PROPERTY_REF(PROP_KEYLIGHT_DIRECTION, KeyLightDirection, keyLightDirection, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_VOXEL_VOLUME_SIZE, VoxelVolumeSize, voxelVolumeSize, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_VOXEL_DATA, VoxelData, voxelData, QByteArray);
    DEFINE_PROPERTY_REF(PROP_VOXEL_SURFACE_STYLE, VoxelSurfaceStyle, voxelSurfaceStyle, uint16_t);
    DEFINE_PROPERTY_REF(PROP_NAME, Name, name, QString);
    DEFINE_PROPERTY_REF_ENUM(PROP_BACKGROUND_MODE, BackgroundMode, backgroundMode, BackgroundMode);
    DEFINE_PROPERTY_GROUP(Stage, stage, StagePropertyGroup);
    DEFINE_PROPERTY_GROUP(Atmosphere, atmosphere, AtmospherePropertyGroup);
    DEFINE_PROPERTY_GROUP(Skybox, skybox, SkyboxPropertyGroup);
    DEFINE_PROPERTY_REF(PROP_SOURCE_URL, SourceUrl, sourceUrl, QString);
    DEFINE_PROPERTY(PROP_LINE_WIDTH, LineWidth, lineWidth, float);
    DEFINE_PROPERTY_REF(LINE_POINTS, LinePoints, linePoints, QVector<glm::vec3>);
    DEFINE_PROPERTY_REF(PROP_HREF, Href, href, QString);
    DEFINE_PROPERTY_REF(PROP_DESCRIPTION, Description, description, QString);
    DEFINE_PROPERTY(PROP_FACE_CAMERA, FaceCamera, faceCamera, bool);
    DEFINE_PROPERTY_REF(PROP_ACTION_DATA, ActionData, actionData, QByteArray);
    DEFINE_PROPERTY(PROP_NORMALS, Normals, normals, QVector<glm::vec3>);
    DEFINE_PROPERTY(PROP_STROKE_WIDTHS, StrokeWidths, strokeWidths, QVector<float>);
    DEFINE_PROPERTY_REF(PROP_X_TEXTURE_URL, XTextureURL, xTextureURL, QString);
    DEFINE_PROPERTY_REF(PROP_Y_TEXTURE_URL, YTextureURL, yTextureURL, QString);
    DEFINE_PROPERTY_REF(PROP_Z_TEXTURE_URL, ZTextureURL, zTextureURL, QString);

    static QString getBackgroundModeString(BackgroundMode mode);


public:
    float getMaxDimension() const { return glm::max(_dimensions.x, _dimensions.y, _dimensions.z); }

    float getAge() const { return (float)(usecTimestampNow() - _created) / (float)USECS_PER_SECOND; }
    bool hasCreatedTime() const { return (_created != UNKNOWN_CREATED_TIME); }

    bool containsBoundsProperties() const { return (_positionChanged || _dimensionsChanged); }
    bool containsPositionChange() const { return _positionChanged; }
    bool containsDimensionsChange() const { return _dimensionsChanged; }
    bool containsAnimationSettingsChange() const { return _animationSettingsChanged; }

    float getGlowLevel() const { return _glowLevel; }
    float getLocalRenderAlpha() const { return _localRenderAlpha; }
    void setGlowLevel(float value) { _glowLevel = value; _glowLevelChanged = true; }
    void setLocalRenderAlpha(float value) { _localRenderAlpha = value; _localRenderAlphaChanged = true; }

    static bool encodeEntityEditPacket(PacketType command, EntityItemID id, const EntityItemProperties& properties,
                                       QByteArray& buffer);

    static bool encodeEraseEntityMessage(const EntityItemID& entityItemID, QByteArray& buffer);

    static bool decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes,
                                       EntityItemID& entityID, EntityItemProperties& properties);

    bool glowLevelChanged() const { return _glowLevelChanged; }
    bool localRenderAlphaChanged() const { return _localRenderAlphaChanged; }

    void clearID() { _id = UNKNOWN_ENTITY_ID; _idSet = false; }
    void markAllChanged();

    void setSittingPoints(const QVector<SittingPoint>& sittingPoints);

    const glm::vec3& getNaturalDimensions() const { return _naturalDimensions; }
    void setNaturalDimensions(const glm::vec3& value) { _naturalDimensions = value; }
    
    const glm::vec3& getNaturalPosition() const { return _naturalPosition; }
    void calculateNaturalPosition(const glm::vec3& min, const glm::vec3& max);
    
    const QStringList& getTextureNames() const { return _textureNames; }
    void setTextureNames(const QStringList& value) { _textureNames = value; }

    QString getSimulatorIDAsString() const { return _simulationOwner.getID().toString().mid(1,36).toUpper(); }

    void setVoxelDataDirty() { _voxelDataChanged = true; }

    void setLinePointsDirty() {_linePointsChanged = true; }

    void setCreated(QDateTime& v);

    bool hasTerseUpdateChanges() const;
    bool hasMiscPhysicsChanges() const;

    void clearSimulationOwner();
    void setSimulationOwner(const QUuid& id, uint8_t priority);
    void setSimulationOwner(const QByteArray& data);
    void promoteSimulationPriority(quint8 priority) { _simulationOwner.promotePriority(priority); }

    void setActionDataDirty() { _actionDataChanged = true; }

private:
    QUuid _id;
    bool _idSet;
    quint64 _lastEdited;
    EntityTypes::EntityType _type;
    void setType(const QString& typeName) { _type = EntityTypes::getEntityTypeFromName(typeName); }

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
    glm::vec3 _naturalPosition;
};

Q_DECLARE_METATYPE(EntityItemProperties);
QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
QScriptValue EntityItemNonDefaultPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValueIgnoreReadOnly(const QScriptValue &object, EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValueHonorReadOnly(const QScriptValue &object, EntityItemProperties& properties);


// define these inline here so the macros work
inline void EntityItemProperties::setPosition(const glm::vec3& value)
                    { _position = glm::clamp(value, (float)-HALF_TREE_SCALE, (float)HALF_TREE_SCALE); _positionChanged = true; }

inline QDebug operator<<(QDebug debug, const EntityItemProperties& properties) {
    debug << "EntityItemProperties[" << "\n";

    debug << "    _type:" << properties.getType() << "\n";

    // TODO: figure out why position and animationSettings don't seem to like the macro approach
    if (properties.containsPositionChange()) {
        debug << "  position:" << properties.getPosition() << "in meters" << "\n";
    }
    if (properties.containsAnimationSettingsChange()) {
        debug << "  animationSettings:" << properties.getAnimationSettings() << "\n";
    }

    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Dimensions, dimensions, "in meters");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Velocity, velocity, "in meters");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Name, name, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Visible, visible, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Rotation, rotation, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Density, density, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Gravity, gravity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Acceleration, acceleration, "in meters per second");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Damping, damping, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Restitution, restitution, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Friction, friction, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Lifetime, lifetime, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Script, script, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ScriptTimestamp, scriptTimestamp, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, CollisionSoundURL, collisionSoundURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Color, color, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ModelURL, modelURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, CompoundShapeURL, compoundShapeURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AnimationURL, animationURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AnimationFPS, animationFPS, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AnimationFrameIndex, animationFrameIndex, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AnimationIsPlaying, animationIsPlaying, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, RegistrationPoint, registrationPoint, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AngularVelocity, angularVelocity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AngularDamping, angularDamping, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, IgnoreForCollisions, ignoreForCollisions, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, CollisionsWillMove, collisionsWillMove, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, IsSpotlight, isSpotlight, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Intensity, intensity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Exponent, exponent, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Cutoff, cutoff, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Locked, locked, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Textures, textures, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, UserData, userData, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, SimulationOwner, simulationOwner, SimulationOwner());
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Text, text, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, LineHeight, lineHeight, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, TextColor, textColor, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, BackgroundColor, backgroundColor, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ShapeType, shapeType, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, MaxParticles, maxParticles, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Lifespan, lifespan, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitRate, emitRate, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitVelocity, emitVelocity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitAcceleration, emitAcceleration, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AccelerationSpread, accelerationSpread, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ParticleRadius, particleRadius, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, MarketplaceID, marketplaceID, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, BackgroundMode, backgroundMode, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, VoxelVolumeSize, voxelVolumeSize, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, VoxelData, voxelData, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, VoxelSurfaceStyle, voxelSurfaceStyle, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Href, href, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Description, description, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ActionData, actionData, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, XTextureURL, xTextureURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, YTextureURL, yTextureURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ZTextureURL, zTextureURL, "");

    properties.getStage().debugDump();
    properties.getAtmosphere().debugDump();
    properties.getSkybox().debugDump();

    debug << "  last edited:" << properties.getLastEdited() << "\n";
    debug << "  edited ago:" << properties.getEditedAgo() << "\n";
    debug << "]";

    return debug;
}

#endif // hifi_EntityItemProperties_h
