//
//  EntityItemProperties.cpp
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <QHash>
#include <QObject>
#include <QtCore/QJsonDocument>

#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <RegisteredMetaTypes.h>

#include "EntitiesLogging.h"
#include "EntityItem.h"
#include "EntityItemProperties.h"
#include "ModelEntityItem.h"
#include "PolyLineEntityItem.h"

AnimationPropertyGroup EntityItemProperties::_staticAnimation;
SkyboxPropertyGroup EntityItemProperties::_staticSkybox;
StagePropertyGroup EntityItemProperties::_staticStage;
KeyLightPropertyGroup EntityItemProperties::_staticKeyLight;

EntityPropertyList PROP_LAST_ITEM = (EntityPropertyList)(PROP_AFTER_LAST_ITEM - 1);

EntityItemProperties::EntityItemProperties(EntityPropertyFlags desiredProperties) :

_id(UNKNOWN_ENTITY_ID),
_idSet(false),
_lastEdited(0),
_type(EntityTypes::Unknown),

_localRenderAlpha(1.0f),

_localRenderAlphaChanged(false),

_defaultSettings(true),
_naturalDimensions(1.0f, 1.0f, 1.0f),
_naturalPosition(0.0f, 0.0f, 0.0f),
_desiredProperties(desiredProperties)
{
}

void EntityItemProperties::setSittingPoints(const QVector<SittingPoint>& sittingPoints) {
    _sittingPoints.clear();
    foreach (SittingPoint sitPoint, sittingPoints) {
        _sittingPoints.append(sitPoint);
    }
}

void EntityItemProperties::calculateNaturalPosition(const glm::vec3& min, const glm::vec3& max) {
    glm::vec3 halfDimension = (max - min) / 2.0f;
    _naturalPosition = max - halfDimension;
}

void EntityItemProperties::setCreated(QDateTime &v) {
    _created = v.toMSecsSinceEpoch() * 1000; // usec per msec
}

void EntityItemProperties::debugDump() const {
    qCDebug(entities) << "EntityItemProperties...";
    qCDebug(entities) << "    _type=" << EntityTypes::getEntityTypeName(_type);
    qCDebug(entities) << "   _id=" << _id;
    qCDebug(entities) << "   _idSet=" << _idSet;
    qCDebug(entities) << "   _position=" << _position.x << "," << _position.y << "," << _position.z;
    qCDebug(entities) << "   _dimensions=" << getDimensions();
    qCDebug(entities) << "   _modelURL=" << _modelURL;
    qCDebug(entities) << "   _compoundShapeURL=" << _compoundShapeURL;

    getAnimation().debugDump();
    getSkybox().debugDump();
    getKeyLight().debugDump();

    qCDebug(entities) << "   changed properties...";
    EntityPropertyFlags props = getChangedProperties();
    props.debugDumpBits();
}

void EntityItemProperties::setLastEdited(quint64 usecTime) {
    _lastEdited = usecTime > _created ? usecTime : _created;
}

const char* shapeTypeNames[] = {
    "none",
    "box",
    "sphere",
    "capsule-x",
    "capsule-y",
    "capsule-z",
    "cylinder-x",
    "cylinder-y",
    "cylinder-z",
    "hull",
    "plane",
    "compound",
    "simple-hull",
    "simple-compound",
    "static-mesh"
};

QHash<QString, ShapeType> stringToShapeTypeLookup;

void addShapeType(ShapeType type) {
    stringToShapeTypeLookup[shapeTypeNames[type]] = type;
}

void buildStringToShapeTypeLookup() {
    addShapeType(SHAPE_TYPE_NONE);
    addShapeType(SHAPE_TYPE_BOX);
    addShapeType(SHAPE_TYPE_SPHERE);
    addShapeType(SHAPE_TYPE_CAPSULE_X);
    addShapeType(SHAPE_TYPE_CAPSULE_Y);
    addShapeType(SHAPE_TYPE_CAPSULE_Z);
    addShapeType(SHAPE_TYPE_CYLINDER_X);
    addShapeType(SHAPE_TYPE_CYLINDER_Y);
    addShapeType(SHAPE_TYPE_CYLINDER_Z);
    addShapeType(SHAPE_TYPE_HULL);
    addShapeType(SHAPE_TYPE_PLANE);
    addShapeType(SHAPE_TYPE_COMPOUND);
    addShapeType(SHAPE_TYPE_SIMPLE_HULL);
    addShapeType(SHAPE_TYPE_SIMPLE_COMPOUND);
    addShapeType(SHAPE_TYPE_STATIC_MESH);
}

QString getCollisionGroupAsString(uint8_t group) {
    switch (group) {
        case USER_COLLISION_GROUP_DYNAMIC:
            return "dynamic";
        case USER_COLLISION_GROUP_STATIC:
            return "static";
        case USER_COLLISION_GROUP_KINEMATIC:
            return "kinematic";
        case USER_COLLISION_GROUP_MY_AVATAR:
            return "myAvatar";
        case USER_COLLISION_GROUP_OTHER_AVATAR:
            return "otherAvatar";
    };
    return "";
}

uint8_t getCollisionGroupAsBitMask(const QStringRef& name) {
    if (0 == name.compare("dynamic")) {
        return USER_COLLISION_GROUP_DYNAMIC;
    } else if (0 == name.compare("static")) {
        return USER_COLLISION_GROUP_STATIC;
    } else if (0 == name.compare("kinematic")) {
        return USER_COLLISION_GROUP_KINEMATIC;
    } else if (0 == name.compare("myAvatar")) {
        return USER_COLLISION_GROUP_MY_AVATAR;
    } else if (0 == name.compare("otherAvatar")) {
        return USER_COLLISION_GROUP_OTHER_AVATAR;
    }
    return 0;
}

QString EntityItemProperties::getCollisionMaskAsString() const {
    QString maskString("");
    for (int i = 0; i < NUM_USER_COLLISION_GROUPS; ++i) {
        uint8_t group = 0x01 << i;
        if (group & _collisionMask) {
            maskString.append(getCollisionGroupAsString(group));
            maskString.append(',');
        }
    }
    return maskString;
}

void EntityItemProperties::setCollisionMaskFromString(const QString& maskString) {
    QVector<QStringRef> groups = maskString.splitRef(',');
    uint8_t mask = 0x00;
    for (auto groupName : groups) {
        mask |= getCollisionGroupAsBitMask(groupName);
    }
    _collisionMask = mask;
    _collisionMaskChanged = true;
}

QString EntityItemProperties::getShapeTypeAsString() const {
    if (_shapeType < sizeof(shapeTypeNames) / sizeof(char *))
        return QString(shapeTypeNames[_shapeType]);
    return QString(shapeTypeNames[SHAPE_TYPE_NONE]);
}

void EntityItemProperties::setShapeTypeFromString(const QString& shapeName) {
    if (stringToShapeTypeLookup.empty()) {
        buildStringToShapeTypeLookup();
    }
    auto shapeTypeItr = stringToShapeTypeLookup.find(shapeName.toLower());
    if (shapeTypeItr != stringToShapeTypeLookup.end()) {
        _shapeType = shapeTypeItr.value();
        _shapeTypeChanged = true;
    }
}

using BackgroundPair = std::pair<const BackgroundMode, const QString>;
const std::array<BackgroundPair, BACKGROUND_MODE_ITEM_COUNT> BACKGROUND_MODES = { {
    BackgroundPair { BACKGROUND_MODE_INHERIT, { "inherit" } },
    BackgroundPair { BACKGROUND_MODE_SKYBOX, { "skybox" } }
} };

QString EntityItemProperties::getBackgroundModeAsString() const {
    return BACKGROUND_MODES[_backgroundMode].second;
}

QString EntityItemProperties::getBackgroundModeString(BackgroundMode mode) {
    return BACKGROUND_MODES[mode].second;
}

void EntityItemProperties::setBackgroundModeFromString(const QString& backgroundMode) {
    auto result = std::find_if(BACKGROUND_MODES.begin(), BACKGROUND_MODES.end(), [&](const BackgroundPair& pair) {
        return (pair.second == backgroundMode);
    });
    if (result != BACKGROUND_MODES.end()) {
        _backgroundMode = result->first;
        _backgroundModeChanged = true;
    }
}

EntityPropertyFlags EntityItemProperties::getChangedProperties() const {
    EntityPropertyFlags changedProperties;

    CHECK_PROPERTY_CHANGE(PROP_POSITION, position);
    CHECK_PROPERTY_CHANGE(PROP_DIMENSIONS, dimensions);
    CHECK_PROPERTY_CHANGE(PROP_ROTATION, rotation);
    CHECK_PROPERTY_CHANGE(PROP_DENSITY, density);
    CHECK_PROPERTY_CHANGE(PROP_VELOCITY, velocity);
    CHECK_PROPERTY_CHANGE(PROP_GRAVITY, gravity);
    CHECK_PROPERTY_CHANGE(PROP_ACCELERATION, acceleration);
    CHECK_PROPERTY_CHANGE(PROP_DAMPING, damping);
    CHECK_PROPERTY_CHANGE(PROP_RESTITUTION, restitution);
    CHECK_PROPERTY_CHANGE(PROP_FRICTION, friction);
    CHECK_PROPERTY_CHANGE(PROP_LIFETIME, lifetime);
    CHECK_PROPERTY_CHANGE(PROP_SCRIPT, script);
    CHECK_PROPERTY_CHANGE(PROP_SCRIPT_TIMESTAMP, scriptTimestamp);
    CHECK_PROPERTY_CHANGE(PROP_COLLISION_SOUND_URL, collisionSoundURL);
    CHECK_PROPERTY_CHANGE(PROP_COLOR, color);
    CHECK_PROPERTY_CHANGE(PROP_COLOR_SPREAD, colorSpread);
    CHECK_PROPERTY_CHANGE(PROP_COLOR_START, colorStart);
    CHECK_PROPERTY_CHANGE(PROP_COLOR_FINISH, colorFinish);
    CHECK_PROPERTY_CHANGE(PROP_ALPHA, alpha);
    CHECK_PROPERTY_CHANGE(PROP_ALPHA_SPREAD, alphaSpread);
    CHECK_PROPERTY_CHANGE(PROP_ALPHA_START, alphaStart);
    CHECK_PROPERTY_CHANGE(PROP_ALPHA_FINISH, alphaFinish);
    CHECK_PROPERTY_CHANGE(PROP_EMITTER_SHOULD_TRAIL, emitterShouldTrail);
    CHECK_PROPERTY_CHANGE(PROP_MODEL_URL, modelURL);
    CHECK_PROPERTY_CHANGE(PROP_COMPOUND_SHAPE_URL, compoundShapeURL);
    CHECK_PROPERTY_CHANGE(PROP_VISIBLE, visible);
    CHECK_PROPERTY_CHANGE(PROP_REGISTRATION_POINT, registrationPoint);
    CHECK_PROPERTY_CHANGE(PROP_ANGULAR_VELOCITY, angularVelocity);
    CHECK_PROPERTY_CHANGE(PROP_ANGULAR_DAMPING, angularDamping);
    CHECK_PROPERTY_CHANGE(PROP_COLLISIONLESS, collisionless);
    CHECK_PROPERTY_CHANGE(PROP_COLLISION_MASK, collisionMask);
    CHECK_PROPERTY_CHANGE(PROP_DYNAMIC, dynamic);
    CHECK_PROPERTY_CHANGE(PROP_IS_SPOTLIGHT, isSpotlight);
    CHECK_PROPERTY_CHANGE(PROP_INTENSITY, intensity);
    CHECK_PROPERTY_CHANGE(PROP_FALLOFF_RADIUS, falloffRadius);
    CHECK_PROPERTY_CHANGE(PROP_EXPONENT, exponent);
    CHECK_PROPERTY_CHANGE(PROP_CUTOFF, cutoff);
    CHECK_PROPERTY_CHANGE(PROP_LOCKED, locked);
    CHECK_PROPERTY_CHANGE(PROP_TEXTURES, textures);
    CHECK_PROPERTY_CHANGE(PROP_USER_DATA, userData);
    CHECK_PROPERTY_CHANGE(PROP_SIMULATION_OWNER, simulationOwner);
    CHECK_PROPERTY_CHANGE(PROP_TEXT, text);
    CHECK_PROPERTY_CHANGE(PROP_LINE_HEIGHT, lineHeight);
    CHECK_PROPERTY_CHANGE(PROP_TEXT_COLOR, textColor);
    CHECK_PROPERTY_CHANGE(PROP_BACKGROUND_COLOR, backgroundColor);
    CHECK_PROPERTY_CHANGE(PROP_SHAPE_TYPE, shapeType);
    CHECK_PROPERTY_CHANGE(PROP_EMITTING_PARTICLES, isEmitting);
    CHECK_PROPERTY_CHANGE(PROP_MAX_PARTICLES, maxParticles);
    CHECK_PROPERTY_CHANGE(PROP_LIFESPAN, lifespan);
    CHECK_PROPERTY_CHANGE(PROP_EMIT_RATE, emitRate);
    CHECK_PROPERTY_CHANGE(PROP_EMIT_SPEED, emitSpeed);
    CHECK_PROPERTY_CHANGE(PROP_SPEED_SPREAD, speedSpread);
    CHECK_PROPERTY_CHANGE(PROP_EMIT_ORIENTATION, emitOrientation);
    CHECK_PROPERTY_CHANGE(PROP_EMIT_DIMENSIONS, emitDimensions);
    CHECK_PROPERTY_CHANGE(PROP_EMIT_RADIUS_START, emitRadiusStart);
    CHECK_PROPERTY_CHANGE(PROP_POLAR_START, polarStart);
    CHECK_PROPERTY_CHANGE(PROP_POLAR_FINISH, polarFinish);
    CHECK_PROPERTY_CHANGE(PROP_AZIMUTH_START, azimuthStart);
    CHECK_PROPERTY_CHANGE(PROP_AZIMUTH_FINISH, azimuthFinish);
    CHECK_PROPERTY_CHANGE(PROP_EMIT_ACCELERATION, emitAcceleration);
    CHECK_PROPERTY_CHANGE(PROP_ACCELERATION_SPREAD, accelerationSpread);
    CHECK_PROPERTY_CHANGE(PROP_PARTICLE_RADIUS, particleRadius);
    CHECK_PROPERTY_CHANGE(PROP_RADIUS_SPREAD, radiusSpread);
    CHECK_PROPERTY_CHANGE(PROP_RADIUS_START, radiusStart);
    CHECK_PROPERTY_CHANGE(PROP_RADIUS_FINISH, radiusFinish);
    CHECK_PROPERTY_CHANGE(PROP_MARKETPLACE_ID, marketplaceID);
    CHECK_PROPERTY_CHANGE(PROP_NAME, name);
    CHECK_PROPERTY_CHANGE(PROP_BACKGROUND_MODE, backgroundMode);
    CHECK_PROPERTY_CHANGE(PROP_SOURCE_URL, sourceUrl);
    CHECK_PROPERTY_CHANGE(PROP_VOXEL_VOLUME_SIZE, voxelVolumeSize);
    CHECK_PROPERTY_CHANGE(PROP_VOXEL_DATA, voxelData);
    CHECK_PROPERTY_CHANGE(PROP_VOXEL_SURFACE_STYLE, voxelSurfaceStyle);
    CHECK_PROPERTY_CHANGE(PROP_LINE_WIDTH, lineWidth);
    CHECK_PROPERTY_CHANGE(PROP_LINE_POINTS, linePoints);
    CHECK_PROPERTY_CHANGE(PROP_HREF, href);
    CHECK_PROPERTY_CHANGE(PROP_DESCRIPTION, description);
    CHECK_PROPERTY_CHANGE(PROP_FACE_CAMERA, faceCamera);
    CHECK_PROPERTY_CHANGE(PROP_ACTION_DATA, actionData);
    CHECK_PROPERTY_CHANGE(PROP_NORMALS, normals);
    CHECK_PROPERTY_CHANGE(PROP_STROKE_WIDTHS, strokeWidths);
    CHECK_PROPERTY_CHANGE(PROP_X_TEXTURE_URL, xTextureURL);
    CHECK_PROPERTY_CHANGE(PROP_Y_TEXTURE_URL, yTextureURL);
    CHECK_PROPERTY_CHANGE(PROP_Z_TEXTURE_URL, zTextureURL);
    CHECK_PROPERTY_CHANGE(PROP_X_N_NEIGHBOR_ID, xNNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_Y_N_NEIGHBOR_ID, yNNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_Z_N_NEIGHBOR_ID, zNNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_X_P_NEIGHBOR_ID, xPNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_Y_P_NEIGHBOR_ID, yPNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_Z_P_NEIGHBOR_ID, zPNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_PARENT_ID, parentID);
    CHECK_PROPERTY_CHANGE(PROP_PARENT_JOINT_INDEX, parentJointIndex);
    CHECK_PROPERTY_CHANGE(PROP_JOINT_ROTATIONS_SET, jointRotationsSet);
    CHECK_PROPERTY_CHANGE(PROP_JOINT_ROTATIONS, jointRotations);
    CHECK_PROPERTY_CHANGE(PROP_JOINT_TRANSLATIONS_SET, jointTranslationsSet);
    CHECK_PROPERTY_CHANGE(PROP_JOINT_TRANSLATIONS, jointTranslations);
    CHECK_PROPERTY_CHANGE(PROP_QUERY_AA_CUBE, queryAACube);
    CHECK_PROPERTY_CHANGE(PROP_LOCAL_POSITION, localPosition);
    CHECK_PROPERTY_CHANGE(PROP_LOCAL_ROTATION, localRotation);
    CHECK_PROPERTY_CHANGE(PROP_LOCAL_VELOCITY, localVelocity);
    CHECK_PROPERTY_CHANGE(PROP_LOCAL_ANGULAR_VELOCITY, localAngularVelocity);

    CHECK_PROPERTY_CHANGE(PROP_FLYING_ALLOWED, flyingAllowed);
    CHECK_PROPERTY_CHANGE(PROP_GHOSTING_ALLOWED, ghostingAllowed);

    CHECK_PROPERTY_CHANGE(PROP_CLIENT_ONLY, clientOnly);
    CHECK_PROPERTY_CHANGE(PROP_OWNING_AVATAR_ID, owningAvatarID);

    CHECK_PROPERTY_CHANGE(PROP_SHAPE, shape);
    CHECK_PROPERTY_CHANGE(PROP_DPI, dpi);

    changedProperties += _animation.getChangedProperties();
    changedProperties += _keyLight.getChangedProperties();
    changedProperties += _skybox.getChangedProperties();
    changedProperties += _stage.getChangedProperties();

    return changedProperties;
}

QScriptValue EntityItemProperties::copyToScriptValue(QScriptEngine* engine, bool skipDefaults) const {
    QScriptValue properties = engine->newObject();
    EntityItemProperties defaultEntityProperties;

    if (_created == UNKNOWN_CREATED_TIME) {
        // No entity properties can have been set so return without setting any default, zero property values.
        return properties;
    }

    if (_idSet) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_ALWAYS(id, _id.toString());
    }

    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_ALWAYS(type, EntityTypes::getEntityTypeName(_type));
    auto created = QDateTime::fromMSecsSinceEpoch(getCreated() / 1000.0f, Qt::UTC); // usec per msec
    created.setTimeSpec(Qt::OffsetFromUTC);
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_ALWAYS(created, created.toString(Qt::ISODate));

    if (!skipDefaults || _lifetime != defaultEntityProperties._lifetime) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(age, getAge()); // gettable, but not settable
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(ageAsText, formatSecondsElapsed(getAge())); // gettable, but not settable
    }

    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_POSITION, position);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DIMENSIONS, dimensions);
    if (!skipDefaults) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DIMENSIONS, naturalDimensions); // gettable, but not settable
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_POSITION, naturalPosition);
    }
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ROTATION, rotation);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_VELOCITY, velocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAVITY, gravity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ACCELERATION, acceleration);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DAMPING, damping);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_RESTITUTION, restitution);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_FRICTION, friction);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DENSITY, density);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LIFETIME, lifetime);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SCRIPT, script);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SCRIPT_TIMESTAMP, scriptTimestamp);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_REGISTRATION_POINT, registrationPoint);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ANGULAR_VELOCITY, angularVelocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ANGULAR_DAMPING, angularDamping);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_VISIBLE, visible);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLLISIONLESS, collisionless);
    COPY_PROXY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_COLLISIONLESS, collisionless, ignoreForCollisions, getCollisionless()); // legacy support
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLLISION_MASK, collisionMask);
    COPY_PROXY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_COLLISION_MASK, collisionMask, collidesWith, getCollisionMaskAsString());
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DYNAMIC, dynamic);
    COPY_PROXY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_DYNAMIC, dynamic, collisionsWillMove, getDynamic()); // legacy support
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_HREF, href);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DESCRIPTION, description);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_FACE_CAMERA, faceCamera);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ACTION_DATA, actionData);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCKED, locked);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_USER_DATA, userData);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MARKETPLACE_ID, marketplaceID);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_NAME, name);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLLISION_SOUND_URL, collisionSoundURL);

    // Boxes, Spheres, Light, Line, Model(??), Particle, PolyLine
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLOR, color);

    // Particles only
    if (_type == EntityTypes::ParticleEffect) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMITTING_PARTICLES, isEmitting);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MAX_PARTICLES, maxParticles);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LIFESPAN, lifespan);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMIT_RATE, emitRate);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMIT_SPEED, emitSpeed);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SPEED_SPREAD, speedSpread);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMIT_ORIENTATION, emitOrientation);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMIT_DIMENSIONS, emitDimensions);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMIT_RADIUS_START, emitRadiusStart);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_POLAR_START, polarStart);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_POLAR_FINISH, polarFinish);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_AZIMUTH_START, azimuthStart);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_AZIMUTH_FINISH, azimuthFinish);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMIT_ACCELERATION, emitAcceleration);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ACCELERATION_SPREAD, accelerationSpread);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_PARTICLE_RADIUS, particleRadius);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_RADIUS_SPREAD, radiusSpread);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_RADIUS_START, radiusStart);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_RADIUS_FINISH, radiusFinish);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLOR_SPREAD, colorSpread);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLOR_START, colorStart);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLOR_FINISH, colorFinish);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA, alpha);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA_SPREAD, alphaSpread);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA_START, alphaStart);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA_FINISH, alphaFinish);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMITTER_SHOULD_TRAIL, emitterShouldTrail);
    }

    // Models only
    if (_type == EntityTypes::Model) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MODEL_URL, modelURL);
        _animation.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_JOINT_ROTATIONS_SET, jointRotationsSet);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_JOINT_ROTATIONS, jointRotations);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_JOINT_TRANSLATIONS_SET, jointTranslationsSet);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_JOINT_TRANSLATIONS, jointTranslations);
    }

    if (_type == EntityTypes::Model || _type == EntityTypes::Zone || _type == EntityTypes::ParticleEffect) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_SHAPE_TYPE, shapeType, getShapeTypeAsString());
    }
    if (_type == EntityTypes::Box) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_SHAPE_TYPE, shapeType, QString("Box"));
    }
    if (_type == EntityTypes::Sphere) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_SHAPE_TYPE, shapeType, QString("Sphere"));
    }
    if (_type == EntityTypes::Box || _type == EntityTypes::Sphere || _type == EntityTypes::Shape) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SHAPE, shape);
    }

    // FIXME - it seems like ParticleEffect should also support this
    if (_type == EntityTypes::Model || _type == EntityTypes::Zone) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COMPOUND_SHAPE_URL, compoundShapeURL);
    }

    // Models & Particles
    if (_type == EntityTypes::Model || _type == EntityTypes::ParticleEffect) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_TEXTURES, textures);
    }

    // Lights only
    if (_type == EntityTypes::Light) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_IS_SPOTLIGHT, isSpotlight);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_INTENSITY, intensity);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_FALLOFF_RADIUS, falloffRadius);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EXPONENT, exponent);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CUTOFF, cutoff);
    }

    // Text only
    if (_type == EntityTypes::Text) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_TEXT, text);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LINE_HEIGHT, lineHeight);
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_TEXT_COLOR, textColor, getTextColor());
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_BACKGROUND_COLOR, backgroundColor, getBackgroundColor());
    }

    // Zones only
    if (_type == EntityTypes::Zone) {
        _keyLight.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);

        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_BACKGROUND_MODE, backgroundMode, getBackgroundModeAsString());

        _stage.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);
        _skybox.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_FLYING_ALLOWED, flyingAllowed);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_GHOSTING_ALLOWED, ghostingAllowed);
    }

    // Web only
    if (_type == EntityTypes::Web) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SOURCE_URL, sourceUrl);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DPI, dpi);
    }

    // PolyVoxel only
    if (_type == EntityTypes::PolyVox) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_VOXEL_VOLUME_SIZE, voxelVolumeSize);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_VOXEL_DATA, voxelData);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_VOXEL_SURFACE_STYLE, voxelSurfaceStyle);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_X_TEXTURE_URL, xTextureURL);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_Y_TEXTURE_URL, yTextureURL);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_Z_TEXTURE_URL, zTextureURL);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_X_N_NEIGHBOR_ID, xNNeighborID);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_Y_N_NEIGHBOR_ID, yNNeighborID);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_Z_N_NEIGHBOR_ID, zNNeighborID);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_X_P_NEIGHBOR_ID, xPNeighborID);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_Y_P_NEIGHBOR_ID, yPNeighborID);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_Z_P_NEIGHBOR_ID, zPNeighborID);
    }

    // Lines & PolyLines
    if (_type == EntityTypes::Line || _type == EntityTypes::PolyLine) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LINE_WIDTH, lineWidth);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LINE_POINTS, linePoints);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_NORMALS, normals);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_STROKE_WIDTHS, strokeWidths);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_TEXTURES, textures);
    }

    // Sitting properties support
    if (!skipDefaults) {
        QScriptValue sittingPoints = engine->newObject();
        for (int i = 0; i < _sittingPoints.size(); ++i) {
            QScriptValue sittingPoint = engine->newObject();
            sittingPoint.setProperty("name", _sittingPoints.at(i).name);
            sittingPoint.setProperty("position", vec3toScriptValue(engine, _sittingPoints.at(i).position));
            sittingPoint.setProperty("rotation", quatToScriptValue(engine, _sittingPoints.at(i).rotation));
            sittingPoints.setProperty(i, sittingPoint);
        }
        sittingPoints.setProperty("length", _sittingPoints.size());
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_ALWAYS(sittingPoints, sittingPoints); // gettable, but not settable
    }

    if (!skipDefaults) {
        AABox aaBox = getAABox();
        QScriptValue boundingBox = engine->newObject();
        QScriptValue bottomRightNear = vec3toScriptValue(engine, aaBox.getCorner());
        QScriptValue topFarLeft = vec3toScriptValue(engine, aaBox.calcTopFarLeft());
        QScriptValue center = vec3toScriptValue(engine, aaBox.calcCenter());
        QScriptValue boundingBoxDimensions = vec3toScriptValue(engine, aaBox.getDimensions());
        boundingBox.setProperty("brn", bottomRightNear);
        boundingBox.setProperty("tfl", topFarLeft);
        boundingBox.setProperty("center", center);
        boundingBox.setProperty("dimensions", boundingBoxDimensions);
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(boundingBox, boundingBox); // gettable, but not settable
    }

    QString textureNamesStr = QJsonDocument::fromVariant(_textureNames).toJson();
    if (!skipDefaults) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(originalTextures, textureNamesStr); // gettable, but not settable
    }

    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_PARENT_ID, parentID);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_PARENT_JOINT_INDEX, parentJointIndex);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_QUERY_AA_CUBE, queryAACube);

    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCAL_POSITION, localPosition);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCAL_ROTATION, localRotation);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCAL_VELOCITY, localVelocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCAL_ANGULAR_VELOCITY, localAngularVelocity);

    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CLIENT_ONLY, clientOnly);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_OWNING_AVATAR_ID, owningAvatarID);

    // Rendering info
    if (!skipDefaults) {
        QScriptValue renderInfo = engine->newObject();

        // currently only supported by models
        if (_type == EntityTypes::Model) {
            renderInfo.setProperty("verticesCount", (int)getRenderInfoVertexCount()); // FIXME - theoretically the number of vertex could be > max int
            renderInfo.setProperty("texturesSize", (int)getRenderInfoTextureSize()); // FIXME - theoretically the size of textures could be > max int
            renderInfo.setProperty("hasTransparent", getRenderInfoHasTransparent());
            renderInfo.setProperty("drawCalls", getRenderInfoDrawCalls());
            renderInfo.setProperty("texturesCount", getRenderInfoTextureCount());
        }

        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(renderInfo, renderInfo);  // Gettable but not settable
    }

    properties.setProperty("clientOnly", convertScriptValue(engine, getClientOnly()));
    properties.setProperty("owningAvatarID", convertScriptValue(engine, getOwningAvatarID()));

    // FIXME - I don't think these properties are supported any more
    //COPY_PROPERTY_TO_QSCRIPTVALUE(localRenderAlpha);

    return properties;
}

void EntityItemProperties::copyFromScriptValue(const QScriptValue& object, bool honorReadOnly) {
    QScriptValue typeScriptValue = object.property("type");
    if (typeScriptValue.isValid()) {
        setType(typeScriptValue.toVariant().toString());
    }

    COPY_PROPERTY_FROM_QSCRIPTVALUE(position, glmVec3, setPosition);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(dimensions, glmVec3, setDimensions);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(rotation, glmQuat, setRotation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(density, float, setDensity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(velocity, glmVec3, setVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(gravity, glmVec3, setGravity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(acceleration, glmVec3, setAcceleration);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(damping, float, setDamping);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(restitution, float, setRestitution);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(friction, float, setFriction);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(lifetime, float, setLifetime);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(script, QString, setScript);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(scriptTimestamp, quint64, setScriptTimestamp);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(registrationPoint, glmVec3, setRegistrationPoint);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(angularVelocity, glmVec3, setAngularVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(angularDamping, float, setAngularDamping);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(visible, bool, setVisible);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(color, xColor, setColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(colorSpread, xColor, setColorSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(colorStart, xColor, setColorStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(colorFinish, xColor, setColorFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(alpha, float, setAlpha);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(alphaSpread, float, setAlphaSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(alphaStart, float, setAlphaStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(alphaFinish, float, setAlphaFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitterShouldTrail , bool, setEmitterShouldTrail);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(modelURL, QString, setModelURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(compoundShapeURL, QString, setCompoundShapeURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localRenderAlpha, float, setLocalRenderAlpha);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(collisionless, bool, setCollisionless);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(ignoreForCollisions, bool, setCollisionless, getCollisionless); // legacy support
    COPY_PROPERTY_FROM_QSCRIPTVALUE(collisionMask, uint8_t, setCollisionMask);
    COPY_PROPERTY_FROM_QSCRITPTVALUE_ENUM(collidesWith, CollisionMask);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(collisionsWillMove, bool, setDynamic, getDynamic); // legacy support
    COPY_PROPERTY_FROM_QSCRIPTVALUE(dynamic, bool, setDynamic);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(isSpotlight, bool, setIsSpotlight);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(intensity, float, setIntensity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(falloffRadius, float, setFalloffRadius);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(exponent, float, setExponent);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(cutoff, float, setCutoff);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(locked, bool, setLocked);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(textures, QString, setTextures);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(userData, QString, setUserData);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(text, QString, setText);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(lineHeight, float, setLineHeight);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(textColor, xColor, setTextColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(backgroundColor, xColor, setBackgroundColor);
    COPY_PROPERTY_FROM_QSCRITPTVALUE_ENUM(shapeType, ShapeType);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(maxParticles, quint32, setMaxParticles);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(lifespan, float, setLifespan);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(isEmitting, bool, setIsEmitting);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitRate, float, setEmitRate);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitSpeed, float, setEmitSpeed);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(speedSpread, float, setSpeedSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitOrientation, glmQuat, setEmitOrientation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitDimensions, glmVec3, setEmitDimensions);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitRadiusStart, float, setEmitRadiusStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(polarStart, float, setPolarStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(polarFinish, float, setPolarFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(azimuthStart, float, setAzimuthStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(azimuthFinish, float, setAzimuthFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitAcceleration, glmVec3, setEmitAcceleration);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(accelerationSpread, glmVec3, setAccelerationSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(particleRadius, float, setParticleRadius);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(radiusSpread, float, setRadiusSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(radiusStart, float, setRadiusStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(radiusFinish, float, setRadiusFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(marketplaceID, QString, setMarketplaceID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(name, QString, setName);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(collisionSoundURL, QString, setCollisionSoundURL);

    COPY_PROPERTY_FROM_QSCRITPTVALUE_ENUM(backgroundMode, BackgroundMode);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(sourceUrl, QString, setSourceUrl);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(voxelVolumeSize, glmVec3, setVoxelVolumeSize);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(voxelData, QByteArray, setVoxelData);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(voxelSurfaceStyle, uint16_t, setVoxelSurfaceStyle);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(lineWidth, float, setLineWidth);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(linePoints, qVectorVec3, setLinePoints);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(href, QString, setHref);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(description, QString, setDescription);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(faceCamera, bool, setFaceCamera);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(actionData, QByteArray, setActionData);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(normals, qVectorVec3, setNormals);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(strokeWidths,qVectorFloat, setStrokeWidths);

    if (!honorReadOnly) {
        // this is used by the json reader to set things that we don't want javascript to able to affect.
        COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(created, QDateTime, setCreated, [this]() {
                auto result = QDateTime::fromMSecsSinceEpoch(_created / 1000, Qt::UTC); // usec per msec
                return result;
            });
        // TODO: expose this to QScriptValue for JSON saves?
        //COPY_PROPERTY_FROM_QSCRIPTVALUE(simulationOwner, ???, setSimulatorPriority);
    }

    _animation.copyFromScriptValue(object, _defaultSettings);
    _keyLight.copyFromScriptValue(object, _defaultSettings);
    _skybox.copyFromScriptValue(object, _defaultSettings);
    _stage.copyFromScriptValue(object, _defaultSettings);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(xTextureURL, QString, setXTextureURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(yTextureURL, QString, setYTextureURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(zTextureURL, QString, setZTextureURL);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(xNNeighborID, EntityItemID, setXNNeighborID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(yNNeighborID, EntityItemID, setYNNeighborID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(zNNeighborID, EntityItemID, setZNNeighborID);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(xPNeighborID, EntityItemID, setXPNeighborID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(yPNeighborID, EntityItemID, setYPNeighborID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(zPNeighborID, EntityItemID, setZPNeighborID);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(parentID, QUuid, setParentID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(parentJointIndex, quint16, setParentJointIndex);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(queryAACube, AACube, setQueryAACube);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(localPosition, glmVec3, setLocalPosition);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localRotation, glmQuat, setLocalRotation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localVelocity, glmVec3, setLocalVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localAngularVelocity, glmVec3, setLocalAngularVelocity);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(jointRotationsSet, qVectorBool, setJointRotationsSet);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(jointRotations, qVectorQuat, setJointRotations);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(jointTranslationsSet, qVectorBool, setJointTranslationsSet);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(jointTranslations, qVectorVec3, setJointTranslations);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(shape, QString, setShape);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(flyingAllowed, bool, setFlyingAllowed);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(ghostingAllowed, bool, setGhostingAllowed);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(clientOnly, bool, setClientOnly);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(owningAvatarID, QUuid, setOwningAvatarID);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(dpi, uint16_t, setDPI);

    _lastEdited = usecTimestampNow();
}

QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties) {
    return properties.copyToScriptValue(engine, false);
}

QScriptValue EntityItemNonDefaultPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties) {
    return properties.copyToScriptValue(engine, true);
}

void EntityItemPropertiesFromScriptValueIgnoreReadOnly(const QScriptValue &object, EntityItemProperties& properties) {
    properties.copyFromScriptValue(object, false);
}

void EntityItemPropertiesFromScriptValueHonorReadOnly(const QScriptValue &object, EntityItemProperties& properties) {
    properties.copyFromScriptValue(object, true);
}


QScriptValue EntityPropertyFlagsToScriptValue(QScriptEngine* engine, const EntityPropertyFlags& flags) {
    return EntityItemProperties::entityPropertyFlagsToScriptValue(engine, flags);
    QScriptValue result = engine->newObject();
	return result;
}

void EntityPropertyFlagsFromScriptValue(const QScriptValue& object, EntityPropertyFlags& flags) {
    EntityItemProperties::entityPropertyFlagsFromScriptValue(object, flags);
}


QScriptValue EntityItemProperties::entityPropertyFlagsToScriptValue(QScriptEngine* engine, const EntityPropertyFlags& flags) {
    QScriptValue result = engine->newObject();
    return result;
}

static QHash<QString, EntityPropertyList> _propertyStringsToEnums;

void EntityItemProperties::entityPropertyFlagsFromScriptValue(const QScriptValue& object, EntityPropertyFlags& flags) {
    static std::once_flag initMap;

    std::call_once(initMap, [](){
        ADD_PROPERTY_TO_MAP(PROP_VISIBLE, Visible, visible, bool);
        ADD_PROPERTY_TO_MAP(PROP_POSITION, Position, position, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_DIMENSIONS, Dimensions, dimensions, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_ROTATION, Rotation, rotation, glm::quat);
        ADD_PROPERTY_TO_MAP(PROP_DENSITY, Density, density, float);
        ADD_PROPERTY_TO_MAP(PROP_VELOCITY, Velocity, velocity, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_GRAVITY, Gravity, gravity, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_ACCELERATION, Acceleration, acceleration, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_DAMPING, Damping, damping, float);
        ADD_PROPERTY_TO_MAP(PROP_RESTITUTION, Restitution, restitution, float);
        ADD_PROPERTY_TO_MAP(PROP_FRICTION, Friction, friction, float);
        ADD_PROPERTY_TO_MAP(PROP_LIFETIME, Lifetime, lifetime, float);
        ADD_PROPERTY_TO_MAP(PROP_SCRIPT, Script, script, QString);
        ADD_PROPERTY_TO_MAP(PROP_SCRIPT_TIMESTAMP, ScriptTimestamp, scriptTimestamp, quint64);
        ADD_PROPERTY_TO_MAP(PROP_COLLISION_SOUND_URL, CollisionSoundURL, collisionSoundURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_COLOR, Color, color, xColor);
        ADD_PROPERTY_TO_MAP(PROP_COLOR_SPREAD, ColorSpread, colorSpread, xColor);
        ADD_PROPERTY_TO_MAP(PROP_COLOR_START, ColorStart, colorStart, xColor);
        ADD_PROPERTY_TO_MAP(PROP_COLOR_FINISH, ColorFinish, colorFinish, xColor);
        ADD_PROPERTY_TO_MAP(PROP_ALPHA, Alpha, alpha, float);
        ADD_PROPERTY_TO_MAP(PROP_ALPHA_SPREAD, AlphaSpread, alphaSpread, float);
        ADD_PROPERTY_TO_MAP(PROP_ALPHA_START, AlphaStart, alphaStart, float);
        ADD_PROPERTY_TO_MAP(PROP_ALPHA_FINISH, AlphaFinish, alphaFinish, float);
        ADD_PROPERTY_TO_MAP(PROP_EMITTER_SHOULD_TRAIL, EmitterShouldTrail, emitterShouldTrail, bool);
        ADD_PROPERTY_TO_MAP(PROP_MODEL_URL, ModelURL, modelURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_COMPOUND_SHAPE_URL, CompoundShapeURL, compoundShapeURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_REGISTRATION_POINT, RegistrationPoint, registrationPoint, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_ANGULAR_VELOCITY, AngularVelocity, angularVelocity, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_ANGULAR_DAMPING, AngularDamping, angularDamping, float);
        ADD_PROPERTY_TO_MAP(PROP_COLLISIONLESS, Collisionless, collisionless, bool);
        ADD_PROPERTY_TO_MAP(PROP_DYNAMIC, unused, ignoreForCollisions, unused); // legacy support
        ADD_PROPERTY_TO_MAP(PROP_COLLISION_MASK, unused, collisionMask, unused);
        ADD_PROPERTY_TO_MAP(PROP_COLLISION_MASK, unused, collidesWith, unused);
        ADD_PROPERTY_TO_MAP(PROP_DYNAMIC, unused, collisionsWillMove, unused); // legacy support
        ADD_PROPERTY_TO_MAP(PROP_DYNAMIC, unused, dynamic, unused);
        ADD_PROPERTY_TO_MAP(PROP_IS_SPOTLIGHT, IsSpotlight, isSpotlight, bool);
        ADD_PROPERTY_TO_MAP(PROP_INTENSITY, Intensity, intensity, float);
        ADD_PROPERTY_TO_MAP(PROP_FALLOFF_RADIUS, FalloffRadius, falloffRadius, float);
        ADD_PROPERTY_TO_MAP(PROP_EXPONENT, Exponent, exponent, float);
        ADD_PROPERTY_TO_MAP(PROP_CUTOFF, Cutoff, cutoff, float);
        ADD_PROPERTY_TO_MAP(PROP_LOCKED, Locked, locked, bool);
        ADD_PROPERTY_TO_MAP(PROP_TEXTURES, Textures, textures, QString);
        ADD_PROPERTY_TO_MAP(PROP_USER_DATA, UserData, userData, QString);
        ADD_PROPERTY_TO_MAP(PROP_SIMULATION_OWNER, SimulationOwner, simulationOwner, SimulationOwner);
        ADD_PROPERTY_TO_MAP(PROP_TEXT, Text, text, QString);
        ADD_PROPERTY_TO_MAP(PROP_LINE_HEIGHT, LineHeight, lineHeight, float);
        ADD_PROPERTY_TO_MAP(PROP_TEXT_COLOR, TextColor, textColor, xColor);
        ADD_PROPERTY_TO_MAP(PROP_BACKGROUND_COLOR, BackgroundColor, backgroundColor, xColor);
        ADD_PROPERTY_TO_MAP(PROP_SHAPE_TYPE, ShapeType, shapeType, ShapeType);
        ADD_PROPERTY_TO_MAP(PROP_MAX_PARTICLES, MaxParticles, maxParticles, quint32);
        ADD_PROPERTY_TO_MAP(PROP_LIFESPAN, Lifespan, lifespan, float);
        ADD_PROPERTY_TO_MAP(PROP_EMITTING_PARTICLES, IsEmitting, isEmitting, bool);
        ADD_PROPERTY_TO_MAP(PROP_EMIT_RATE, EmitRate, emitRate, float);
        ADD_PROPERTY_TO_MAP(PROP_EMIT_SPEED, EmitSpeed, emitSpeed, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_SPEED_SPREAD, SpeedSpread, speedSpread, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_EMIT_ORIENTATION, EmitOrientation, emitOrientation, glm::quat);
        ADD_PROPERTY_TO_MAP(PROP_EMIT_DIMENSIONS, EmitDimensions, emitDimensions, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_EMIT_RADIUS_START, EmitRadiusStart, emitRadiusStart, float);
        ADD_PROPERTY_TO_MAP(PROP_POLAR_START, EmitPolarStart, polarStart, float);
        ADD_PROPERTY_TO_MAP(PROP_POLAR_FINISH, EmitPolarFinish, polarFinish, float);
        ADD_PROPERTY_TO_MAP(PROP_AZIMUTH_START, EmitAzimuthStart, azimuthStart, float);
        ADD_PROPERTY_TO_MAP(PROP_AZIMUTH_FINISH, EmitAzimuthFinish, azimuthFinish, float);
        ADD_PROPERTY_TO_MAP(PROP_EMIT_ACCELERATION, EmitAcceleration, emitAcceleration, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_ACCELERATION_SPREAD, AccelerationSpread, accelerationSpread, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_PARTICLE_RADIUS, ParticleRadius, particleRadius, float);
        ADD_PROPERTY_TO_MAP(PROP_RADIUS_SPREAD, RadiusSpread, radiusSpread, float);
        ADD_PROPERTY_TO_MAP(PROP_RADIUS_START, RadiusStart, radiusStart, float);
        ADD_PROPERTY_TO_MAP(PROP_RADIUS_FINISH, RadiusFinish, radiusFinish, float);
        ADD_PROPERTY_TO_MAP(PROP_MARKETPLACE_ID, MarketplaceID, marketplaceID, QString);
        ADD_PROPERTY_TO_MAP(PROP_KEYLIGHT_COLOR, KeyLightColor, keyLightColor, xColor);
        ADD_PROPERTY_TO_MAP(PROP_KEYLIGHT_INTENSITY, KeyLightIntensity, keyLightIntensity, float);
        ADD_PROPERTY_TO_MAP(PROP_KEYLIGHT_AMBIENT_INTENSITY, KeyLightAmbientIntensity, keyLightAmbientIntensity, float);
        ADD_PROPERTY_TO_MAP(PROP_KEYLIGHT_DIRECTION, KeyLightDirection, keyLightDirection, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_VOXEL_VOLUME_SIZE, VoxelVolumeSize, voxelVolumeSize, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_VOXEL_DATA, VoxelData, voxelData, QByteArray);
        ADD_PROPERTY_TO_MAP(PROP_VOXEL_SURFACE_STYLE, VoxelSurfaceStyle, voxelSurfaceStyle, uint16_t);
        ADD_PROPERTY_TO_MAP(PROP_NAME, Name, name, QString);
        ADD_PROPERTY_TO_MAP(PROP_BACKGROUND_MODE, BackgroundMode, backgroundMode, BackgroundMode);
        ADD_PROPERTY_TO_MAP(PROP_SOURCE_URL, SourceUrl, sourceUrl, QString);
        ADD_PROPERTY_TO_MAP(PROP_LINE_WIDTH, LineWidth, lineWidth, float);
        ADD_PROPERTY_TO_MAP(PROP_LINE_POINTS, LinePoints, linePoints, QVector<glm::vec3>);
        ADD_PROPERTY_TO_MAP(PROP_HREF, Href, href, QString);
        ADD_PROPERTY_TO_MAP(PROP_DESCRIPTION, Description, description, QString);
        ADD_PROPERTY_TO_MAP(PROP_FACE_CAMERA, FaceCamera, faceCamera, bool);
        ADD_PROPERTY_TO_MAP(PROP_ACTION_DATA, ActionData, actionData, QByteArray);
        ADD_PROPERTY_TO_MAP(PROP_NORMALS, Normals, normals, QVector<glm::vec3>);
        ADD_PROPERTY_TO_MAP(PROP_STROKE_WIDTHS, StrokeWidths, strokeWidths, QVector<float>);
        ADD_PROPERTY_TO_MAP(PROP_X_TEXTURE_URL, XTextureURL, xTextureURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_Y_TEXTURE_URL, YTextureURL, yTextureURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_Z_TEXTURE_URL, ZTextureURL, zTextureURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_X_N_NEIGHBOR_ID, XNNeighborID, xNNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_Y_N_NEIGHBOR_ID, YNNeighborID, yNNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_Z_N_NEIGHBOR_ID, ZNNeighborID, zNNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_X_P_NEIGHBOR_ID, XPNeighborID, xPNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_Y_P_NEIGHBOR_ID, YPNeighborID, yPNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_Z_P_NEIGHBOR_ID, ZPNeighborID, zPNeighborID, EntityItemID);

        ADD_PROPERTY_TO_MAP(PROP_PARENT_ID, ParentID, parentID, QUuid);
        ADD_PROPERTY_TO_MAP(PROP_PARENT_JOINT_INDEX, ParentJointIndex, parentJointIndex, uint16_t);

        ADD_PROPERTY_TO_MAP(PROP_LOCAL_POSITION, LocalPosition, localPosition, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_LOCAL_ROTATION, LocalRotation, localRotation, glm::quat);
        ADD_PROPERTY_TO_MAP(PROP_LOCAL_VELOCITY, LocalVelocity, localVelocity, glm::vec3);
        ADD_PROPERTY_TO_MAP(PROP_LOCAL_ANGULAR_VELOCITY, LocalAngularVelocity, localAngularVelocity, glm::vec3);

        ADD_PROPERTY_TO_MAP(PROP_JOINT_ROTATIONS_SET, JointRotationsSet, jointRotationsSet, QVector<bool>);
        ADD_PROPERTY_TO_MAP(PROP_JOINT_ROTATIONS, JointRotations, jointRotations, QVector<glm::quat>);
        ADD_PROPERTY_TO_MAP(PROP_JOINT_TRANSLATIONS_SET, JointTranslationsSet, jointTranslationsSet, QVector<bool>);
        ADD_PROPERTY_TO_MAP(PROP_JOINT_TRANSLATIONS, JointTranslations, jointTranslations, QVector<glm::vec3>);

        ADD_PROPERTY_TO_MAP(PROP_SHAPE, Shape, shape, QString);

        ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_URL, Animation, animation, URL, url);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_FPS, Animation, animation, FPS, fps);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_FRAME_INDEX, Animation, animation, CurrentFrame, currentFrame);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_PLAYING, Animation, animation, Running, running);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_LOOP, Animation, animation, Loop, loop);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_FIRST_FRAME, Animation, animation, FirstFrame, firstFrame);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_LAST_FRAME, Animation, animation, LastFrame, lastFrame);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_HOLD, Animation, animation, Hold, hold);

        ADD_GROUP_PROPERTY_TO_MAP(PROP_SKYBOX_COLOR, Skybox, skybox, Color, color);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_SKYBOX_URL, Skybox, skybox, URL, url);

        ADD_GROUP_PROPERTY_TO_MAP(PROP_STAGE_SUN_MODEL_ENABLED, Stage, stage, SunModelEnabled, sunModelEnabled);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_STAGE_LATITUDE, Stage, stage, Latitude, latitude);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_STAGE_LONGITUDE, Stage, stage, Longitude, longitude);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_STAGE_ALTITUDE, Stage, stage, Altitude, altitude);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_STAGE_DAY, Stage, stage, Day, day);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_STAGE_HOUR, Stage, stage, Hour, hour);
        ADD_GROUP_PROPERTY_TO_MAP(PROP_STAGE_AUTOMATIC_HOURDAY, Stage, stage, AutomaticHourDay, automaticHourDay);

        ADD_PROPERTY_TO_MAP(PROP_FLYING_ALLOWED, FlyingAllowed, flyingAllowed, bool);
        ADD_PROPERTY_TO_MAP(PROP_GHOSTING_ALLOWED, GhostingAllowed, ghostingAllowed, bool);

        ADD_PROPERTY_TO_MAP(PROP_DPI, DPI, dpi, uint16_t);

        // FIXME - these are not yet handled
        //ADD_PROPERTY_TO_MAP(PROP_CREATED, Created, created, quint64);

    });

    if (object.isString()) {
        // TODO: figure out how to do this without a double lookup in the map
        if (_propertyStringsToEnums.contains(object.toString())) {
            flags << _propertyStringsToEnums[object.toString()];
        }
    } else if (object.isArray()) {
        quint32 length = object.property("length").toInt32();
        for (quint32 i = 0; i < length; i++) {
            QString propertyName = object.property(i).toString();
            // TODO: figure out how to do this without a double lookup in the map
            if (_propertyStringsToEnums.contains(propertyName)) {
                flags << _propertyStringsToEnums[propertyName];
            }
        }
    }
}

// TODO: Implement support for edit packets that can span an MTU sized buffer. We need to implement a mechanism for the
//       encodeEntityEditPacket() method to communicate the the caller which properties couldn't fit in the buffer. Similar
//       to how we handle this in the Octree streaming case.
//
// TODO: Right now, all possible properties for all subclasses are handled here. Ideally we'd prefer
//       to handle this in a more generic way. Allowing subclasses of EntityItem to register their properties
//
// TODO: There's a lot of repeated patterns in the code below to handle each property. It would be nice if the property
//       registration mechanism allowed us to collapse these repeated sections of code into a single implementation that
//       utilized the registration table to shorten up and simplify this code.
//
// TODO: Implement support for paged properties, spanning MTU, and custom properties
//
// TODO: Implement support for script and visible properties.
//
bool EntityItemProperties::encodeEntityEditPacket(PacketType command, EntityItemID id, const EntityItemProperties& properties,
                                                  QByteArray& buffer) {
    OctreePacketData ourDataPacket(false, buffer.size()); // create a packetData object to add out packet details too.
    OctreePacketData* packetData = &ourDataPacket; // we want a pointer to this so we can use our APPEND_ENTITY_PROPERTY macro

    bool success = true; // assume the best
    OctreeElement::AppendState appendState = OctreeElement::COMPLETED; // assume the best

    // TODO: We need to review how jurisdictions should be handled for entities. (The old Models and Particles code
    // didn't do anything special for jurisdictions, so we're keeping that same behavior here.)
    //
    // Always include the root octcode. This is only because the OctreeEditPacketSender will check these octcodes
    // to determine which server to send the changes to in the case of multiple jurisdictions. The root will be sent
    // to all servers.
    glm::vec3 rootPosition(0);
    float rootScale = 0.5f;
    unsigned char* octcode = pointToOctalCode(rootPosition.x, rootPosition.y, rootPosition.z, rootScale);

    success = packetData->startSubTree(octcode);
    delete[] octcode;

    // assuming we have room to fit our octalCode, proceed...
    if (success) {

        // Now add our edit content details...

        // id
        // encode our ID as a byte count coded byte stream
        QByteArray encodedID = id.toRfc4122(); // NUM_BYTES_RFC4122_UUID

        // encode our ID as a byte count coded byte stream
        ByteCountCoded<quint32> tokenCoder;
        QByteArray encodedToken;

        // encode our type as a byte count coded byte stream
        ByteCountCoded<quint32> typeCoder = (quint32)properties.getType();
        QByteArray encodedType = typeCoder;

        quint64 updateDelta = 0; // this is an edit so by definition, it's update is in sync
        ByteCountCoded<quint64> updateDeltaCoder = updateDelta;
        QByteArray encodedUpdateDelta = updateDeltaCoder;

        EntityPropertyFlags propertyFlags(PROP_LAST_ITEM);
        EntityPropertyFlags requestedProperties = properties.getChangedProperties();
        EntityPropertyFlags propertiesDidntFit = requestedProperties;

        // TODO: we need to handle the multi-pass form of this, similar to how we handle entity data
        //
        // If we are being called for a subsequent pass at appendEntityData() that failed to completely encode this item,
        // then our modelTreeElementExtraEncodeData should include data about which properties we need to append.
        //if (modelTreeElementExtraEncodeData && modelTreeElementExtraEncodeData->includedItems.contains(getEntityItemID())) {
        //    requestedProperties = modelTreeElementExtraEncodeData->includedItems.value(getEntityItemID());
        //}

        LevelDetails entityLevel = packetData->startLevel();

        // Last Edited quint64 always first, before any other details, which allows us easy access to adjusting this
        // timestamp for clock skew
        quint64 lastEdited = properties.getLastEdited();
        bool successLastEditedFits = packetData->appendValue(lastEdited);

        bool successIDFits = packetData->appendRawData(encodedID);
        if (successIDFits) {
            successIDFits = packetData->appendRawData(encodedToken);
        }
        bool successTypeFits = packetData->appendRawData(encodedType);

        // NOTE: We intentionally do not send "created" times in edit messages. This is because:
        //   1) if the edit is to an existing entity, the created time can not be changed
        //   2) if the edit is to a new entity, the created time is the last edited time

        // TODO: Should we get rid of this in this in edit packets, since this has to always be 0?
        bool successLastUpdatedFits = packetData->appendRawData(encodedUpdateDelta);

        int propertyFlagsOffset = packetData->getUncompressedByteOffset();
        QByteArray encodedPropertyFlags = propertyFlags;
        int oldPropertyFlagsLength = encodedPropertyFlags.length();
        bool successPropertyFlagsFits = packetData->appendRawData(encodedPropertyFlags);
        int propertyCount = 0;

        bool headerFits = successIDFits && successTypeFits && successLastEditedFits
        && successLastUpdatedFits && successPropertyFlagsFits;

        int startOfEntityItemData = packetData->getUncompressedByteOffset();

        if (headerFits) {
            bool successPropertyFits;
            propertyFlags -= PROP_LAST_ITEM; // clear the last item for now, we may or may not set it as the actual item

            // These items would go here once supported....
            //      PROP_PAGED_PROPERTY,
            //      PROP_CUSTOM_PROPERTIES_INCLUDED,

            APPEND_ENTITY_PROPERTY(PROP_SIMULATION_OWNER, properties._simulationOwner.toByteArray());
            APPEND_ENTITY_PROPERTY(PROP_POSITION, properties.getPosition());
            APPEND_ENTITY_PROPERTY(PROP_DIMENSIONS, properties.getDimensions()); // NOTE: PROP_RADIUS obsolete
            APPEND_ENTITY_PROPERTY(PROP_ROTATION, properties.getRotation());
            APPEND_ENTITY_PROPERTY(PROP_DENSITY, properties.getDensity());
            APPEND_ENTITY_PROPERTY(PROP_VELOCITY, properties.getVelocity());
            APPEND_ENTITY_PROPERTY(PROP_GRAVITY, properties.getGravity());
            APPEND_ENTITY_PROPERTY(PROP_ACCELERATION, properties.getAcceleration());
            APPEND_ENTITY_PROPERTY(PROP_DAMPING, properties.getDamping());
            APPEND_ENTITY_PROPERTY(PROP_RESTITUTION, properties.getRestitution());
            APPEND_ENTITY_PROPERTY(PROP_FRICTION, properties.getFriction());
            APPEND_ENTITY_PROPERTY(PROP_LIFETIME, properties.getLifetime());
            APPEND_ENTITY_PROPERTY(PROP_SCRIPT, properties.getScript());
            APPEND_ENTITY_PROPERTY(PROP_SCRIPT_TIMESTAMP, properties.getScriptTimestamp());
            APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
            APPEND_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, properties.getRegistrationPoint());
            APPEND_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, properties.getAngularVelocity());
            APPEND_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, properties.getAngularDamping());
            APPEND_ENTITY_PROPERTY(PROP_VISIBLE, properties.getVisible());
            APPEND_ENTITY_PROPERTY(PROP_COLLISIONLESS, properties.getCollisionless());
            APPEND_ENTITY_PROPERTY(PROP_COLLISION_MASK, properties.getCollisionMask());
            APPEND_ENTITY_PROPERTY(PROP_DYNAMIC, properties.getDynamic());
            APPEND_ENTITY_PROPERTY(PROP_LOCKED, properties.getLocked());
            APPEND_ENTITY_PROPERTY(PROP_USER_DATA, properties.getUserData());
            APPEND_ENTITY_PROPERTY(PROP_HREF, properties.getHref());
            APPEND_ENTITY_PROPERTY(PROP_DESCRIPTION, properties.getDescription());
            APPEND_ENTITY_PROPERTY(PROP_PARENT_ID, properties.getParentID());
            APPEND_ENTITY_PROPERTY(PROP_PARENT_JOINT_INDEX, properties.getParentJointIndex());
            APPEND_ENTITY_PROPERTY(PROP_QUERY_AA_CUBE, properties.getQueryAACube());

            if (properties.getType() == EntityTypes::Web) {
                APPEND_ENTITY_PROPERTY(PROP_SOURCE_URL, properties.getSourceUrl());
                APPEND_ENTITY_PROPERTY(PROP_DPI, properties.getDPI());
            }

            if (properties.getType() == EntityTypes::Text) {
                APPEND_ENTITY_PROPERTY(PROP_TEXT, properties.getText());
                APPEND_ENTITY_PROPERTY(PROP_LINE_HEIGHT, properties.getLineHeight());
                APPEND_ENTITY_PROPERTY(PROP_TEXT_COLOR, properties.getTextColor());
                APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, properties.getBackgroundColor());
                APPEND_ENTITY_PROPERTY(PROP_FACE_CAMERA, properties.getFaceCamera());
            }

            if (properties.getType() == EntityTypes::Model) {
                APPEND_ENTITY_PROPERTY(PROP_MODEL_URL, properties.getModelURL());
                APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, properties.getCompoundShapeURL());
                APPEND_ENTITY_PROPERTY(PROP_TEXTURES, properties.getTextures());
                APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)(properties.getShapeType()));

                _staticAnimation.setProperties(properties);
                _staticAnimation.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);

                APPEND_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS_SET, properties.getJointRotationsSet());
                APPEND_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS, properties.getJointRotations());
                APPEND_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS_SET, properties.getJointTranslationsSet());
                APPEND_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS, properties.getJointTranslations());
            }

            if (properties.getType() == EntityTypes::Light) {
                APPEND_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, properties.getIsSpotlight());
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
                APPEND_ENTITY_PROPERTY(PROP_INTENSITY, properties.getIntensity());
                APPEND_ENTITY_PROPERTY(PROP_FALLOFF_RADIUS, properties.getFalloffRadius());
                APPEND_ENTITY_PROPERTY(PROP_EXPONENT, properties.getExponent());
                APPEND_ENTITY_PROPERTY(PROP_CUTOFF, properties.getCutoff());
            }

            if (properties.getType() == EntityTypes::ParticleEffect) {
                APPEND_ENTITY_PROPERTY(PROP_TEXTURES, properties.getTextures());
                APPEND_ENTITY_PROPERTY(PROP_MAX_PARTICLES, properties.getMaxParticles());
                APPEND_ENTITY_PROPERTY(PROP_LIFESPAN, properties.getLifespan());
                APPEND_ENTITY_PROPERTY(PROP_EMITTING_PARTICLES, properties.getIsEmitting());
                APPEND_ENTITY_PROPERTY(PROP_EMIT_RATE, properties.getEmitRate());
                APPEND_ENTITY_PROPERTY(PROP_EMIT_SPEED, properties.getEmitSpeed());
                APPEND_ENTITY_PROPERTY(PROP_SPEED_SPREAD, properties.getSpeedSpread());
                APPEND_ENTITY_PROPERTY(PROP_EMIT_ORIENTATION, properties.getEmitOrientation());
                APPEND_ENTITY_PROPERTY(PROP_EMIT_DIMENSIONS, properties.getEmitDimensions());
                APPEND_ENTITY_PROPERTY(PROP_EMIT_RADIUS_START, properties.getEmitRadiusStart());
                APPEND_ENTITY_PROPERTY(PROP_POLAR_START, properties.getPolarStart());
                APPEND_ENTITY_PROPERTY(PROP_POLAR_FINISH, properties.getPolarFinish());
                APPEND_ENTITY_PROPERTY(PROP_AZIMUTH_START, properties.getAzimuthStart());
                APPEND_ENTITY_PROPERTY(PROP_AZIMUTH_FINISH, properties.getAzimuthFinish());
                APPEND_ENTITY_PROPERTY(PROP_EMIT_ACCELERATION, properties.getEmitAcceleration());
                APPEND_ENTITY_PROPERTY(PROP_ACCELERATION_SPREAD, properties.getAccelerationSpread());
                APPEND_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, properties.getParticleRadius());
                APPEND_ENTITY_PROPERTY(PROP_RADIUS_SPREAD, properties.getRadiusSpread());
                APPEND_ENTITY_PROPERTY(PROP_RADIUS_START, properties.getRadiusStart());
                APPEND_ENTITY_PROPERTY(PROP_RADIUS_FINISH, properties.getRadiusFinish());
                APPEND_ENTITY_PROPERTY(PROP_COLOR_SPREAD, properties.getColorSpread());
                APPEND_ENTITY_PROPERTY(PROP_COLOR_START, properties.getColorStart());
                APPEND_ENTITY_PROPERTY(PROP_COLOR_FINISH, properties.getColorFinish());
                APPEND_ENTITY_PROPERTY(PROP_ALPHA_SPREAD, properties.getAlphaSpread());
                APPEND_ENTITY_PROPERTY(PROP_ALPHA_START, properties.getAlphaStart());
                APPEND_ENTITY_PROPERTY(PROP_ALPHA_FINISH, properties.getAlphaFinish());
                APPEND_ENTITY_PROPERTY(PROP_EMITTER_SHOULD_TRAIL, properties.getEmitterShouldTrail());
            }

            if (properties.getType() == EntityTypes::Zone) {
                _staticKeyLight.setProperties(properties);
                _staticKeyLight.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);

                _staticStage.setProperties(properties);
                _staticStage.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);

                APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)properties.getShapeType());
                APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, properties.getCompoundShapeURL());

                APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, (uint32_t)properties.getBackgroundMode());

                _staticSkybox.setProperties(properties);
                _staticSkybox.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);

                APPEND_ENTITY_PROPERTY(PROP_FLYING_ALLOWED, properties.getFlyingAllowed());
                APPEND_ENTITY_PROPERTY(PROP_GHOSTING_ALLOWED, properties.getGhostingAllowed());
            }

            if (properties.getType() == EntityTypes::PolyVox) {
                APPEND_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, properties.getVoxelVolumeSize());
                APPEND_ENTITY_PROPERTY(PROP_VOXEL_DATA, properties.getVoxelData());
                APPEND_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, properties.getVoxelSurfaceStyle());
                APPEND_ENTITY_PROPERTY(PROP_X_TEXTURE_URL, properties.getXTextureURL());
                APPEND_ENTITY_PROPERTY(PROP_Y_TEXTURE_URL, properties.getYTextureURL());
                APPEND_ENTITY_PROPERTY(PROP_Z_TEXTURE_URL, properties.getZTextureURL());
                APPEND_ENTITY_PROPERTY(PROP_X_N_NEIGHBOR_ID, properties.getXNNeighborID());
                APPEND_ENTITY_PROPERTY(PROP_Y_N_NEIGHBOR_ID, properties.getYNNeighborID());
                APPEND_ENTITY_PROPERTY(PROP_Z_N_NEIGHBOR_ID, properties.getZNNeighborID());
                APPEND_ENTITY_PROPERTY(PROP_X_P_NEIGHBOR_ID, properties.getXPNeighborID());
                APPEND_ENTITY_PROPERTY(PROP_Y_P_NEIGHBOR_ID, properties.getYPNeighborID());
                APPEND_ENTITY_PROPERTY(PROP_Z_P_NEIGHBOR_ID, properties.getZPNeighborID());
            }

            if (properties.getType() == EntityTypes::Line) {
                APPEND_ENTITY_PROPERTY(PROP_LINE_WIDTH, properties.getLineWidth());
                APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, properties.getLinePoints());
            }

            if (properties.getType() == EntityTypes::PolyLine) {
                APPEND_ENTITY_PROPERTY(PROP_LINE_WIDTH, properties.getLineWidth());
                APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, properties.getLinePoints());
                APPEND_ENTITY_PROPERTY(PROP_NORMALS, properties.getNormals());
                APPEND_ENTITY_PROPERTY(PROP_STROKE_WIDTHS, properties.getStrokeWidths());
                APPEND_ENTITY_PROPERTY(PROP_TEXTURES, properties.getTextures());
            }
            // NOTE: Spheres and Boxes are just special cases of Shape, and they need to include their PROP_SHAPE
            // when encoding/decoding edits because otherwise they can't polymorph to other shape types
            if (properties.getType() == EntityTypes::Shape ||
                properties.getType() == EntityTypes::Box ||
                properties.getType() == EntityTypes::Sphere) {
                APPEND_ENTITY_PROPERTY(PROP_SHAPE, properties.getShape());
            }
            APPEND_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, properties.getMarketplaceID());
            APPEND_ENTITY_PROPERTY(PROP_NAME, properties.getName());
            APPEND_ENTITY_PROPERTY(PROP_COLLISION_SOUND_URL, properties.getCollisionSoundURL());
            APPEND_ENTITY_PROPERTY(PROP_ACTION_DATA, properties.getActionData());
            APPEND_ENTITY_PROPERTY(PROP_ALPHA, properties.getAlpha());
        }

        if (propertyCount > 0) {
            int endOfEntityItemData = packetData->getUncompressedByteOffset();

            encodedPropertyFlags = propertyFlags;
            int newPropertyFlagsLength = encodedPropertyFlags.length();
            packetData->updatePriorBytes(propertyFlagsOffset,
                                         (const unsigned char*)encodedPropertyFlags.constData(), encodedPropertyFlags.length());

            // if the size of the PropertyFlags shrunk, we need to shift everything down to front of packet.
            if (newPropertyFlagsLength < oldPropertyFlagsLength) {
                int oldSize = packetData->getUncompressedSize();

                const unsigned char* modelItemData = packetData->getUncompressedData(propertyFlagsOffset + oldPropertyFlagsLength);
                int modelItemDataLength = endOfEntityItemData - startOfEntityItemData;
                int newEntityItemDataStart = propertyFlagsOffset + newPropertyFlagsLength;
                packetData->updatePriorBytes(newEntityItemDataStart, modelItemData, modelItemDataLength);

                int newSize = oldSize - (oldPropertyFlagsLength - newPropertyFlagsLength);
                packetData->setUncompressedSize(newSize);

            } else {
                assert(newPropertyFlagsLength == oldPropertyFlagsLength); // should not have grown
            }

            packetData->endLevel(entityLevel);
        } else {
            packetData->discardLevel(entityLevel);
            appendState = OctreeElement::NONE; // if we got here, then we didn't include the item
        }

        // If any part of the model items didn't fit, then the element is considered partial
        if (appendState != OctreeElement::COMPLETED) {
            // TODO: handle mechanism for handling partial fitting data!
            // add this item into our list for the next appendElementData() pass
            //modelTreeElementExtraEncodeData->includedItems.insert(getEntityItemID(), propertiesDidntFit);

            // for now, if it's not complete, it's not successful
            success = false;
        }
    }

    if (success) {
        packetData->endSubTree();

        const char* finalizedData = reinterpret_cast<const char*>(packetData->getFinalizedData());
        int finalizedSize = packetData->getFinalizedSize();

        if (finalizedSize <= buffer.size()) {
            buffer.replace(0, finalizedSize, finalizedData, finalizedSize);
            buffer.resize(finalizedSize);
        } else {
            qCDebug(entities) << "ERROR - encoded edit message doesn't fit in output buffer.";
            success = false;
        }
    } else {
        packetData->discardSubTree();
    }
    return success;
}

// TODO:
//   how to handle lastEdited?
//   how to handle lastUpdated?
//   consider handling case where no properties are included... we should just ignore this packet...
//
// TODO: Right now, all possible properties for all subclasses are handled here. Ideally we'd prefer
//       to handle this in a more generic way. Allowing subclasses of EntityItem to register their properties
//
// TODO: There's a lot of repeated patterns in the code below to handle each property. It would be nice if the property
//       registration mechanism allowed us to collapse these repeated sections of code into a single implementation that
//       utilized the registration table to shorten up and simplify this code.
//
// TODO: Implement support for paged properties, spanning MTU, and custom properties
//
// TODO: Implement support for script and visible properties.
//
bool EntityItemProperties::decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes,
                                                  EntityItemID& entityID, EntityItemProperties& properties) {
    bool valid = false;

    const unsigned char* dataAt = data;
    processedBytes = 0;

    // the first part of the data is an octcode, this is a required element of the edit packet format, but we don't
    // actually use it, we do need to skip it and read to the actual data we care about.
    int octets = numberOfThreeBitSectionsInCode(data);
    int bytesToReadOfOctcode = (int)bytesRequiredForCodeLength(octets);

    // we don't actually do anything with this octcode...
    dataAt += bytesToReadOfOctcode;
    processedBytes += bytesToReadOfOctcode;

    // Edit packets have a last edited time stamp immediately following the octcode.
    // NOTE: the edit times have been set by the editor to match out clock, so we don't need to adjust
    // these times for clock skew at this point.
    quint64 lastEdited;
    memcpy(&lastEdited, dataAt, sizeof(lastEdited));
    dataAt += sizeof(lastEdited);
    processedBytes += sizeof(lastEdited);
    properties.setLastEdited(lastEdited);

    // NOTE: We intentionally do not send "created" times in edit messages. This is because:
    //   1) if the edit is to an existing entity, the created time can not be changed
    //   2) if the edit is to a new entity, the created time is the last edited time

    // encoded id
    QUuid editID = QUuid::fromRfc4122(QByteArray::fromRawData(reinterpret_cast<const char*>(dataAt), NUM_BYTES_RFC4122_UUID));
    dataAt += NUM_BYTES_RFC4122_UUID;
    processedBytes += NUM_BYTES_RFC4122_UUID;

    entityID = editID;
    valid = true;

    // Entity Type...
    QByteArray encodedType((const char*)dataAt, (bytesToRead - processedBytes));
    ByteCountCoded<quint32> typeCoder = encodedType;
    quint32 entityTypeCode = typeCoder;
    properties.setType((EntityTypes::EntityType)entityTypeCode);
    encodedType = typeCoder; // determine true bytesToRead
    dataAt += encodedType.size();
    processedBytes += encodedType.size();

    // Update Delta - when was this item updated relative to last edit... this really should be 0
    // TODO: Should we get rid of this in this in edit packets, since this has to always be 0?
    // TODO: do properties need to handle lastupdated???

    // last updated is stored as ByteCountCoded delta from lastEdited
    QByteArray encodedUpdateDelta((const char*)dataAt, (bytesToRead - processedBytes));
    ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
    encodedUpdateDelta = updateDeltaCoder; // determine true bytesToRead
    dataAt += encodedUpdateDelta.size();
    processedBytes += encodedUpdateDelta.size();

    // TODO: Do we need this lastUpdated?? We don't seem to use it.
    //quint64 updateDelta = updateDeltaCoder;
    //quint64 lastUpdated = lastEdited + updateDelta; // don't adjust for clock skew since we already did that for lastEdited

    // Property Flags...
    QByteArray encodedPropertyFlags((const char*)dataAt, (bytesToRead - processedBytes));
    EntityPropertyFlags propertyFlags = encodedPropertyFlags;
    dataAt += propertyFlags.getEncodedLength();
    processedBytes += propertyFlags.getEncodedLength();

    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SIMULATION_OWNER, QByteArray, setSimulationOwner);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_POSITION, glm::vec3, setPosition);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DIMENSIONS, glm::vec3, setDimensions);  // NOTE: PROP_RADIUS obsolete
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ROTATION, glm::quat, setRotation);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DENSITY, float, setDensity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VELOCITY, glm::vec3, setVelocity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_GRAVITY, glm::vec3, setGravity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ACCELERATION, glm::vec3, setAcceleration);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DAMPING, float, setDamping);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RESTITUTION, float, setRestitution);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_FRICTION, float, setFriction);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LIFETIME, float, setLifetime);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SCRIPT, QString, setScript);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SCRIPT_TIMESTAMP, quint64, setScriptTimestamp);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, xColor, setColor);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_REGISTRATION_POINT, glm::vec3, setRegistrationPoint);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANGULAR_VELOCITY, glm::vec3, setAngularVelocity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANGULAR_DAMPING, float, setAngularDamping);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VISIBLE, bool, setVisible);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLLISIONLESS, bool, setCollisionless);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLLISION_MASK, uint8_t, setCollisionMask);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DYNAMIC, bool, setDynamic);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LOCKED, bool, setLocked);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_USER_DATA, QString, setUserData);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_HREF, QString, setHref);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DESCRIPTION, QString, setDescription);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARENT_ID, QUuid, setParentID);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARENT_JOINT_INDEX, quint16, setParentJointIndex);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_QUERY_AA_CUBE, AACube, setQueryAACube);

    if (properties.getType() == EntityTypes::Web) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SOURCE_URL, QString, setSourceUrl);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DPI, uint16_t, setDPI);
    }

    if (properties.getType() == EntityTypes::Text) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXT, QString, setText);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_HEIGHT, float, setLineHeight);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXT_COLOR, xColor, setTextColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_BACKGROUND_COLOR, xColor, setBackgroundColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_FACE_CAMERA, bool, setFaceCamera);
    }

    if (properties.getType() == EntityTypes::Model) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MODEL_URL, QString, setModelURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXTURES, QString, setTextures);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SHAPE_TYPE, ShapeType, setShapeType);

        properties.getAnimation().decodeFromEditPacket(propertyFlags, dataAt, processedBytes);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_JOINT_ROTATIONS_SET, QVector<bool>, setJointRotationsSet);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_JOINT_ROTATIONS, QVector<glm::quat>, setJointRotations);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_JOINT_TRANSLATIONS_SET, QVector<bool>, setJointTranslationsSet);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_JOINT_TRANSLATIONS, QVector<glm::vec3>, setJointTranslations);
    }

    if (properties.getType() == EntityTypes::Light) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_IS_SPOTLIGHT, bool, setIsSpotlight);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, xColor, setColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_INTENSITY, float, setIntensity);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_FALLOFF_RADIUS, float, setFalloffRadius);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EXPONENT, float, setExponent);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CUTOFF, float, setCutoff);
    }

    if (properties.getType() == EntityTypes::ParticleEffect) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXTURES, QString, setTextures);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MAX_PARTICLES, quint32, setMaxParticles);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LIFESPAN, float, setLifespan);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMITTING_PARTICLES, bool, setIsEmitting);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_RATE, float, setEmitRate);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_SPEED, float, setEmitSpeed);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SPEED_SPREAD, float, setSpeedSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_ORIENTATION, glm::quat, setEmitOrientation);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_DIMENSIONS, glm::vec3, setEmitDimensions);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_RADIUS_START, float, setEmitRadiusStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_POLAR_START, float, setPolarStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_POLAR_FINISH, float, setPolarFinish);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_AZIMUTH_START, float, setAzimuthStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_AZIMUTH_FINISH, float, setAzimuthFinish);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_ACCELERATION, glm::vec3, setEmitAcceleration);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ACCELERATION_SPREAD, glm::vec3, setAccelerationSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARTICLE_RADIUS, float, setParticleRadius);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RADIUS_SPREAD, float, setRadiusSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RADIUS_START, float, setRadiusStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RADIUS_FINISH, float, setRadiusFinish);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR_SPREAD, xColor, setColorSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR_START, xColor, setColorStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR_FINISH, xColor, setColorFinish);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA_SPREAD, float, setAlphaSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA_START, float, setAlphaStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA_FINISH, float, setAlphaFinish);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMITTER_SHOULD_TRAIL, bool, setEmitterShouldTrail);
    }

    if (properties.getType() == EntityTypes::Zone) {
        properties.getKeyLight().decodeFromEditPacket(propertyFlags, dataAt , processedBytes);
        properties.getStage().decodeFromEditPacket(propertyFlags, dataAt , processedBytes);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SHAPE_TYPE, ShapeType, setShapeType);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_BACKGROUND_MODE, BackgroundMode, setBackgroundMode);
        properties.getSkybox().decodeFromEditPacket(propertyFlags, dataAt , processedBytes);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_FLYING_ALLOWED, bool, setFlyingAllowed);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_GHOSTING_ALLOWED, bool, setGhostingAllowed);
    }

    if (properties.getType() == EntityTypes::PolyVox) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VOXEL_VOLUME_SIZE, glm::vec3, setVoxelVolumeSize);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VOXEL_DATA, QByteArray, setVoxelData);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VOXEL_SURFACE_STYLE, uint16_t, setVoxelSurfaceStyle);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_X_TEXTURE_URL, QString, setXTextureURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_Y_TEXTURE_URL, QString, setYTextureURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_Z_TEXTURE_URL, QString, setZTextureURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_X_N_NEIGHBOR_ID, EntityItemID, setXNNeighborID);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_Y_N_NEIGHBOR_ID, EntityItemID, setYNNeighborID);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_Z_N_NEIGHBOR_ID, EntityItemID, setZNNeighborID);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_X_P_NEIGHBOR_ID, EntityItemID, setXPNeighborID);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_Y_P_NEIGHBOR_ID, EntityItemID, setYPNeighborID);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_Z_P_NEIGHBOR_ID, EntityItemID, setZPNeighborID);
    }

    if (properties.getType() == EntityTypes::Line) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_WIDTH, float, setLineWidth);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_POINTS, QVector<glm::vec3>, setLinePoints);
    }


    if (properties.getType() == EntityTypes::PolyLine) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_WIDTH, float, setLineWidth);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_POINTS, QVector<glm::vec3>, setLinePoints);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_NORMALS, QVector<glm::vec3>, setNormals);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_STROKE_WIDTHS, QVector<float>, setStrokeWidths);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXTURES, QString, setTextures);
    }

    // NOTE: Spheres and Boxes are just special cases of Shape, and they need to include their PROP_SHAPE
    // when encoding/decoding edits because otherwise they can't polymorph to other shape types
    if (properties.getType() == EntityTypes::Shape || 
        properties.getType() == EntityTypes::Box ||
        properties.getType() == EntityTypes::Sphere) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SHAPE, QString, setShape);
    }

    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MARKETPLACE_ID, QString, setMarketplaceID);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_NAME, QString, setName);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLLISION_SOUND_URL, QString, setCollisionSoundURL);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ACTION_DATA, QByteArray, setActionData);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA, float, setAlpha);

    return valid;
}


// NOTE: This version will only encode the portion of the edit message immediately following the
// header it does not include the send times and sequence number because that is handled by the
// edit packet sender...
bool EntityItemProperties::encodeEraseEntityMessage(const EntityItemID& entityItemID, QByteArray& buffer) {

    char* copyAt = buffer.data();
    uint16_t numberOfIds = 1; // only one entity ID in this message

    int outputLength = 0;

    if (buffer.size() < (int) (sizeof(numberOfIds) + NUM_BYTES_RFC4122_UUID)) {
        qCDebug(entities) << "ERROR - encodeEraseEntityMessage() called with buffer that is too small!";
        return false;
    }

    memcpy(copyAt, &numberOfIds, sizeof(numberOfIds));
    copyAt += sizeof(numberOfIds);
    outputLength = sizeof(numberOfIds);

    memcpy(copyAt, entityItemID.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
    copyAt += NUM_BYTES_RFC4122_UUID;
    outputLength += NUM_BYTES_RFC4122_UUID;

    buffer.resize(outputLength);

    return true;
}

void EntityItemProperties::markAllChanged() {
    _simulationOwnerChanged = true;
    _positionChanged = true;
    _dimensionsChanged = true;
    _rotationChanged = true;
    _densityChanged = true;
    _velocityChanged = true;
    _gravityChanged = true;
    _accelerationChanged = true;
    _dampingChanged = true;
    _restitutionChanged = true;
    _frictionChanged = true;
    _lifetimeChanged = true;
    _userDataChanged = true;
    _scriptChanged = true;
    _scriptTimestampChanged = true;
    _collisionSoundURLChanged = true;
    _registrationPointChanged = true;
    _angularVelocityChanged = true;
    _angularDampingChanged = true;
    _nameChanged = true;
    _visibleChanged = true;
    _colorChanged = true;
    _alphaChanged = true;
    _modelURLChanged = true;
    _compoundShapeURLChanged = true;
    _localRenderAlphaChanged = true;
    _isSpotlightChanged = true;
    _collisionlessChanged = true;
    _collisionMaskChanged = true;
    _dynamicChanged = true;

    _intensityChanged = true;
    _falloffRadiusChanged = true;
    _exponentChanged = true;
    _cutoffChanged = true;
    _lockedChanged = true;
    _texturesChanged = true;

    _textChanged = true;
    _lineHeightChanged = true;
    _textColorChanged = true;
    _backgroundColorChanged = true;
    _shapeTypeChanged = true;

    _isEmittingChanged = true;
    _maxParticlesChanged = true;
    _lifespanChanged = true;
    _emitRateChanged = true;
    _emitSpeedChanged = true;
    _speedSpreadChanged = true;
    _emitOrientationChanged = true;
    _emitDimensionsChanged = true;
    _emitRadiusStartChanged = true;
    _polarStartChanged = true;
    _polarFinishChanged = true;
    _azimuthStartChanged = true;
    _azimuthFinishChanged = true;
    _emitAccelerationChanged = true;
    _accelerationSpreadChanged = true;
    _particleRadiusChanged = true;
    _radiusSpreadChanged = true;
    _colorSpreadChanged = true;
    _alphaSpreadChanged = true;

    // Only mark the following as changed if their values are specified in the properties when the particle is created. If their
    // values are specified then they are marked as changed in getChangedProperties().
    //_radiusStartChanged = true;
    //_radiusFinishChanged = true;
    //_colorStartChanged = true;
    //_colorFinishChanged = true;
    //_alphaStartChanged = true;
    //_alphaFinishChanged = true;

    _marketplaceIDChanged = true;

    _keyLight.markAllChanged();

    _backgroundModeChanged = true;

    _animation.markAllChanged();
    _skybox.markAllChanged();
    _stage.markAllChanged();

    _sourceUrlChanged = true;
    _voxelVolumeSizeChanged = true;
    _voxelDataChanged = true;
    _voxelSurfaceStyleChanged = true;

    _lineWidthChanged = true;
    _linePointsChanged = true;

    _hrefChanged = true;
    _descriptionChanged = true;
    _faceCameraChanged = true;
    _actionDataChanged = true;

    _normalsChanged = true;
    _strokeWidthsChanged = true;

    _xTextureURLChanged = true;
    _yTextureURLChanged = true;
    _zTextureURLChanged = true;

    _xNNeighborIDChanged = true;
    _yNNeighborIDChanged = true;
    _zNNeighborIDChanged = true;

    _xPNeighborIDChanged = true;
    _yPNeighborIDChanged = true;
    _zPNeighborIDChanged = true;

    _parentIDChanged = true;
    _parentJointIndexChanged = true;

    _jointRotationsSetChanged = true;
    _jointRotationsChanged = true;
    _jointTranslationsSetChanged = true;
    _jointTranslationsChanged = true;

    _queryAACubeChanged = true;

    _flyingAllowedChanged = true;
    _ghostingAllowedChanged = true;

    _clientOnlyChanged = true;
    _owningAvatarIDChanged = true;

    _dpiChanged = true;
}

// The minimum bounding box for the entity.
AABox EntityItemProperties::getAABox() const {

    // _position represents the position of the registration point.
    glm::vec3 registrationRemainder = glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint;

    glm::vec3 unrotatedMinRelativeToEntity = - (_dimensions * _registrationPoint);
    glm::vec3 unrotatedMaxRelativeToEntity = _dimensions * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(_rotation);

    // shift the extents to be relative to the position/registration point
    rotatedExtentsRelativeToRegistrationPoint.shiftBy(_position);

    return AABox(rotatedExtentsRelativeToRegistrationPoint);
}

bool EntityItemProperties::hasTerseUpdateChanges() const {
    // a TerseUpdate includes the transform and its derivatives
    return _positionChanged || _velocityChanged || _rotationChanged || _angularVelocityChanged || _accelerationChanged;
}

bool EntityItemProperties::hasMiscPhysicsChanges() const {
    return _gravityChanged || _dimensionsChanged || _densityChanged || _frictionChanged
        || _restitutionChanged || _dampingChanged || _angularDampingChanged || _registrationPointChanged ||
        _compoundShapeURLChanged || _dynamicChanged || _collisionlessChanged || _collisionMaskChanged;
}

void EntityItemProperties::clearSimulationOwner() {
    _simulationOwner.clear();
    _simulationOwnerChanged = true;
}

void EntityItemProperties::setSimulationOwner(const QUuid& id, uint8_t priority) {
    if (!_simulationOwner.matchesValidID(id) || _simulationOwner.getPriority() != priority) {
        _simulationOwner.set(id, priority);
        _simulationOwnerChanged = true;
    }
}

void EntityItemProperties::setSimulationOwner(const QByteArray& data) {
    if (_simulationOwner.fromByteArray(data)) {
        _simulationOwnerChanged = true;
    }
}

QList<QString> EntityItemProperties::listChangedProperties() {
    QList<QString> out;
    if (containsPositionChange()) {
        out += "position";
    }
    if (dimensionsChanged()) {
        out += "dimensions";
    }
    if (velocityChanged()) {
        out += "velocity";
    }
    if (nameChanged()) {
        out += "name";
    }
    if (visibleChanged()) {
        out += "visible";
    }
    if (rotationChanged()) {
        out += "rotation";
    }
    if (densityChanged()) {
        out += "density";
    }
    if (gravityChanged()) {
        out += "gravity";
    }
    if (accelerationChanged()) {
        out += "acceleration";
    }
    if (dampingChanged()) {
        out += "damping";
    }
    if (restitutionChanged()) {
        out += "restitution";
    }
    if (frictionChanged()) {
        out += "friction";
    }
    if (lifetimeChanged()) {
        out += "lifetime";
    }
    if (scriptChanged()) {
        out += "script";
    }
    if (scriptTimestampChanged()) {
        out += "scriptTimestamp";
    }
    if (collisionSoundURLChanged()) {
        out += "collisionSoundURL";
    }
    if (colorChanged()) {
        out += "color";
    }
    if (colorSpreadChanged()) {
        out += "colorSpread";
    }
    if (colorStartChanged()) {
        out += "colorStart";
    }
    if (colorFinishChanged()) {
        out += "colorFinish";
    }
    if (alphaChanged()) {
        out += "alpha";
    }
    if (alphaSpreadChanged()) {
        out += "alphaSpread";
    }
    if (alphaStartChanged()) {
        out += "alphaStart";
    }
    if (alphaFinishChanged()) {
        out += "alphaFinish";
    }
    if (emitterShouldTrailChanged()) {
        out += "emitterShouldTrail";
    }
    if (modelURLChanged()) {
        out += "modelURL";
    }
    if (compoundShapeURLChanged()) {
        out += "compoundShapeURL";
    }
    if (registrationPointChanged()) {
        out += "registrationPoint";
    }
    if (angularVelocityChanged()) {
        out += "angularVelocity";
    }
    if (angularDampingChanged()) {
        out += "angularDamping";
    }
    if (collisionlessChanged()) {
        out += "collisionless";
    }
    if (collisionMaskChanged()) {
        out += "collisionMask";
    }
    if (dynamicChanged()) {
        out += "dynamic";
    }
    if (isSpotlightChanged()) {
        out += "isSpotlight";
    }
    if (intensityChanged()) {
        out += "intensity";
    }
    if (falloffRadiusChanged()) {
        out += "falloffRadius";
    }
    if (exponentChanged()) {
        out += "exponent";
    }
    if (cutoffChanged()) {
        out += "cutoff";
    }
    if (lockedChanged()) {
        out += "locked";
    }
    if (texturesChanged()) {
        out += "textures";
    }
    if (userDataChanged()) {
        out += "userData";
    }
    if (simulationOwnerChanged()) {
        out += "simulationOwner";
    }
    if (textChanged()) {
        out += "text";
    }
    if (lineHeightChanged()) {
        out += "lineHeight";
    }
    if (textColorChanged()) {
        out += "textColor";
    }
    if (backgroundColorChanged()) {
        out += "backgroundColor";
    }
    if (shapeTypeChanged()) {
        out += "shapeType";
    }
    if (maxParticlesChanged()) {
        out += "maxParticles";
    }
    if (lifespanChanged()) {
        out += "lifespan";
    }
    if (isEmittingChanged()) {
        out += "isEmitting";
    }
    if (emitRateChanged()) {
        out += "emitRate";
    }
    if (emitSpeedChanged()) {
        out += "emitSpeed";
    }
    if (speedSpreadChanged()) {
        out += "speedSpread";
    }
    if (emitOrientationChanged()) {
        out += "emitOrientation";
    }
    if (emitDimensionsChanged()) {
        out += "emitDimensions";
    }
    if (emitRadiusStartChanged()) {
        out += "emitRadiusStart";
    }
    if (polarStartChanged()) {
        out += "polarStart";
    }
    if (polarFinishChanged()) {
        out += "polarFinish";
    }
    if (azimuthStartChanged()) {
        out += "azimuthStart";
    }
    if (azimuthFinishChanged()) {
        out += "azimuthFinish";
    }
    if (emitAccelerationChanged()) {
        out += "emitAcceleration";
    }
    if (accelerationSpreadChanged()) {
        out += "accelerationSpread";
    }
    if (particleRadiusChanged()) {
        out += "particleRadius";
    }
    if (radiusSpreadChanged()) {
        out += "radiusSpread";
    }
    if (radiusStartChanged()) {
        out += "radiusStart";
    }
    if (radiusFinishChanged()) {
        out += "radiusFinish";
    }
    if (marketplaceIDChanged()) {
        out += "marketplaceID";
    }
    if (backgroundModeChanged()) {
        out += "backgroundMode";
    }
    if (voxelVolumeSizeChanged()) {
        out += "voxelVolumeSize";
    }
    if (voxelDataChanged()) {
        out += "voxelData";
    }
    if (voxelSurfaceStyleChanged()) {
        out += "voxelSurfaceStyle";
    }
    if (hrefChanged()) {
        out += "href";
    }
    if (descriptionChanged()) {
        out += "description";
    }
    if (actionDataChanged()) {
        out += "actionData";
    }
    if (xTextureURLChanged()) {
        out += "xTextureURL";
    }
    if (yTextureURLChanged()) {
        out += "yTextureURL";
    }
    if (zTextureURLChanged()) {
        out += "zTextureURL";
    }
    if (xNNeighborIDChanged()) {
        out += "xNNeighborID";
    }
    if (yNNeighborIDChanged()) {
        out += "yNNeighborID";
    }
    if (zNNeighborIDChanged()) {
        out += "zNNeighborID";
    }
    if (xPNeighborIDChanged()) {
        out += "xPNeighborID";
    }
    if (yPNeighborIDChanged()) {
        out += "yPNeighborID";
    }
    if (zPNeighborIDChanged()) {
        out += "zPNeighborID";
    }
    if (parentIDChanged()) {
        out += "parentID";
    }
    if (parentJointIndexChanged()) {
        out += "parentJointIndex";
    }
    if (jointRotationsSetChanged()) {
        out += "jointRotationsSet";
    }
    if (jointRotationsChanged()) {
        out += "jointRotations";
    }
    if (jointTranslationsSetChanged()) {
        out += "jointTranslationsSet";
    }
    if (jointTranslationsChanged()) {
        out += "jointTranslations";
    }
    if (queryAACubeChanged()) {
        out += "queryAACube";
    }

    if (clientOnlyChanged()) {
        out += "clientOnly";
    }
    if (owningAvatarIDChanged()) {
        out += "owningAvatarID";
    }

    if (flyingAllowedChanged()) {
        out += "flyingAllowed";
    }
    if (ghostingAllowedChanged()) {
        out += "ghostingAllowed";
    }

    if (dpiChanged()) {
        out += "dpi";
    }

    if (shapeChanged()) {
        out += "shape";
    }

    getAnimation().listChangedProperties(out);
    getKeyLight().listChangedProperties(out);
    getSkybox().listChangedProperties(out);
    getStage().listChangedProperties(out);

    return out;
}

bool EntityItemProperties::parentDependentPropertyChanged() const {
    return localPositionChanged() || positionChanged() ||
        localRotationChanged() || rotationChanged() ||
        localVelocityChanged() || localAngularVelocityChanged();
}

bool EntityItemProperties::parentRelatedPropertyChanged() const {
    return parentDependentPropertyChanged() || parentIDChanged() || parentJointIndexChanged();
}
