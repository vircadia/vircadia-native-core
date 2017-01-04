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
#include <NumericalConstants.h>
#include <PropertyFlags.h>
#include <OctreeConstants.h>
#include <ShapeInfo.h>

#include "AnimationPropertyGroup.h"
#include "EntityItemID.h"
#include "EntityItemPropertiesDefaults.h"
#include "EntityItemPropertiesMacros.h"
#include "EntityTypes.h"
#include "EntityPropertyFlags.h"
#include "LightEntityItem.h"
#include "LineEntityItem.h"
#include "ParticleEffectEntityItem.h"
#include "PolyVoxEntityItem.h"
#include "SimulationOwner.h"
#include "SkyboxPropertyGroup.h"
#include "StagePropertyGroup.h"
#include "TextEntityItem.h"
#include "ZoneEntityItem.h"

const quint64 UNKNOWN_CREATED_TIME = 0;

/// A collection of properties of an entity item used in the scripting API. Translates between the actual properties of an
/// entity and a JavaScript style hash/QScriptValue storing a set of properties. Used in scripting to set/get the complete
/// set of entity item properties via JavaScript hashes/QScriptValues
/// all units for SI units (meter, second, radian, etc) 
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
    friend class ShapeEntityItem; // TODO: consider removing this friend relationship and use public methods
public:
    EntityItemProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags());
    virtual ~EntityItemProperties() = default;

    void merge(const EntityItemProperties& other);

    EntityTypes::EntityType getType() const { return _type; }
    void setType(EntityTypes::EntityType type) { _type = type; }

    virtual QScriptValue copyToScriptValue(QScriptEngine* engine, bool skipDefaults) const;
    virtual void copyFromScriptValue(const QScriptValue& object, bool honorReadOnly);

    static QScriptValue entityPropertyFlagsToScriptValue(QScriptEngine* engine, const EntityPropertyFlags& flags);
    static void entityPropertyFlagsFromScriptValue(const QScriptValue& object, EntityPropertyFlags& flags);

    // editing related features supported by all entities
    quint64 getLastEdited() const { return _lastEdited; }
    float getEditedAgo() const /// Elapsed seconds since this entity was last edited
        { return (float)(usecTimestampNow() - getLastEdited()) / (float)USECS_PER_SECOND; }

    EntityPropertyFlags getChangedProperties() const;

    bool parentDependentPropertyChanged() const; // was there a changed in a property that requires parent info to interpret?
    bool parentRelatedPropertyChanged() const; // parentDependentPropertyChanged or parentID or parentJointIndex

    AABox getAABox() const;

    void debugDump() const;
    void setLastEdited(quint64 usecTime);

    // Note:  DEFINE_PROPERTY(PROP_FOO, Foo, foo, type, value) creates the following methods and variables:
    // type getFoo() const;
    // void setFoo(type);
    // bool fooChanged() const;
    // type _foo { value };
    // bool _fooChanged { false };

    DEFINE_PROPERTY(PROP_VISIBLE, Visible, visible, bool, ENTITY_ITEM_DEFAULT_VISIBLE);
    DEFINE_PROPERTY_REF_WITH_SETTER(PROP_POSITION, Position, position, glm::vec3, ENTITY_ITEM_ZERO_VEC3);
    DEFINE_PROPERTY_REF(PROP_DIMENSIONS, Dimensions, dimensions, glm::vec3, ENTITY_ITEM_DEFAULT_DIMENSIONS);
    DEFINE_PROPERTY_REF(PROP_ROTATION, Rotation, rotation, glm::quat, ENTITY_ITEM_DEFAULT_ROTATION);
    DEFINE_PROPERTY(PROP_DENSITY, Density, density, float, ENTITY_ITEM_DEFAULT_DENSITY);
    DEFINE_PROPERTY_REF(PROP_VELOCITY, Velocity, velocity, glm::vec3, ENTITY_ITEM_DEFAULT_VELOCITY);
    DEFINE_PROPERTY_REF(PROP_GRAVITY, Gravity, gravity, glm::vec3, ENTITY_ITEM_DEFAULT_GRAVITY);
    DEFINE_PROPERTY_REF(PROP_ACCELERATION, Acceleration, acceleration, glm::vec3, ENTITY_ITEM_DEFAULT_ACCELERATION);
    DEFINE_PROPERTY(PROP_DAMPING, Damping, damping, float, ENTITY_ITEM_DEFAULT_DAMPING);
    DEFINE_PROPERTY(PROP_RESTITUTION, Restitution, restitution, float, ENTITY_ITEM_DEFAULT_RESTITUTION);
    DEFINE_PROPERTY(PROP_FRICTION, Friction, friction, float, ENTITY_ITEM_DEFAULT_FRICTION);
    DEFINE_PROPERTY(PROP_LIFETIME, Lifetime, lifetime, float, ENTITY_ITEM_DEFAULT_LIFETIME);
    DEFINE_PROPERTY(PROP_CREATED, Created, created, quint64, UNKNOWN_CREATED_TIME);
    DEFINE_PROPERTY_REF(PROP_SCRIPT, Script, script, QString, ENTITY_ITEM_DEFAULT_SCRIPT);
    DEFINE_PROPERTY(PROP_SCRIPT_TIMESTAMP, ScriptTimestamp, scriptTimestamp, quint64, ENTITY_ITEM_DEFAULT_SCRIPT_TIMESTAMP);
    DEFINE_PROPERTY_REF(PROP_COLLISION_SOUND_URL, CollisionSoundURL, collisionSoundURL, QString, ENTITY_ITEM_DEFAULT_COLLISION_SOUND_URL);
    DEFINE_PROPERTY_REF(PROP_COLOR, Color, color, xColor, ParticleEffectEntityItem::DEFAULT_COLOR);
    DEFINE_PROPERTY_REF(PROP_COLOR_SPREAD, ColorSpread, colorSpread, xColor, ParticleEffectEntityItem::DEFAULT_COLOR_SPREAD);
    DEFINE_PROPERTY_REF(PROP_COLOR_START, ColorStart, colorStart, xColor, ParticleEffectEntityItem::DEFAULT_COLOR);
    DEFINE_PROPERTY_REF(PROP_COLOR_FINISH, ColorFinish, colorFinish, xColor, ParticleEffectEntityItem::DEFAULT_COLOR);
    DEFINE_PROPERTY(PROP_ALPHA, Alpha, alpha, float, ParticleEffectEntityItem::DEFAULT_ALPHA);
    DEFINE_PROPERTY(PROP_ALPHA_SPREAD, AlphaSpread, alphaSpread, float, ParticleEffectEntityItem::DEFAULT_ALPHA_SPREAD);
    DEFINE_PROPERTY(PROP_ALPHA_START, AlphaStart, alphaStart, float, ParticleEffectEntityItem::DEFAULT_ALPHA);
    DEFINE_PROPERTY(PROP_ALPHA_FINISH, AlphaFinish, alphaFinish, float, ParticleEffectEntityItem::DEFAULT_ALPHA);
    DEFINE_PROPERTY_REF(PROP_MODEL_URL, ModelURL, modelURL, QString, "");
    DEFINE_PROPERTY_REF(PROP_COMPOUND_SHAPE_URL, CompoundShapeURL, compoundShapeURL, QString, "");
    DEFINE_PROPERTY_REF(PROP_REGISTRATION_POINT, RegistrationPoint, registrationPoint, glm::vec3, ENTITY_ITEM_DEFAULT_REGISTRATION_POINT);
    DEFINE_PROPERTY_REF(PROP_ANGULAR_VELOCITY, AngularVelocity, angularVelocity, glm::vec3, ENTITY_ITEM_DEFAULT_ANGULAR_VELOCITY);
    DEFINE_PROPERTY(PROP_ANGULAR_DAMPING, AngularDamping, angularDamping, float, ENTITY_ITEM_DEFAULT_ANGULAR_DAMPING);
    DEFINE_PROPERTY(PROP_COLLISIONLESS, Collisionless, collisionless, bool, ENTITY_ITEM_DEFAULT_COLLISIONLESS);
    DEFINE_PROPERTY(PROP_COLLISION_MASK, CollisionMask, collisionMask, uint8_t, ENTITY_COLLISION_MASK_DEFAULT);
    DEFINE_PROPERTY(PROP_DYNAMIC, Dynamic, dynamic, bool, ENTITY_ITEM_DEFAULT_DYNAMIC);
    DEFINE_PROPERTY(PROP_IS_SPOTLIGHT, IsSpotlight, isSpotlight, bool, LightEntityItem::DEFAULT_IS_SPOTLIGHT);
    DEFINE_PROPERTY(PROP_INTENSITY, Intensity, intensity, float, LightEntityItem::DEFAULT_INTENSITY);
    DEFINE_PROPERTY(PROP_FALLOFF_RADIUS, FalloffRadius, falloffRadius, float, LightEntityItem::DEFAULT_FALLOFF_RADIUS);
    DEFINE_PROPERTY(PROP_EXPONENT, Exponent, exponent, float, LightEntityItem::DEFAULT_EXPONENT);
    DEFINE_PROPERTY(PROP_CUTOFF, Cutoff, cutoff, float, LightEntityItem::DEFAULT_CUTOFF);
    DEFINE_PROPERTY(PROP_LOCKED, Locked, locked, bool, ENTITY_ITEM_DEFAULT_LOCKED);
    DEFINE_PROPERTY_REF(PROP_TEXTURES, Textures, textures, QString, "");
    DEFINE_PROPERTY_REF(PROP_USER_DATA, UserData, userData, QString, ENTITY_ITEM_DEFAULT_USER_DATA);
    DEFINE_PROPERTY_REF(PROP_SIMULATION_OWNER, SimulationOwner, simulationOwner, SimulationOwner, SimulationOwner());
    DEFINE_PROPERTY_REF(PROP_TEXT, Text, text, QString, TextEntityItem::DEFAULT_TEXT);
    DEFINE_PROPERTY(PROP_LINE_HEIGHT, LineHeight, lineHeight, float, TextEntityItem::DEFAULT_LINE_HEIGHT);
    DEFINE_PROPERTY_REF(PROP_TEXT_COLOR, TextColor, textColor, xColor, TextEntityItem::DEFAULT_TEXT_COLOR);
    DEFINE_PROPERTY_REF(PROP_BACKGROUND_COLOR, BackgroundColor, backgroundColor, xColor, TextEntityItem::DEFAULT_BACKGROUND_COLOR);
    DEFINE_PROPERTY_REF_ENUM(PROP_SHAPE_TYPE, ShapeType, shapeType, ShapeType, SHAPE_TYPE_NONE);
    DEFINE_PROPERTY(PROP_MAX_PARTICLES, MaxParticles, maxParticles, quint32, ParticleEffectEntityItem::DEFAULT_MAX_PARTICLES);
    DEFINE_PROPERTY(PROP_LIFESPAN, Lifespan, lifespan, float, ParticleEffectEntityItem::DEFAULT_LIFESPAN);
    DEFINE_PROPERTY(PROP_EMITTING_PARTICLES, IsEmitting, isEmitting, bool, true);
    DEFINE_PROPERTY(PROP_EMIT_RATE, EmitRate, emitRate, float, ParticleEffectEntityItem::DEFAULT_EMIT_RATE);
    DEFINE_PROPERTY(PROP_EMIT_SPEED, EmitSpeed, emitSpeed, float, ParticleEffectEntityItem::DEFAULT_EMIT_SPEED);
    DEFINE_PROPERTY(PROP_SPEED_SPREAD, SpeedSpread, speedSpread, float, ParticleEffectEntityItem::DEFAULT_SPEED_SPREAD);
    DEFINE_PROPERTY_REF(PROP_EMIT_ORIENTATION, EmitOrientation, emitOrientation, glm::quat, ParticleEffectEntityItem::DEFAULT_EMIT_ORIENTATION);
    DEFINE_PROPERTY_REF(PROP_EMIT_DIMENSIONS, EmitDimensions, emitDimensions, glm::vec3, ParticleEffectEntityItem::DEFAULT_EMIT_DIMENSIONS);
    DEFINE_PROPERTY(PROP_EMIT_RADIUS_START, EmitRadiusStart, emitRadiusStart, float, ParticleEffectEntityItem::DEFAULT_EMIT_RADIUS_START);
    DEFINE_PROPERTY(PROP_POLAR_START, PolarStart, polarStart, float, ParticleEffectEntityItem::DEFAULT_POLAR_START);
    DEFINE_PROPERTY(PROP_POLAR_FINISH, PolarFinish, polarFinish, float, ParticleEffectEntityItem::DEFAULT_POLAR_FINISH);
    DEFINE_PROPERTY(PROP_AZIMUTH_START, AzimuthStart, azimuthStart, float, ParticleEffectEntityItem::DEFAULT_AZIMUTH_START);
    DEFINE_PROPERTY(PROP_AZIMUTH_FINISH, AzimuthFinish, azimuthFinish, float, ParticleEffectEntityItem::DEFAULT_AZIMUTH_FINISH);
    DEFINE_PROPERTY_REF(PROP_EMIT_ACCELERATION, EmitAcceleration, emitAcceleration, glm::vec3, ParticleEffectEntityItem::DEFAULT_EMIT_ACCELERATION);
    DEFINE_PROPERTY_REF(PROP_ACCELERATION_SPREAD, AccelerationSpread, accelerationSpread, glm::vec3, ParticleEffectEntityItem::DEFAULT_ACCELERATION_SPREAD);
    DEFINE_PROPERTY(PROP_PARTICLE_RADIUS, ParticleRadius, particleRadius, float, ParticleEffectEntityItem::DEFAULT_PARTICLE_RADIUS);
    DEFINE_PROPERTY(PROP_RADIUS_SPREAD, RadiusSpread, radiusSpread, float, ParticleEffectEntityItem::DEFAULT_RADIUS_SPREAD);
    DEFINE_PROPERTY(PROP_RADIUS_START, RadiusStart, radiusStart, float, ParticleEffectEntityItem::DEFAULT_RADIUS_START);
    DEFINE_PROPERTY(PROP_RADIUS_FINISH, RadiusFinish, radiusFinish, float, ParticleEffectEntityItem::DEFAULT_RADIUS_FINISH);
    DEFINE_PROPERTY(PROP_EMITTER_SHOULD_TRAIL, EmitterShouldTrail, emitterShouldTrail, bool, ParticleEffectEntityItem::DEFAULT_EMITTER_SHOULD_TRAIL);
    DEFINE_PROPERTY_REF(PROP_MARKETPLACE_ID, MarketplaceID, marketplaceID, QString, ENTITY_ITEM_DEFAULT_MARKETPLACE_ID);
    DEFINE_PROPERTY_GROUP(KeyLight, keyLight, KeyLightPropertyGroup);
    DEFINE_PROPERTY_REF(PROP_VOXEL_VOLUME_SIZE, VoxelVolumeSize, voxelVolumeSize, glm::vec3, PolyVoxEntityItem::DEFAULT_VOXEL_VOLUME_SIZE);
    DEFINE_PROPERTY_REF(PROP_VOXEL_DATA, VoxelData, voxelData, QByteArray, PolyVoxEntityItem::DEFAULT_VOXEL_DATA);
    DEFINE_PROPERTY_REF(PROP_VOXEL_SURFACE_STYLE, VoxelSurfaceStyle, voxelSurfaceStyle, uint16_t, PolyVoxEntityItem::DEFAULT_VOXEL_SURFACE_STYLE);
    DEFINE_PROPERTY_REF(PROP_NAME, Name, name, QString, ENTITY_ITEM_DEFAULT_NAME);
    DEFINE_PROPERTY_REF_ENUM(PROP_BACKGROUND_MODE, BackgroundMode, backgroundMode, BackgroundMode, BACKGROUND_MODE_INHERIT);
    DEFINE_PROPERTY_GROUP(Stage, stage, StagePropertyGroup);
    DEFINE_PROPERTY_GROUP(Skybox, skybox, SkyboxPropertyGroup);
    DEFINE_PROPERTY_GROUP(Animation, animation, AnimationPropertyGroup);
    DEFINE_PROPERTY_REF(PROP_SOURCE_URL, SourceUrl, sourceUrl, QString, "");
    DEFINE_PROPERTY(PROP_LINE_WIDTH, LineWidth, lineWidth, float, LineEntityItem::DEFAULT_LINE_WIDTH);
    DEFINE_PROPERTY_REF(LINE_POINTS, LinePoints, linePoints, QVector<glm::vec3>, QVector<glm::vec3>());
    DEFINE_PROPERTY_REF(PROP_HREF, Href, href, QString, "");
    DEFINE_PROPERTY_REF(PROP_DESCRIPTION, Description, description, QString, "");
    DEFINE_PROPERTY(PROP_FACE_CAMERA, FaceCamera, faceCamera, bool, TextEntityItem::DEFAULT_FACE_CAMERA);
    DEFINE_PROPERTY_REF(PROP_ACTION_DATA, ActionData, actionData, QByteArray, QByteArray());
    DEFINE_PROPERTY(PROP_NORMALS, Normals, normals, QVector<glm::vec3>, QVector<glm::vec3>());
    DEFINE_PROPERTY(PROP_STROKE_WIDTHS, StrokeWidths, strokeWidths, QVector<float>, QVector<float>());
    DEFINE_PROPERTY_REF(PROP_X_TEXTURE_URL, XTextureURL, xTextureURL, QString, "");
    DEFINE_PROPERTY_REF(PROP_Y_TEXTURE_URL, YTextureURL, yTextureURL, QString, "");
    DEFINE_PROPERTY_REF(PROP_Z_TEXTURE_URL, ZTextureURL, zTextureURL, QString, "");
    DEFINE_PROPERTY_REF(PROP_X_N_NEIGHBOR_ID, XNNeighborID, xNNeighborID, EntityItemID, UNKNOWN_ENTITY_ID);
    DEFINE_PROPERTY_REF(PROP_Y_N_NEIGHBOR_ID, YNNeighborID, yNNeighborID, EntityItemID, UNKNOWN_ENTITY_ID);
    DEFINE_PROPERTY_REF(PROP_Z_N_NEIGHBOR_ID, ZNNeighborID, zNNeighborID, EntityItemID, UNKNOWN_ENTITY_ID);
    DEFINE_PROPERTY_REF(PROP_X_P_NEIGHBOR_ID, XPNeighborID, xPNeighborID, EntityItemID, UNKNOWN_ENTITY_ID);
    DEFINE_PROPERTY_REF(PROP_Y_P_NEIGHBOR_ID, YPNeighborID, yPNeighborID, EntityItemID, UNKNOWN_ENTITY_ID);
    DEFINE_PROPERTY_REF(PROP_Z_P_NEIGHBOR_ID, ZPNeighborID, zPNeighborID, EntityItemID, UNKNOWN_ENTITY_ID);
    DEFINE_PROPERTY_REF(PROP_PARENT_ID, ParentID, parentID, QUuid, UNKNOWN_ENTITY_ID);
    DEFINE_PROPERTY_REF(PROP_PARENT_JOINT_INDEX, ParentJointIndex, parentJointIndex, quint16, -1);
    DEFINE_PROPERTY_REF(PROP_QUERY_AA_CUBE, QueryAACube, queryAACube, AACube, AACube());
    DEFINE_PROPERTY_REF(PROP_SHAPE, Shape, shape, QString, "Sphere");

    // these are used when bouncing location data into and out of scripts
    DEFINE_PROPERTY_REF(PROP_LOCAL_POSITION, LocalPosition, localPosition, glmVec3, ENTITY_ITEM_ZERO_VEC3);
    DEFINE_PROPERTY_REF(PROP_LOCAL_ROTATION, LocalRotation, localRotation, glmQuat, ENTITY_ITEM_DEFAULT_ROTATION);
    DEFINE_PROPERTY_REF(PROP_LOCAL_VELOCITY, LocalVelocity, localVelocity, glmVec3, ENTITY_ITEM_ZERO_VEC3);
    DEFINE_PROPERTY_REF(PROP_LOCAL_ANGULAR_VELOCITY, LocalAngularVelocity, localAngularVelocity, glmVec3, ENTITY_ITEM_ZERO_VEC3);

    DEFINE_PROPERTY_REF(PROP_JOINT_ROTATIONS_SET, JointRotationsSet, jointRotationsSet, QVector<bool>, QVector<bool>());
    DEFINE_PROPERTY_REF(PROP_JOINT_ROTATIONS, JointRotations, jointRotations, QVector<glm::quat>, QVector<glm::quat>());
    DEFINE_PROPERTY_REF(PROP_JOINT_TRANSLATIONS_SET, JointTranslationsSet, jointTranslationsSet, QVector<bool>, QVector<bool>());
    DEFINE_PROPERTY_REF(PROP_JOINT_TRANSLATIONS, JointTranslations, jointTranslations, QVector<glm::vec3>, QVector<glm::vec3>());

    DEFINE_PROPERTY(PROP_FLYING_ALLOWED, FlyingAllowed, flyingAllowed, bool, ZoneEntityItem::DEFAULT_FLYING_ALLOWED);
    DEFINE_PROPERTY(PROP_GHOSTING_ALLOWED, GhostingAllowed, ghostingAllowed, bool, ZoneEntityItem::DEFAULT_GHOSTING_ALLOWED);

    DEFINE_PROPERTY(PROP_CLIENT_ONLY, ClientOnly, clientOnly, bool, false);
    DEFINE_PROPERTY_REF(PROP_OWNING_AVATAR_ID, OwningAvatarID, owningAvatarID, QUuid, UNKNOWN_ENTITY_ID);

    DEFINE_PROPERTY_REF(PROP_DPI, DPI, dpi, uint16_t, ENTITY_ITEM_DEFAULT_DPI);

    DEFINE_PROPERTY_REF(PROP_LAST_EDITED_BY, LastEditedBy, lastEditedBy, QUuid, ENTITY_ITEM_DEFAULT_LAST_EDITED_BY);

    static QString getBackgroundModeString(BackgroundMode mode);


public:
    float getMaxDimension() const { return glm::compMax(_dimensions); }

    float getAge() const { return (float)(usecTimestampNow() - _created) / (float)USECS_PER_SECOND; }
    bool hasCreatedTime() const { return (_created != UNKNOWN_CREATED_TIME); }

    bool containsBoundsProperties() const { return (_positionChanged || _dimensionsChanged); }
    bool containsPositionChange() const { return _positionChanged; }
    bool containsDimensionsChange() const { return _dimensionsChanged; }

    float getLocalRenderAlpha() const { return _localRenderAlpha; }
    void setLocalRenderAlpha(float value) { _localRenderAlpha = value; _localRenderAlphaChanged = true; }

    static bool encodeEntityEditPacket(PacketType command, EntityItemID id, const EntityItemProperties& properties,
                                       QByteArray& buffer);

    static bool encodeEraseEntityMessage(const EntityItemID& entityItemID, QByteArray& buffer);

    static bool decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes,
                                       EntityItemID& entityID, EntityItemProperties& properties);

    bool localRenderAlphaChanged() const { return _localRenderAlphaChanged; }

    void clearID() { _id = UNKNOWN_ENTITY_ID; _idSet = false; }
    void markAllChanged();

    void setSittingPoints(const QVector<SittingPoint>& sittingPoints);

    const glm::vec3& getNaturalDimensions() const { return _naturalDimensions; }
    void setNaturalDimensions(const glm::vec3& value) { _naturalDimensions = value; }
    
    const glm::vec3& getNaturalPosition() const { return _naturalPosition; }
    void calculateNaturalPosition(const glm::vec3& min, const glm::vec3& max);
    
    const QVariantMap& getTextureNames() const { return _textureNames; }
    void setTextureNames(const QVariantMap& value) { _textureNames = value; }

    QString getSimulatorIDAsString() const { return _simulationOwner.getID().toString().mid(1,36).toUpper(); }

    void setVoxelDataDirty() { _voxelDataChanged = true; }

    void setLinePointsDirty() {_linePointsChanged = true; }

    void setQueryAACubeDirty() { _queryAACubeChanged = true; }

    void setCreated(QDateTime& v);

    bool hasTerseUpdateChanges() const;
    bool hasMiscPhysicsChanges() const;

    void clearSimulationOwner();
    void setSimulationOwner(const QUuid& id, uint8_t priority);
    void setSimulationOwner(const QByteArray& data);
    void promoteSimulationPriority(quint8 priority) { _simulationOwner.promotePriority(priority); }

    void setActionDataDirty() { _actionDataChanged = true; }

    QList<QString> listChangedProperties();

    bool getDimensionsInitialized() const { return _dimensionsInitialized; }
    void setDimensionsInitialized(bool dimensionsInitialized) { _dimensionsInitialized = dimensionsInitialized; }

    void setJointRotationsDirty() { _jointRotationsSetChanged = true; _jointRotationsChanged = true; }
    void setJointTranslationsDirty() { _jointTranslationsSetChanged = true; _jointTranslationsChanged = true; }

    // render info related items
    size_t getRenderInfoVertexCount() const { return _renderInfoVertexCount; }
    void setRenderInfoVertexCount(size_t value) { _renderInfoVertexCount = value; }
    int getRenderInfoTextureCount() const { return _renderInfoTextureCount; }
    void setRenderInfoTextureCount(int value) { _renderInfoTextureCount = value; }
    size_t getRenderInfoTextureSize() const { return _renderInfoTextureSize; }
    void setRenderInfoTextureSize(size_t value) { _renderInfoTextureSize = value; }
    int getRenderInfoDrawCalls() const { return _renderInfoDrawCalls; }
    void setRenderInfoDrawCalls(int value) { _renderInfoDrawCalls = value; }
    bool getRenderInfoHasTransparent() const { return _renderInfoHasTransparent; }
    void setRenderInfoHasTransparent(bool value) { _renderInfoHasTransparent = value; }


protected:
    QString getCollisionMaskAsString() const;
    void setCollisionMaskFromString(const QString& maskString);

private:
    QUuid _id;
    bool _idSet;
    quint64 _lastEdited;
    EntityTypes::EntityType _type;
    void setType(const QString& typeName) { _type = EntityTypes::getEntityTypeFromName(typeName); }

    float _localRenderAlpha;
    bool _localRenderAlphaChanged;
    bool _defaultSettings;
    bool _dimensionsInitialized = true; // Only false if creating an entity locally with no dimensions properties

    // NOTE: The following are pseudo client only properties. They are only used in clients which can access
    // properties of model geometry. But these properties are not serialized like other properties.
    QVector<SittingPoint> _sittingPoints;
    QVariantMap _textureNames;
    glm::vec3 _naturalDimensions;
    glm::vec3 _naturalPosition;

    size_t _renderInfoVertexCount { 0 };
    int _renderInfoTextureCount { 0 };
    size_t _renderInfoTextureSize { 0 };
    int _renderInfoDrawCalls { 0 };
    bool _renderInfoHasTransparent { false };

    EntityPropertyFlags _desiredProperties; // if set will narrow scopes of copy/to/from to just these properties
};

Q_DECLARE_METATYPE(EntityItemProperties);
QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
QScriptValue EntityItemNonDefaultPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValueIgnoreReadOnly(const QScriptValue& object, EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValueHonorReadOnly(const QScriptValue& object, EntityItemProperties& properties);

Q_DECLARE_METATYPE(EntityPropertyFlags);
QScriptValue EntityPropertyFlagsToScriptValue(QScriptEngine* engine, const EntityPropertyFlags& flags);
void EntityPropertyFlagsFromScriptValue(const QScriptValue& object, EntityPropertyFlags& flags);


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
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Created, created, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Lifetime, lifetime, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Script, script, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ScriptTimestamp, scriptTimestamp, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, CollisionSoundURL, collisionSoundURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Color, color, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ColorSpread, colorSpread, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ColorStart, colorStart, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ColorFinish, colorFinish, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Alpha, alpha, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AlphaSpread, alphaSpread, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AlphaStart, alphaStart, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AlphaFinish, alphaFinish, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ModelURL, modelURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, CompoundShapeURL, compoundShapeURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, RegistrationPoint, registrationPoint, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AngularVelocity, angularVelocity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AngularDamping, angularDamping, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Collisionless, collisionless, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Dynamic, dynamic, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, IsSpotlight, isSpotlight, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Intensity, intensity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, FalloffRadius, falloffRadius, "");
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
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, IsEmitting, isEmitting, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitRate, emitRate, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitSpeed, emitSpeed, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, SpeedSpread, speedSpread, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitOrientation, emitOrientation, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitDimensions, emitDimensions, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitRadiusStart, emitRadiusStart, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, PolarStart, polarStart, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, PolarFinish, polarFinish, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AzimuthStart, azimuthStart, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AzimuthFinish, azimuthFinish, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitAcceleration, emitAcceleration, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AccelerationSpread, accelerationSpread, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ParticleRadius, particleRadius, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, RadiusSpread, radiusSpread, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, RadiusStart, radiusStart, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, RadiusFinish, radiusFinish, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, MarketplaceID, marketplaceID, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, BackgroundMode, backgroundMode, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, VoxelVolumeSize, voxelVolumeSize, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, VoxelData, voxelData, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, VoxelSurfaceStyle, voxelSurfaceStyle, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Href, href, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Description, description, "");
    if (properties.actionDataChanged()) {
        debug << " " << "actionData" << ":" << properties.getActionData().toHex() << "" << "\n";
    }
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, XTextureURL, xTextureURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, YTextureURL, yTextureURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ZTextureURL, zTextureURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, XNNeighborID, xNNeighborID, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, YNNeighborID, yNNeighborID, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ZNNeighborID, zNNeighborID, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, XPNeighborID, xPNeighborID, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, YPNeighborID, yPNeighborID, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ZPNeighborID, zPNeighborID, "");

    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ParentID, parentID, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ParentJointIndex, parentJointIndex, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, QueryAACube, queryAACube, "");

    DEBUG_PROPERTY_IF_CHANGED(debug, properties, JointRotationsSet, jointRotationsSet, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, JointRotations, jointRotations, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, JointTranslationsSet, jointTranslationsSet, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, JointTranslations, jointTranslations, "");

    DEBUG_PROPERTY_IF_CHANGED(debug, properties, FlyingAllowed, flyingAllowed, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, GhostingAllowed, ghostingAllowed, "");

    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ClientOnly, clientOnly, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, OwningAvatarID, owningAvatarID, "");

    DEBUG_PROPERTY_IF_CHANGED(debug, properties, LastEditedBy, lastEditedBy, "");

    properties.getAnimation().debugDump();
    properties.getSkybox().debugDump();
    properties.getStage().debugDump();

    debug << "  last edited:" << properties.getLastEdited() << "\n";
    debug << "  edited ago:" << properties.getEditedAgo() << "\n";
    debug << "]";

    return debug;
}

#endif // hifi_EntityItemProperties_h
