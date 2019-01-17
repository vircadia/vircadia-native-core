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

#include "EntityItemProperties.h"

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/ecdsa.h>

#include <QDebug>
#include <QHash>
#include <QObject>
#include <QtCore/QJsonDocument>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

#include <NetworkAccessManager.h>
#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <RegisteredMetaTypes.h>
#include <Extents.h>
#include <VariantMapToScriptValue.h>

#include "EntitiesLogging.h"
#include "EntityItem.h"
#include "ModelEntityItem.h"
#include "PolyLineEntityItem.h"

AnimationPropertyGroup EntityItemProperties::_staticAnimation;
SkyboxPropertyGroup EntityItemProperties::_staticSkybox;
HazePropertyGroup EntityItemProperties::_staticHaze;
BloomPropertyGroup EntityItemProperties::_staticBloom;
KeyLightPropertyGroup EntityItemProperties::_staticKeyLight;
AmbientLightPropertyGroup EntityItemProperties::_staticAmbientLight;
GrabPropertyGroup EntityItemProperties::_staticGrab;

EntityPropertyList PROP_LAST_ITEM = (EntityPropertyList)(PROP_AFTER_LAST_ITEM - 1);

EntityItemProperties::EntityItemProperties(EntityPropertyFlags desiredProperties) :
    _id(UNKNOWN_ENTITY_ID),
    _idSet(false),
    _lastEdited(0),
    _type(EntityTypes::Unknown),

    _defaultSettings(true),
    _naturalDimensions(1.0f, 1.0f, 1.0f),
    _naturalPosition(0.0f, 0.0f, 0.0f),
    _desiredProperties(desiredProperties)
{

}

void EntityItemProperties::calculateNaturalPosition(const vec3& min, const vec3& max) {
    vec3 halfDimension = (max - min) / 2.0f;
    _naturalPosition = max - halfDimension;
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
    getHaze().debugDump();
    getKeyLight().debugDump();
    getAmbientLight().debugDump();
    getBloom().debugDump();
    getGrab().debugDump();

    qCDebug(entities) << "   changed properties...";
    EntityPropertyFlags props = getChangedProperties();
    props.debugDumpBits();
}

void EntityItemProperties::setLastEdited(quint64 usecTime) {
    _lastEdited = usecTime > _created ? usecTime : _created;
}

bool EntityItemProperties::constructFromBuffer(const unsigned char* data, int dataLength) {
    ReadBitstreamToTreeParams args;
    EntityItemPointer tempEntity = EntityTypes::constructEntityItem(data, dataLength);
    if (!tempEntity) {
        return false;
    }
    tempEntity->readEntityDataFromBuffer(data, dataLength, args);
    (*this) = tempEntity->getProperties();
    return true;
}

QHash<QString, ShapeType> stringToShapeTypeLookup;

void addShapeType(ShapeType type) {
    stringToShapeTypeLookup[ShapeInfo::getNameForShapeType(type)] = type;
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
    addShapeType(SHAPE_TYPE_ELLIPSOID);
}

QHash<QString, MaterialMappingMode> stringToMaterialMappingModeLookup;

void addMaterialMappingMode(MaterialMappingMode mode) {
    stringToMaterialMappingModeLookup[MaterialMappingModeHelpers::getNameForMaterialMappingMode(mode)] = mode;
}

void buildStringToMaterialMappingModeLookup() {
    addMaterialMappingMode(UV);
    addMaterialMappingMode(PROJECTED);
}

QString getCollisionGroupAsString(uint16_t group) {
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

uint16_t getCollisionGroupAsBitMask(const QStringRef& name) {
    if (0 == name.compare(QString("dynamic"))) {
        return USER_COLLISION_GROUP_DYNAMIC;
    } else if (0 == name.compare(QString("static"))) {
        return USER_COLLISION_GROUP_STATIC;
    } else if (0 == name.compare(QString("kinematic"))) {
        return USER_COLLISION_GROUP_KINEMATIC;
    } else if (0 == name.compare(QString("myAvatar"))) {
        return USER_COLLISION_GROUP_MY_AVATAR;
    } else if (0 == name.compare(QString("otherAvatar"))) {
        return USER_COLLISION_GROUP_OTHER_AVATAR;
    }
    return 0;
}

QString EntityItemProperties::getCollisionMaskAsString() const {
    QString maskString("");
    for (int i = 0; i < NUM_USER_COLLISION_GROUPS; ++i) {
        uint16_t group = 0x0001 << i;
        if (group & _collisionMask) {
            maskString.append(getCollisionGroupAsString(group));
            maskString.append(',');
        }
    }
    return maskString;
}

void EntityItemProperties::setCollisionMaskFromString(const QString& maskString) {
    QVector<QStringRef> groups = maskString.splitRef(',');
    uint16_t mask = 0x0000;
    for (auto groupName : groups) {
        mask |= getCollisionGroupAsBitMask(groupName);
    }
    _collisionMask = mask;
    _collisionMaskChanged = true;
}

QString EntityItemProperties::getShapeTypeAsString() const {
    return ShapeInfo::getNameForShapeType(_shapeType);
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

QString EntityItemProperties::getComponentModeAsString(uint32_t mode) {
    // return "inherit" if mode is not valid
    if (mode < COMPONENT_MODE_ITEM_COUNT) {
        return COMPONENT_MODES[mode].second;
    } else {
        return COMPONENT_MODES[COMPONENT_MODE_INHERIT].second;
    }
}

QString EntityItemProperties::getHazeModeAsString() const {
    return getComponentModeAsString(_hazeMode);
}

QString EntityItemProperties::getBloomModeAsString() const {
    return getComponentModeAsString(_bloomMode);
}

std::array<ComponentPair, COMPONENT_MODE_ITEM_COUNT>::const_iterator EntityItemProperties::findComponent(const QString& mode) {
    return std::find_if(COMPONENT_MODES.begin(), COMPONENT_MODES.end(), [&](const ComponentPair& pair) { 
        return (pair.second == mode);
    });
}

void EntityItemProperties::setHazeModeFromString(const QString& hazeMode) {
    auto result = findComponent(hazeMode);

    if (result != COMPONENT_MODES.end()) {
        _hazeMode = result->first;
        _hazeModeChanged = true;
    }
}

void EntityItemProperties::setBloomModeFromString(const QString& bloomMode) {
    auto result = findComponent(bloomMode);

    if (result != COMPONENT_MODES.end()) {
        _bloomMode = result->first;
        _bloomModeChanged = true;
    }
}

QString EntityItemProperties::getKeyLightModeAsString() const {
    return getComponentModeAsString(_keyLightMode);
}

void EntityItemProperties::setKeyLightModeFromString(const QString& keyLightMode) {
    auto result = findComponent(keyLightMode);

    if (result != COMPONENT_MODES.end()) {
        _keyLightMode = result->first;
        _keyLightModeChanged = true;
    }
}

QString EntityItemProperties::getAmbientLightModeAsString() const {
    return getComponentModeAsString(_ambientLightMode);
}

void EntityItemProperties::setAmbientLightModeFromString(const QString& ambientLightMode) {
    auto result = findComponent(ambientLightMode);

    if (result != COMPONENT_MODES.end()) {
        _ambientLightMode = result->first;
        _ambientLightModeChanged = true;
    }
}

QString EntityItemProperties::getSkyboxModeAsString() const {
    return getComponentModeAsString(_skyboxMode);
}

void EntityItemProperties::setSkyboxModeFromString(const QString& skyboxMode) {
    auto result = findComponent(skyboxMode);

    if (result != COMPONENT_MODES.end()) {
        _skyboxMode = result->first;
        _skyboxModeChanged = true;
    }
}

QString EntityItemProperties::getMaterialMappingModeAsString() const {
    return MaterialMappingModeHelpers::getNameForMaterialMappingMode(_materialMappingMode);
}

void EntityItemProperties::setMaterialMappingModeFromString(const QString& materialMappingMode) {
    if (stringToMaterialMappingModeLookup.empty()) {
        buildStringToMaterialMappingModeLookup();
    }
    auto materialMappingModeItr = stringToMaterialMappingModeLookup.find(materialMappingMode.toLower());
    if (materialMappingModeItr != stringToMaterialMappingModeLookup.end()) {
        _materialMappingMode = materialMappingModeItr.value();
        _materialMappingModeChanged = true;
    }
}

QString EntityItemProperties::getEntityHostTypeAsString() const {
    switch (_entityHostType) {
        case entity::HostType::DOMAIN:
            return "domain";
        case entity::HostType::AVATAR:
            return "avatar";
        case entity::HostType::LOCAL:
            return "local";
        default:
            return "";
    }
}

void EntityItemProperties::setEntityHostTypeFromString(const QString& entityHostType) {
    if (entityHostType == "domain") {
        _entityHostType = entity::HostType::DOMAIN;
    } else if (entityHostType == "avatar") {
        _entityHostType = entity::HostType::AVATAR;
    } else if (entityHostType == "local") {
        _entityHostType = entity::HostType::LOCAL;
    }
}

QHash<QString, BillboardMode> stringToBillboardModeLookup;

void addBillboardMode(BillboardMode mode) {
    stringToBillboardModeLookup[BillboardModeHelpers::getNameForBillboardMode(mode)] = mode;
}

void buildStringToBillboardModeLookup() {
    addBillboardMode(BillboardMode::NONE);
    addBillboardMode(BillboardMode::YAW);
    addBillboardMode(BillboardMode::FULL);
}

QString EntityItemProperties::getBillboardModeAsString() const {
    return BillboardModeHelpers::getNameForBillboardMode(_billboardMode);
}

void EntityItemProperties::setBillboardModeFromString(const QString& materialMappingMode) {
    if (stringToBillboardModeLookup.empty()) {
        buildStringToBillboardModeLookup();
    }
    auto billboardModeItr = stringToBillboardModeLookup.find(materialMappingMode.toLower());
    if (billboardModeItr != stringToBillboardModeLookup.end()) {
        _billboardMode = billboardModeItr.value();
        _billboardModeChanged = true;
    }
}

EntityPropertyFlags EntityItemProperties::getChangedProperties() const {
    EntityPropertyFlags changedProperties;

    // Core
    CHECK_PROPERTY_CHANGE(PROP_SIMULATION_OWNER, simulationOwner);
    CHECK_PROPERTY_CHANGE(PROP_VISIBLE, visible);
    CHECK_PROPERTY_CHANGE(PROP_NAME, name);
    CHECK_PROPERTY_CHANGE(PROP_LOCKED, locked);
    CHECK_PROPERTY_CHANGE(PROP_USER_DATA, userData);
    CHECK_PROPERTY_CHANGE(PROP_HREF, href);
    CHECK_PROPERTY_CHANGE(PROP_DESCRIPTION, description);
    CHECK_PROPERTY_CHANGE(PROP_POSITION, position);
    CHECK_PROPERTY_CHANGE(PROP_DIMENSIONS, dimensions);
    CHECK_PROPERTY_CHANGE(PROP_ROTATION, rotation);
    CHECK_PROPERTY_CHANGE(PROP_REGISTRATION_POINT, registrationPoint);
    CHECK_PROPERTY_CHANGE(PROP_CREATED, created);
    CHECK_PROPERTY_CHANGE(PROP_LAST_EDITED_BY, lastEditedBy);
    CHECK_PROPERTY_CHANGE(PROP_ENTITY_HOST_TYPE, entityHostType);
    CHECK_PROPERTY_CHANGE(PROP_OWNING_AVATAR_ID, owningAvatarID);
    CHECK_PROPERTY_CHANGE(PROP_PARENT_ID, parentID);
    CHECK_PROPERTY_CHANGE(PROP_PARENT_JOINT_INDEX, parentJointIndex);
    CHECK_PROPERTY_CHANGE(PROP_QUERY_AA_CUBE, queryAACube);
    CHECK_PROPERTY_CHANGE(PROP_CAN_CAST_SHADOW, canCastShadow);
    CHECK_PROPERTY_CHANGE(PROP_VISIBLE_IN_SECONDARY_CAMERA, isVisibleInSecondaryCamera);
    changedProperties += _grab.getChangedProperties();

    // Physics
    CHECK_PROPERTY_CHANGE(PROP_DENSITY, density);
    CHECK_PROPERTY_CHANGE(PROP_VELOCITY, velocity);
    CHECK_PROPERTY_CHANGE(PROP_ANGULAR_VELOCITY, angularVelocity);
    CHECK_PROPERTY_CHANGE(PROP_GRAVITY, gravity);
    CHECK_PROPERTY_CHANGE(PROP_ACCELERATION, acceleration);
    CHECK_PROPERTY_CHANGE(PROP_DAMPING, damping);
    CHECK_PROPERTY_CHANGE(PROP_ANGULAR_DAMPING, angularDamping);
    CHECK_PROPERTY_CHANGE(PROP_RESTITUTION, restitution);
    CHECK_PROPERTY_CHANGE(PROP_FRICTION, friction);
    CHECK_PROPERTY_CHANGE(PROP_LIFETIME, lifetime);
    CHECK_PROPERTY_CHANGE(PROP_COLLISIONLESS, collisionless);
    CHECK_PROPERTY_CHANGE(PROP_COLLISION_MASK, collisionMask);
    CHECK_PROPERTY_CHANGE(PROP_DYNAMIC, dynamic);
    CHECK_PROPERTY_CHANGE(PROP_COLLISION_SOUND_URL, collisionSoundURL);
    CHECK_PROPERTY_CHANGE(PROP_ACTION_DATA, actionData);

    // Cloning
    CHECK_PROPERTY_CHANGE(PROP_CLONEABLE, cloneable);
    CHECK_PROPERTY_CHANGE(PROP_CLONE_LIFETIME, cloneLifetime);
    CHECK_PROPERTY_CHANGE(PROP_CLONE_LIMIT, cloneLimit);
    CHECK_PROPERTY_CHANGE(PROP_CLONE_DYNAMIC, cloneDynamic);
    CHECK_PROPERTY_CHANGE(PROP_CLONE_AVATAR_ENTITY, cloneAvatarEntity);
    CHECK_PROPERTY_CHANGE(PROP_CLONE_ORIGIN_ID, cloneOriginID);

    // Scripts
    CHECK_PROPERTY_CHANGE(PROP_SCRIPT, script);
    CHECK_PROPERTY_CHANGE(PROP_SCRIPT_TIMESTAMP, scriptTimestamp);
    CHECK_PROPERTY_CHANGE(PROP_SERVER_SCRIPTS, serverScripts);

    // Certifiable Properties
    CHECK_PROPERTY_CHANGE(PROP_ITEM_NAME, itemName);
    CHECK_PROPERTY_CHANGE(PROP_ITEM_DESCRIPTION, itemDescription);
    CHECK_PROPERTY_CHANGE(PROP_ITEM_CATEGORIES, itemCategories);
    CHECK_PROPERTY_CHANGE(PROP_ITEM_ARTIST, itemArtist);
    CHECK_PROPERTY_CHANGE(PROP_ITEM_LICENSE, itemLicense);
    CHECK_PROPERTY_CHANGE(PROP_LIMITED_RUN, limitedRun);
    CHECK_PROPERTY_CHANGE(PROP_MARKETPLACE_ID, marketplaceID);
    CHECK_PROPERTY_CHANGE(PROP_EDITION_NUMBER, editionNumber);
    CHECK_PROPERTY_CHANGE(PROP_ENTITY_INSTANCE_NUMBER, entityInstanceNumber);
    CHECK_PROPERTY_CHANGE(PROP_CERTIFICATE_ID, certificateID);
    CHECK_PROPERTY_CHANGE(PROP_STATIC_CERTIFICATE_VERSION, staticCertificateVersion);

    // Location data for scripts
    CHECK_PROPERTY_CHANGE(PROP_LOCAL_POSITION, localPosition);
    CHECK_PROPERTY_CHANGE(PROP_LOCAL_ROTATION, localRotation);
    CHECK_PROPERTY_CHANGE(PROP_LOCAL_VELOCITY, localVelocity);
    CHECK_PROPERTY_CHANGE(PROP_LOCAL_ANGULAR_VELOCITY, localAngularVelocity);
    CHECK_PROPERTY_CHANGE(PROP_LOCAL_DIMENSIONS, localDimensions);

    // Common
    CHECK_PROPERTY_CHANGE(PROP_SHAPE_TYPE, shapeType);
    CHECK_PROPERTY_CHANGE(PROP_COMPOUND_SHAPE_URL, compoundShapeURL);
    CHECK_PROPERTY_CHANGE(PROP_COLOR, color);
    CHECK_PROPERTY_CHANGE(PROP_ALPHA, alpha);
    CHECK_PROPERTY_CHANGE(PROP_TEXTURES, textures);

    // Particles
    CHECK_PROPERTY_CHANGE(PROP_MAX_PARTICLES, maxParticles);
    CHECK_PROPERTY_CHANGE(PROP_LIFESPAN, lifespan);
    CHECK_PROPERTY_CHANGE(PROP_EMITTING_PARTICLES, isEmitting);
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
    CHECK_PROPERTY_CHANGE(PROP_COLOR_SPREAD, colorSpread);
    CHECK_PROPERTY_CHANGE(PROP_COLOR_START, colorStart);
    CHECK_PROPERTY_CHANGE(PROP_COLOR_FINISH, colorFinish);
    CHECK_PROPERTY_CHANGE(PROP_ALPHA_SPREAD, alphaSpread);
    CHECK_PROPERTY_CHANGE(PROP_ALPHA_START, alphaStart);
    CHECK_PROPERTY_CHANGE(PROP_ALPHA_FINISH, alphaFinish);
    CHECK_PROPERTY_CHANGE(PROP_EMITTER_SHOULD_TRAIL, emitterShouldTrail);
    CHECK_PROPERTY_CHANGE(PROP_PARTICLE_SPIN, particleSpin);
    CHECK_PROPERTY_CHANGE(PROP_SPIN_SPREAD, spinSpread);
    CHECK_PROPERTY_CHANGE(PROP_SPIN_START, spinStart);
    CHECK_PROPERTY_CHANGE(PROP_SPIN_FINISH, spinFinish);
    CHECK_PROPERTY_CHANGE(PROP_PARTICLE_ROTATE_WITH_ENTITY, rotateWithEntity);

    // Model
    CHECK_PROPERTY_CHANGE(PROP_MODEL_URL, modelURL);
    CHECK_PROPERTY_CHANGE(PROP_JOINT_ROTATIONS_SET, jointRotationsSet);
    CHECK_PROPERTY_CHANGE(PROP_JOINT_ROTATIONS, jointRotations);
    CHECK_PROPERTY_CHANGE(PROP_JOINT_TRANSLATIONS_SET, jointTranslationsSet);
    CHECK_PROPERTY_CHANGE(PROP_JOINT_TRANSLATIONS, jointTranslations);
    CHECK_PROPERTY_CHANGE(PROP_RELAY_PARENT_JOINTS, relayParentJoints);
    changedProperties += _animation.getChangedProperties();

    // Light
    CHECK_PROPERTY_CHANGE(PROP_IS_SPOTLIGHT, isSpotlight);
    CHECK_PROPERTY_CHANGE(PROP_INTENSITY, intensity);
    CHECK_PROPERTY_CHANGE(PROP_EXPONENT, exponent);
    CHECK_PROPERTY_CHANGE(PROP_CUTOFF, cutoff);
    CHECK_PROPERTY_CHANGE(PROP_FALLOFF_RADIUS, falloffRadius);

    // Text
    CHECK_PROPERTY_CHANGE(PROP_TEXT, text);
    CHECK_PROPERTY_CHANGE(PROP_LINE_HEIGHT, lineHeight);
    CHECK_PROPERTY_CHANGE(PROP_TEXT_COLOR, textColor);
    CHECK_PROPERTY_CHANGE(PROP_TEXT_ALPHA, textAlpha);
    CHECK_PROPERTY_CHANGE(PROP_BACKGROUND_COLOR, backgroundColor);
    CHECK_PROPERTY_CHANGE(PROP_BACKGROUND_ALPHA, backgroundAlpha);
    CHECK_PROPERTY_CHANGE(PROP_BILLBOARD_MODE, billboardMode);
    CHECK_PROPERTY_CHANGE(PROP_LEFT_MARGIN, leftMargin);
    CHECK_PROPERTY_CHANGE(PROP_RIGHT_MARGIN, rightMargin);
    CHECK_PROPERTY_CHANGE(PROP_TOP_MARGIN, topMargin);
    CHECK_PROPERTY_CHANGE(PROP_BOTTOM_MARGIN, bottomMargin);

    // Zone
    changedProperties += _keyLight.getChangedProperties();
    changedProperties += _ambientLight.getChangedProperties();
    changedProperties += _skybox.getChangedProperties();
    changedProperties += _haze.getChangedProperties();
    changedProperties += _bloom.getChangedProperties();
    CHECK_PROPERTY_CHANGE(PROP_FLYING_ALLOWED, flyingAllowed);
    CHECK_PROPERTY_CHANGE(PROP_GHOSTING_ALLOWED, ghostingAllowed);
    CHECK_PROPERTY_CHANGE(PROP_FILTER_URL, filterURL);
    CHECK_PROPERTY_CHANGE(PROP_KEY_LIGHT_MODE, keyLightMode);
    CHECK_PROPERTY_CHANGE(PROP_AMBIENT_LIGHT_MODE, ambientLightMode);
    CHECK_PROPERTY_CHANGE(PROP_SKYBOX_MODE, skyboxMode);
    CHECK_PROPERTY_CHANGE(PROP_HAZE_MODE, hazeMode);
    CHECK_PROPERTY_CHANGE(PROP_BLOOM_MODE, bloomMode);

    // Polyvox
    CHECK_PROPERTY_CHANGE(PROP_VOXEL_VOLUME_SIZE, voxelVolumeSize);
    CHECK_PROPERTY_CHANGE(PROP_VOXEL_DATA, voxelData);
    CHECK_PROPERTY_CHANGE(PROP_VOXEL_SURFACE_STYLE, voxelSurfaceStyle);
    CHECK_PROPERTY_CHANGE(PROP_X_TEXTURE_URL, xTextureURL);
    CHECK_PROPERTY_CHANGE(PROP_Y_TEXTURE_URL, yTextureURL);
    CHECK_PROPERTY_CHANGE(PROP_Z_TEXTURE_URL, zTextureURL);
    CHECK_PROPERTY_CHANGE(PROP_X_N_NEIGHBOR_ID, xNNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_Y_N_NEIGHBOR_ID, yNNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_Z_N_NEIGHBOR_ID, zNNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_X_P_NEIGHBOR_ID, xPNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_Y_P_NEIGHBOR_ID, yPNeighborID);
    CHECK_PROPERTY_CHANGE(PROP_Z_P_NEIGHBOR_ID, zPNeighborID);

    // Web
    CHECK_PROPERTY_CHANGE(PROP_SOURCE_URL, sourceUrl);
    CHECK_PROPERTY_CHANGE(PROP_DPI, dpi);

    // Polyline
    CHECK_PROPERTY_CHANGE(PROP_LINE_POINTS, linePoints);
    CHECK_PROPERTY_CHANGE(PROP_STROKE_WIDTHS, strokeWidths);
    CHECK_PROPERTY_CHANGE(PROP_STROKE_NORMALS, normals);
    CHECK_PROPERTY_CHANGE(PROP_STROKE_COLORS, strokeColors);
    CHECK_PROPERTY_CHANGE(PROP_IS_UV_MODE_STRETCH, isUVModeStretch);
    CHECK_PROPERTY_CHANGE(PROP_LINE_GLOW, glow);
    CHECK_PROPERTY_CHANGE(PROP_LINE_FACE_CAMERA, faceCamera);

    // Shape
    CHECK_PROPERTY_CHANGE(PROP_SHAPE, shape);

    // Material
    CHECK_PROPERTY_CHANGE(PROP_MATERIAL_URL, materialURL);
    CHECK_PROPERTY_CHANGE(PROP_MATERIAL_MAPPING_MODE, materialMappingMode);
    CHECK_PROPERTY_CHANGE(PROP_MATERIAL_PRIORITY, priority);
    CHECK_PROPERTY_CHANGE(PROP_PARENT_MATERIAL_NAME, parentMaterialName);
    CHECK_PROPERTY_CHANGE(PROP_MATERIAL_MAPPING_POS, materialMappingPos);
    CHECK_PROPERTY_CHANGE(PROP_MATERIAL_MAPPING_SCALE, materialMappingScale);
    CHECK_PROPERTY_CHANGE(PROP_MATERIAL_MAPPING_ROT, materialMappingRot);
    CHECK_PROPERTY_CHANGE(PROP_MATERIAL_DATA, materialData);
    CHECK_PROPERTY_CHANGE(PROP_MATERIAL_REPEAT, materialRepeat);

    // Image
    CHECK_PROPERTY_CHANGE(PROP_IMAGE_URL, imageURL);
    CHECK_PROPERTY_CHANGE(PROP_EMISSIVE, emissive);
    CHECK_PROPERTY_CHANGE(PROP_KEEP_ASPECT_RATIO, keepAspectRatio);
    CHECK_PROPERTY_CHANGE(PROP_SUB_IMAGE, subImage);

    // Grid
    CHECK_PROPERTY_CHANGE(PROP_GRID_FOLLOW_CAMERA, followCamera);
    CHECK_PROPERTY_CHANGE(PROP_MAJOR_GRID_EVERY, majorGridEvery);
    CHECK_PROPERTY_CHANGE(PROP_MINOR_GRID_EVERY, minorGridEvery);

    return changedProperties;
}

/**jsdoc
 * Different entity types have different properties: some common to all entities (listed below) and some specific to each 
 * {@link Entities.EntityType|EntityType} (linked to below). The properties are accessed as an object of property names and 
 * values.
 *
 * @typedef {object} Entities.EntityProperties
 * @property {Uuid} id - The ID of the entity. <em>Read-only.</em>
 * @property {string} name="" - A name for the entity. Need not be unique.
 * @property {Entities.EntityType} type - The entity type. You cannot change the type of an entity after it's created. (Though 
 *     its value may switch among <code>"Box"</code>, <code>"Shape"</code>, and <code>"Sphere"</code> depending on changes to 
 *     the <code>shape</code> property set for entities of these types.) <em>Read-only.</em>
 * @property {EntityHostType} entityHostType="domain" - How this entity will behave, including if and how it is sent to other people.
 *     The value can only be set at entity creation by using the <code>entityHostType</code> parameter in
 *     {@link Entities.addEntity}.
 * @property {boolean} avatarEntity=false - If <code>true</code> then the entity is an avatar entity;  An avatar entity follows you to each domain you visit,
 *     rendering at the same world coordinates unless it's parented to your avatar. <em>Value cannot be changed after the entity is created.</em><br />
 *     The value can only be set at entity creation by using the <code>entityHostType</code> parameter in 
 *     {@link Entities.addEntity}.  <code>clientOnly</code> is an alias.
 * @property {boolean} localEntity=false - If <code>true</code> then the entity is a local entity;  Local entities only render for you and are not sent over the wire.
 *     <em>Value cannot be changed after the entity is created.</em><br />
 *     The value can only be set at entity creation by using the <code>entityHostType</code> parameter in
 *     {@link Entities.addEntity}.
 * @property {Uuid} owningAvatarID=Uuid.NULL - The session ID of the owning avatar if <code>avatarEntity</code> is 
 *     <code>true</code>, otherwise {@link Uuid|Uuid.NULL}. <em>Read-only.</em>
 *
 * @property {string} created - The UTC date and time that the entity was created, in ISO 8601 format as
 *     <code>yyyy-MM-ddTHH:mm:ssZ</code>. <em>Read-only.</em>
 * @property {number} age - The age of the entity in seconds since it was created. <em>Read-only.</em>
 * @property {string} ageAsText - The age of the entity since it was created, formatted as <code>h hours m minutes s 
 *     seconds</code>.
 * @property {number} lifetime=-1 - How long an entity lives for, in seconds, before being automatically deleted. A value of
 *     <code>-1</code> means that the entity lives for ever.
 * @property {number} lastEdited - When the entity was last edited, expressed as the number of microseconds since
 *     1970-01-01T00:00:00 UTC. <em>Read-only.</em>
 * @property {Uuid} lastEditedBy - The session ID of the avatar or agent that most recently created or edited the entity.
 *     <em>Read-only.</em>
 *
 * @property {boolean} locked=false - Whether or not the entity can be edited or deleted. If <code>true</code> then the 
 *     entity's properties other than <code>locked</code> cannot be changed, and the entity cannot be deleted.
 * @property {boolean} visible=true - Whether or not the entity is rendered. If <code>true</code> then the entity is rendered.
 * @property {boolean} canCastShadow=true - Whether or not the entity can cast a shadow. Currently applicable only to 
 *     {@link Entities.EntityType|Model} and {@link Entities.EntityType|Shape} entities. Shadows are cast if inside a 
 *     {@link Entities.EntityType|Zone} entity with <code>castShadows</code> enabled in its 
 *     {@link Entities.EntityProperties-Zone|keyLight} property.
 * @property {boolean} isVisibleInSecondaryCamera=true - Whether or not the entity is rendered in the secondary camera. If <code>true</code> then the entity is rendered.
 *
 * @property {Vec3} position=0,0,0 - The position of the entity.
 * @property {Quat} rotation=0,0,0,1 - The orientation of the entity with respect to world coordinates.
 * @property {Vec3} registrationPoint=0.5,0.5,0.5 - The point in the entity that is set to the entity's position and is rotated 
 *      about, {@link Vec3(0)|Vec3.ZERO} &ndash; {@link Vec3(0)|Vec3.ONE}. A value of {@link Vec3(0)|Vec3.ZERO} is the entity's
 *      minimum x, y, z corner; a value of {@link Vec3(0)|Vec3.ONE} is the entity's maximum x, y, z corner.
 *
 * @property {Vec3} naturalPosition=0,0,0 - The center of the entity's unscaled mesh model if it has one, otherwise
 *     {@link Vec3(0)|Vec3.ZERO}. <em>Read-only.</em>
 * @property {Vec3} naturalDimensions - The dimensions of the entity's unscaled mesh model if it has one, otherwise 
 *     {@link Vec3(0)|Vec3.ONE}. <em>Read-only.</em>
 *
 * @property {Vec3} velocity=0,0,0 - The linear velocity of the entity in m/s with respect to world coordinates.
 * @property {number} damping=0.39347 - How much to slow down the linear velocity of an entity over time, <code>0.0</code> 
 *     &ndash; <code>1.0</code>. A higher damping value slows down the entity more quickly. The default value is for an 
 *     exponential decay timescale of 2.0s, where it takes 2.0s for the movement to slow to <code>1/e = 0.368</code> of its 
 *     initial value.
 * @property {Vec3} angularVelocity=0,0,0 - The angular velocity of the entity in rad/s with respect to its axes, about its
 *     registration point.
 * @property {number} angularDamping=0.39347 - How much to slow down the angular velocity of an entity over time, 
 *     <code>0.0</code> &ndash; <code>1.0</code>. A higher damping value slows down the entity more quickly. The default value 
 *     is for an exponential decay timescale of 2.0s, where it takes 2.0s for the movement to slow to <code>1/e = 0.368</code> 
 *     of its initial value.
 *
 * @property {Vec3} gravity=0,0,0 - The acceleration due to gravity in m/s<sup>2</sup> that the entity should move with, in 
 *     world coordinates. Set to <code>{ x: 0, y: -9.8, z: 0 }</code> to simulate Earth's gravity. Gravity is applied to an 
 *     entity's motion only if its <code>dynamic</code> property is <code>true</code>. If changing an entity's 
 *     <code>gravity</code> from {@link Vec3(0)|Vec3.ZERO}, you need to give it a small <code>velocity</code> in order to kick 
 *     off physics simulation.
 *     The <code>gravity</code> value is applied in addition to the <code>acceleration</code> value.
 * @property {Vec3} acceleration=0,0,0 - A general acceleration in m/s<sup>2</sup> that the entity should move with, in world 
 *     coordinates. The acceleration is applied to an entity's motion only if its <code>dynamic</code> property is 
 *     <code>true</code>. If changing an entity's <code>acceleration</code> from {@link Vec3(0)|Vec3.ZERO}, you need to give it 
 *     a small <code>velocity</code> in order to kick off physics simulation.
 *     The <code>acceleration</code> value is applied in addition to the <code>gravity</code> value.
 * @property {number} restitution=0.5 - The "bounciness" of an entity when it collides, <code>0.0</code> &ndash; 
 *     <code>0.99</code>. The higher the value, the more bouncy.
 * @property {number} friction=0.5 - How much to slow down an entity when it's moving against another, <code>0.0</code> &ndash; 
 *     <code>10.0</code>. The higher the value, the more quickly it slows down. Examples: <code>0.1</code> for ice, 
 *     <code>0.9</code> for sandpaper.
 * @property {number} density=1000 - The density of the entity in kg/m<sup>3</sup>, <code>100</code> for balsa wood &ndash; 
 *     <code>10000</code> for silver. The density is used in conjunction with the entity's bounding box volume to work out its 
 *     mass in the application of physics.
 *
 * @property {boolean} collisionless=false - Whether or not the entity should collide with items per its 
 *     <code>collisionMask<code> property. If <code>true</code> then the entity does not collide. A synonym is <code>ignoreForCollisions</code>.
 * @property {Entities.CollisionMask} collisionMask=31 - What types of items the entity should collide with.
 * @property {string} collidesWith="static,dynamic,kinematic,myAvatar,otherAvatar," - Synonym for <code>collisionMask</code>,
 *     in text format.
 * @property {string} collisionSoundURL="" - The sound to play when the entity experiences a collision. Valid file formats are
 *     as per the {@link SoundCache} object.
 * @property {boolean} dynamic=false - Whether or not the entity should be affected by collisions. If <code>true</code> then 
 *     the entity's movement is affected by collisions. A synonym is <code>collisionsWillMove</code>.
 *
 * @property {string} href="" - A "hifi://" metaverse address that a user is taken to when they click on the entity.
 * @property {string} description="" - A description of the <code>href</code> property value.
 *
 * @property {string} userData="" - Used to store extra data about the entity in JSON format. WARNING: Other apps such as the 
 *     Create app can also use this property, so make sure you handle data stored by other apps &mdash; edit only your bit and 
 *     leave the rest of the data intact. You can use <code>JSON.parse()</code> to parse the string into a JavaScript object 
 *     which you can manipulate the properties of, and use <code>JSON.stringify()</code> to convert the object into a string to 
 *     put in the property.
 *
 * @property {string} script="" - The URL of the client entity script, if any, that is attached to the entity.
 * @property {number} scriptTimestamp=0 - Intended to be used to indicate when the client entity script was loaded. Should be 
 *     an integer number of milliseconds since midnight GMT on January 1, 1970 (e.g., as supplied by <code>Date.now()</code>. 
 *     If you update the property's value, the <code>script</code> is re-downloaded and reloaded. This is how the "reload" 
 *     button beside the "script URL" field in properties tab of the Create app works.
 * @property {string} serverScripts="" - The URL of the server entity script, if any, that is attached to the entity.
 *
 * @property {Uuid} parentID=Uuid.NULL - The ID of the entity or avatar that this entity is parented to. {@link Uuid|Uuid.NULL} 
 *     if the entity is not parented.
 * @property {number} parentJointIndex=65535 - The joint of the entity or avatar that this entity is parented to. Use 
 *     <code>65535</code> or <code>-1</code> to parent to the entity or avatar's position and orientation rather than a joint.
 * @property {Vec3} localPosition=0,0,0 - The position of the entity relative to its parent if the entity is parented, 
 *     otherwise the same value as <code>position</code>. If the entity is parented to an avatar and is an <code>avatarEntity</code> 
 *     so that it scales with the avatar, this value remains the original local position value while the avatar scale changes.
 * @property {Quat} localRotation=0,0,0,1 - The rotation of the entity relative to its parent if the entity is parented, 
 *     otherwise the same value as <code>rotation</code>.
 * @property {Vec3} localVelocity=0,0,0 - The velocity of the entity relative to its parent if the entity is parented, 
 *     otherwise the same value as <code>velocity</code>.
 * @property {Vec3} localAngularVelocity=0,0,0 - The angular velocity of the entity relative to its parent if the entity is 
 *     parented, otherwise the same value as <code>position</code>.
 * @property {Vec3} localDimensions - The dimensions of the entity. If the entity is parented to an avatar and is an
 *     <code>avatarEntity</code> so that it scales with the avatar, this value remains the original dimensions value while the 
 *     avatar scale changes.
 *
 * @property {Entities.BoundingBox} boundingBox - The axis-aligned bounding box that tightly encloses the entity. 
 *     <em>Read-only.</em>
 * @property {AACube} queryAACube - The axis-aligned cube that determines where the entity lives in the entity server's octree. 
 *     The cube may be considerably larger than the entity in some situations, e.g., when the entity is grabbed by an avatar: 
 *     the position of the entity is determined through avatar mixer updates and so the AA cube is expanded in order to reduce 
 *     unnecessary entity server updates. Scripts should not change this property's value.
 *
 * @property {string} actionData="" - Base-64 encoded compressed dump of the actions associated with the entity. This property
 *     is typically not used in scripts directly; rather, functions that manipulate an entity's actions update it.
 *     The size of this property increases with the number of actions. Because this property value has to fit within a High 
 *     Fidelity datagram packet there is a limit to the number of actions that an entity can have, and edits which would result 
 *     in overflow are rejected. <em>Read-only.</em>
 * @property {Entities.RenderInfo} renderInfo - Information on the cost of rendering the entity. Currently information is only 
 *     provided for <code>Model</code> entities. <em>Read-only.</em>
 *
 * @property {boolean} cloneable=false - If <code>true</code> then the entity can be cloned via {@link Entities.cloneEntity}.
 * @property {number} cloneLifetime=300 - The entity lifetime for clones created from this entity.
 * @property {number} cloneLimit=0 - The total number of clones of this entity that can exist in the domain at any given time.
 * @property {boolean} cloneDynamic=false - If <code>true</code> then clones created from this entity will have their 
 *     <code>dynamic</code> property set to <code>true</code>.
 * @property {boolean} cloneAvatarEntity=false - If <code>true</code> then clones created from this entity will be created as 
 *     avatar entities: their <code>avatarEntity</code> property will be set to <code>true</code>.
 * @property {Uuid} cloneOriginID - The ID of the entity that this entity was cloned from.
 *
 * @property {Entities.Grab} grab - The grab-related properties.
 *
 * @property {string} itemName="" - Certifiable name of the Marketplace item.
 * @property {string} itemDescription="" - Certifiable description of the Marketplace item.
 * @property {string} itemCategories="" - Certifiable category of the Marketplace item.
 * @property {string} itemArtist="" - Certifiable  artist that created the Marketplace item.
 * @property {string} itemLicense="" - Certifiable license URL for the Marketplace item.
 * @property {number} limitedRun=4294967295 - Certifiable maximum integer number of editions (copies) of the Marketplace item 
 *     allowed to be sold.
 * @property {number} editionNumber=0 - Certifiable integer edition (copy) number or the Marketplace item. Each copy sold in 
 *     the Marketplace is numbered sequentially, starting at 1.
 * @property {number} entityInstanceNumber=0 - Certifiable integer instance number for identical entities in a Marketplace 
 *     item. A Marketplace item may have identical parts. If so, then each is numbered sequentially with an instance number.
 * @property {string} marketplaceID="" - Certifiable UUID for the Marketplace item, as used in the URL of the item's download
 *     and its Marketplace Web page.
 * @property {string} certificateID="" - Hash of the entity's static certificate JSON, signed by the artist's private key.
 * @property {number} staticCertificateVersion=0 - The version of the method used to generate the <code>certificateID</code>.
 *
 * @see The different entity types have additional properties as follows:
 * @see {@link Entities.EntityProperties-Box|EntityProperties-Box}
 * @see {@link Entities.EntityProperties-Sphere|EntityProperties-Sphere}
 * @see {@link Entities.EntityProperties-Shape|EntityProperties-Shape}
 * @see {@link Entities.EntityProperties-Model|EntityProperties-Model}
 * @see {@link Entities.EntityProperties-Text|EntityProperties-Text}
 * @see {@link Entities.EntityProperties-Image|EntityProperties-Image}
 * @see {@link Entities.EntityProperties-Web|EntityProperties-Web}
 * @see {@link Entities.EntityProperties-ParticleEffect|EntityProperties-ParticleEffect}
 * @see {@link Entities.EntityProperties-Line|EntityProperties-Line}
 * @see {@link Entities.EntityProperties-PolyLine|EntityProperties-PolyLine}
 * @see {@link Entities.EntityProperties-PolyVox|EntityProperties-PolyVox}
 * @see {@link Entities.EntityProperties-Grid|EntityProperties-Grid}
 * @see {@link Entities.EntityProperties-Light|EntityProperties-Light}
 * @see {@link Entities.EntityProperties-Zone|EntityProperties-Zone}
 * @see {@link Entities.EntityProperties-Material|EntityProperties-Material}
 */

/**jsdoc
 * The <code>"Box"</code> {@link Entities.EntityType|EntityType} is the same as the <code>"Shape"</code>
 * {@link Entities.EntityType|EntityType} except that its <code>shape</code> value is always set to <code>"Cube"</code>
 * when the entity is created. If its <code>shape</code> property value is subsequently changed then the entity's 
 * <code>type</code> will be reported as <code>"Sphere"</code> if the <code>shape</code> is set to <code>"Sphere"</code>, 
 * otherwise it will be reported as <code>"Shape"</code>.
 * @typedef {object} Entities.EntityProperties-Box
 */

/**jsdoc
 * The <code>"Light"</code> {@link Entities.EntityType|EntityType} adds local lighting effects.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Light
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity. Entity surface outside these dimensions are not lit 
 *     by the light.
 * @property {Color} color=255,255,255 - The color of the light emitted.
 * @property {number} intensity=1 - The brightness of the light.
 * @property {number} falloffRadius=0.1 - The distance from the light's center at which intensity is reduced by 25%.
 * @property {boolean} isSpotlight=false - If <code>true</code> then the light is directional, emitting along the entity's
 *     local negative z-axis; otherwise the light is a point light which emanates in all directions.
 * @property {number} exponent=0 - Affects the softness of the spotlight beam: the higher the value the softer the beam.
 * @property {number} cutoff=1.57 - Affects the size of the spotlight beam: the higher the value the larger the beam.
 * @example <caption>Create a spotlight pointing at the ground.</caption>
 * Entities.addEntity({
 *     type: "Light",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -4 })),
 *     rotation: Quat.fromPitchYawRollDegrees(-75, 0, 0),
 *     dimensions: { x: 5, y: 5, z: 5 },
 *     intensity: 100,
 *     falloffRadius: 0.3,
 *     isSpotlight: true,
 *     exponent: 20,
 *     cutoff: 30,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"Line"</code> {@link Entities.EntityType|EntityType} draws thin, straight lines between a sequence of two or more
 * points.  Deprecated: Use PolyLines instead.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Line
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity. Must be sufficient to contain all the
 *     <code>linePoints</code>.
 * @property {Vec3[]} linePoints=[]] - The sequence of points to draw lines between. The values are relative to the entity's
 *     position. A maximum of 70 points can be specified. The property's value is set only if all the <code>linePoints</code>
 *     lie within the entity's <code>dimensions</code>.
 * @property {Color} color=255,255,255 - The color of the line.
 * @example <caption>Draw lines in a "V".</caption>
 * var entity = Entities.addEntity({
 *     type: "Line",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
 *     rotation: MyAvatar.orientation,
 *     dimensions: { x: 2, y: 2, z: 1 },
 *     linePoints: [
 *         { x: -1, y: 1, z: 0 },
 *         { x: 0, y: -1, z: 0 },
 *         { x: 1, y: 1, z: 0 },
 *     ],
 *     color: { red: 255, green: 0, blue: 0 },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"Material"</code> {@link Entities.EntityType|EntityType} modifies the existing materials on
 * {@link Entities.EntityType|Model} entities, {@link Entities.EntityType|Shape} entities (albedo only), 
 * {@link Overlays.OverlayType|model overlays}, and avatars.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.<br />
 * To apply a material to an entity or overlay, set the material entity's <code>parentID</code> property to the entity or 
 * overlay's ID.
 * To apply a material to an avatar, set the material entity's <code>parentID</code> property to the avatar's session UUID.
 * To apply a material to your avatar such that it persists across domains and log-ins, create the material as an avatar entity 
 * by setting the <code>entityHostType</code> parameter in {@link Entities.addEntity} to <code>"avatar"</code>.
 * Material entities render as non-scalable spheres if they don't have their parent set.
 * @typedef {object} Entities.EntityProperties-Material
 * @property {string} materialURL="" - URL to a {@link MaterialResource}. If you append <code>?name</code> to the URL, the 
 *     material with that name in the {@link MaterialResource} will be applied to the entity. <br />
 *     Alternatively, set the property value to <code>"materialData"</code> to use the <code>materialData</code> property  
 *     for the {@link MaterialResource} values.
 * @property {number} priority=0 - The priority for applying the material to its parent. Only the highest priority material is 
 *     applied, with materials of the same priority randomly assigned. Materials that come with the model have a priority of 
 *     <code>0</code>.
 * @property {string|number} parentMaterialName="0" - Selects the submesh or submeshes within the parent to apply the material 
 *     to. If in the format <code>"mat::string"</code>, all submeshes with material name <code>"string"</code> are replaced. 
 *     Otherwise the property value is parsed as an unsigned integer, specifying the mesh index to modify. Invalid values are 
 *     parsed to <code>0</code>.
 * @property {string} materialMappingMode="uv" - How the material is mapped to the entity. Either <code>"uv"</code> or 
 *     <code>"projected"</code>. In "uv" mode, the material will be evaluated within the UV space of the mesh it is applied to.  In
 *     "projected" mode, the 3D transform of the Material Entity will be used to evaluate the texture coordinates for the material.
 * @property {Vec2} materialMappingPos=0,0 - Offset position in UV-space of the top left of the material, range 
 *     <code>{ x: 0, y: 0 }</code> &ndash; <code>{ x: 1, y: 1 }</code>.
 * @property {Vec2} materialMappingScale=1,1 - How much to scale the material within the parent's UV-space.
 * @property {number} materialMappingRot=0 - How much to rotate the material within the parent's UV-space, in degrees.
 * @property {string} materialData="" - Used to store {@link MaterialResource} data as a JSON string. You can use 
 *     <code>JSON.parse()</code> to parse the string into a JavaScript object which you can manipulate the properties of, and 
 *     use <code>JSON.stringify()</code> to convert the object into a string to put in the property.
 * @property {boolean} materialRepeat=true - If true, the material will repeat.  If false, fragments outside of texCoord 0 - 1 will be discarded.
 *     Works in both "uv" and "projected" modes.
 * @example <caption>Color a sphere using a Material entity.</caption>
 * var entityID = Entities.addEntity({
 *     type: "Sphere",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
 *     dimensions: { x: 1, y: 1, z: 1 },
 *     color: { red: 128, green: 128, blue: 128 },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 *
 * var materialID = Entities.addEntity({
 *     type: "Material",
 *     parentID: entityID,
 *     materialURL: "materialData",
 *     priority: 1,
 *     materialData: JSON.stringify({
 *         materialVersion: 1,
 *         materials: {
 *             // Value overrides entity's "color" property.
 *             albedo: [1.0, 1.0, 0]  // Yellow
 *         }
 *     }),
 * });
 */

/**jsdoc
 * The <code>"Model"</code> {@link Entities.EntityType|EntityType} displays an FBX or OBJ model.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Model
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity. When adding an entity, if no <code>dimensions</code> 
 *     value is specified then the model is automatically sized to its 
 *     <code>{@link Entities.EntityProperties|naturalDimensions}</code>.
 * @property {Color} color=255,255,255 - <em>Currently not used.</em>
 * @property {string} modelURL="" - The URL of the FBX of OBJ model. Baked FBX models' URLs end in ".baked.fbx".<br />
 * @property {string} textures="" - A JSON string of texture name, URL pairs used when rendering the model in place of the
 *     model's original textures. Use a texture name from the <code>originalTextures</code> property to override that texture. 
 *     Only the texture names and URLs to be overridden need be specified; original textures are used where there are no 
 *     overrides. You can use <code>JSON.stringify()</code> to convert a JavaScript object of name, URL pairs into a JSON 
 *     string.
 * @property {string} originalTextures="{}" - A JSON string of texture name, URL pairs used in the model. The property value is 
 *     filled in after the entity has finished rezzing (i.e., textures have loaded). You can use <code>JSON.parse()</code> to 
 *     parse the JSON string into a JavaScript object of name, URL pairs. <em>Read-only.</em>
 *
 * @property {ShapeType} shapeType="none" - The shape of the collision hull used if collisions are enabled.
 * @property {string} compoundShapeURL="" - The OBJ file to use for the compound shape if <code>shapeType</code> is
 *     <code>"compound"</code>.
 *
 * @property {Entities.AnimationProperties} animation - An animation to play on the model.
 *
 * @property {Quat[]} jointRotations=[] - Joint rotations applied to the model; <code>[]</code> if none are applied or the 
 *     model hasn't loaded. The array indexes are per {@link Entities.getJointIndex|getJointIndex}. Rotations are relative to 
 *     each joint's parent.<br />
 *     Joint rotations can be set by {@link Entities.setLocalJointRotation|setLocalJointRotation} and similar functions, or by
 *     setting the value of this property. If you set a joint rotation using this property you also need to set the 
 *     corresponding <code>jointRotationsSet</code> value to <code>true</code>.
 * @property {boolean[]} jointRotationsSet=[] - <code>true</code> values for joints that have had rotations applied, 
 *     <code>false</code> otherwise; <code>[]</code> if none are applied or the model hasn't loaded. The array indexes are per 
 *     {@link Entities.getJointIndex|getJointIndex}.
 * @property {Vec3[]} jointTranslations=[] - Joint translations applied to the model; <code>[]</code> if none are applied or 
 *     the model hasn't loaded. The array indexes are per {@link Entities.getJointIndex|getJointIndex}. Rotations are relative 
 *     to each joint's parent.<br />
 *     Joint translations can be set by {@link Entities.setLocalJointTranslation|setLocalJointTranslation} and similar 
 *     functions, or by setting the value of this property. If you set a joint translation using this property you also need to 
 *     set the corresponding <code>jointTranslationsSet</code> value to <code>true</code>.
 * @property {boolean[]} jointTranslationsSet=[] - <code>true</code> values for joints that have had translations applied, 
 *     <code>false</code> otherwise; <code>[]</code> if none are applied or the model hasn't loaded. The array indexes are per 
 *     {@link Entities.getJointIndex|getJointIndex}.
 * @property {boolean} relayParentJoints=false - If <code>true</code> and the entity is parented to an avatar, then the 
 *     avatar's joint rotations are applied to the entity's joints.
 *
 * @example <caption>Rez a Vive tracker puck.</caption>
 * var entity = Entities.addEntity({
 *     type: "Model",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -2 })),
 *     rotation: MyAvatar.orientation,
 *     modelURL: "http://content.highfidelity.com/seefo/production/puck-attach/vive_tracker_puck.obj",
 *     dimensions: { x: 0.0945, y: 0.0921, z: 0.0423 },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"ParticleEffect"</code> {@link Entities.EntityType|EntityType} displays a particle system that can be used to 
 * simulate things such as fire, smoke, snow, magic spells, etc. The particles emanate from an ellipsoid or part thereof.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-ParticleEffect

 * @property {boolean} isEmitting=true - If <code>true</code> then particles are emitted.
 * @property {number} maxParticles=1000 - The maximum number of particles to render at one time. Older particles are deleted if 
 *     necessary when new ones are created.
 * @property {number} lifespan=3s - How long, in seconds, each particle lives.
 * @property {number} emitRate=15 - The number of particles per second to emit.
 * @property {number} emitSpeed=5 - The speed, in m/s, that each particle is emitted at.
 * @property {number} speedSpread=1 - The spread in speeds at which particles are emitted at. If <code>emitSpeed == 5</code> 
 *     and <code>speedSpread == 1</code>, particles will be emitted with speeds in the range 4m/s &ndash; 6m/s.
 * @property {Vec3} emitAcceleration=0,-9.8,0 - The acceleration that is applied to each particle during its lifetime. The 
 *     default is Earth's gravity value.
 * @property {Vec3} accelerationSpread=0,0,0 - The spread in accelerations that each particle is given. If
 *     <code>emitAccelerations == {x: 0, y: -9.8, z: 0}</code> and <code>accelerationSpread ==
 *     {x: 0, y: 1, z: 0}</code>, each particle will have an acceleration in the range <code>{x: 0, y: -10.8, z: 0}</code>
 *     &ndash; <code>{x: 0, y: -8.8, z: 0}</code>.
 * @property {Vec3} dimensions - The dimensions of the particle effect, i.e., a bounding box containing all the particles
 *     during their lifetimes, assuming that <code>emitterShouldTrail</code> is <code>false</code>. <em>Read-only.</em>
 * @property {boolean} emitterShouldTrail=false - If <code>true</code> then particles are "left behind" as the emitter moves,
 *     otherwise they stay with the entity's dimensions.
 *
 * @property {Quat} emitOrientation=-0.707,0,0,0.707 - The orientation of particle emission relative to the entity's axes. By
 *     default, particles emit along the entity's local z-axis, and <code>azimuthStart</code> and <code>azimuthFinish</code> 
 *     are relative to the entity's local x-axis. The default value is a rotation of -90 degrees about the local x-axis, i.e., 
 *     the particles emit vertically.
 * @property {Vec3} emitDimensions=0,0,0 - The dimensions of the ellipsoid from which particles are emitted.
 * @property {number} emitRadiusStart=1 - The starting radius within the ellipsoid at which particles start being emitted;
 *     range <code>0.0</code> &ndash; <code>1.0</code> for the ellipsoid center to the ellipsoid surface, respectively.
 *     Particles are emitted from the portion of the ellipsoid that lies between <code>emitRadiusStart</code> and the 
 *     ellipsoid's surface.
 * @property {number} polarStart=0 - The angle in radians from the entity's local z-axis at which particles start being emitted 
 *     within the ellipsoid; range <code>0</code> &ndash; <code>Math.PI</code>. Particles are emitted from the portion of the 
 *     ellipsoid that lies between <code>polarStart<code> and <code>polarFinish</code>.
 * @property {number} polarFinish=0 - The angle in radians from the entity's local z-axis at which particles stop being emitted 
 *     within the ellipsoid; range <code>0</code> &ndash; <code>Math.PI</code>. Particles are emitted from the portion of the 
 *     ellipsoid that lies between <code>polarStart<code> and <code>polarFinish</code>.
 * @property {number} azimuthStart=-Math.PI - The angle in radians from the entity's local x-axis about the entity's local 
 *     z-axis at which particles start being emitted; range <code>-Math.PI</code> &ndash; <code>Math.PI</code>. Particles are 
 *     emitted from the portion of the ellipsoid that lies between <code>azimuthStart<code> and <code>azimuthFinish</code>.
 * @property {number} azimuthFinish=Math.PI - The angle in radians from the entity's local x-axis about the entity's local
 *     z-axis at which particles stop being emitted; range <code>-Math.PI</code> &ndash; <code>Math.PI</code>. Particles are
 *     emitted from the portion of the ellipsoid that lies between <code>azimuthStart<code> and <code>azimuthFinish</code>.
 *
 * @property {string} textures="" - The URL of a JPG or PNG image file to display for each particle. If you want transparency,
 *     use PNG format.
 * @property {number} particleRadius=0.025 - The radius of each particle at the middle of its life.
 * @property {number} radiusStart=NaN - The radius of each particle at the start of its life. If <code>NaN</code>, the
 *     <code>particleRadius</code> value is used.
 * @property {number} radiusFinish=NaN - The radius of each particle at the end of its life. If <code>NaN</code>, the
 *     <code>particleRadius</code> value is used.
 * @property {number} radiusSpread=0 - The spread in radius that each particle is given. If <code>particleRadius == 0.5</code>
 *     and <code>radiusSpread == 0.25</code>, each particle will have a radius in the range <code>0.25</code> &ndash; 
 *     <code>0.75</code>.
 * @property {Color} color=255,255,255 - The color of each particle at the middle of its life.
 * @property {ColorFloat} colorStart={} - The color of each particle at the start of its life. If any of the component values are
 *     undefined, the <code>color</code> value is used.
 * @property {ColorFloat} colorFinish={} - The color of each particle at the end of its life. If any of the component values are
 *     undefined, the <code>color</code> value is used.
 * @property {Color} colorSpread=0,0,0 - The spread in color that each particle is given. If
 *     <code>color == {red: 100, green: 100, blue: 100}</code> and <code>colorSpread ==
 *     {red: 10, green: 25, blue: 50}</code>, each particle will have a color in the range 
 *     <code>{red: 90, green: 75, blue: 50}</code> &ndash; <code>{red: 110, green: 125, blue: 150}</code>.
 * @property {number} alpha=1 - The alpha of each particle at the middle of its life.
 * @property {number} alphaStart=NaN - The alpha of each particle at the start of its life. If <code>NaN</code>, the
 *     <code>alpha</code> value is used.
 * @property {number} alphaFinish=NaN - The alpha of each particle at the end of its life. If <code>NaN</code>, the
 *     <code>alpha</code> value is used.
 * @property {number} alphaSpread=0 - The spread in alpha that each particle is given. If <code>alpha == 0.5</code>
 *     and <code>alphaSpread == 0.25</code>, each particle will have an alpha in the range <code>0.25</code> &ndash; 
 *     <code>0.75</code>.
 * @property {number} particleSpin=0 - The spin of each particle at the middle of its life. In the range <code>-2*PI</code> &ndash; <code>2*PI</code>.
 * @property {number} spinStart=NaN - The spin of each particle at the start of its life. In the range <code>-2*PI</code> &ndash; <code>2*PI</code>.
 *     If <code>NaN</code>, the <code>particleSpin</code> value is used.
 * @property {number} spinFinish=NaN - The spin of each particle at the end of its life. In the range <code>-2*PI</code> &ndash; <code>2*PI</code>.
 *     If <code>NaN</code>, the <code>particleSpin</code> value is used.
 * @property {number} spinSpread=0 - The spread in spin that each particle is given. In the range <code>0</code> &ndash; <code>2*PI</code>.  If <code>particleSpin == PI</code>
 *     and <code>spinSpread == PI/2</code>, each particle will have a spin in the range <code>PI/2</code> &ndash; <code>3*PI/2</code>.
 * @property {boolean} rotateWithEntity=false - Whether or not the particles' spin will rotate with the entity.  If false, when <code>particleSpin == 0</code>, the particles will point
 * up in the world.  If true, they will point towards the entity's up vector, based on its orientation.
 *
 * @property {ShapeType} shapeType="none" - <em>Currently not used.</em> <em>Read-only.</em>
 *
 * @example <caption>Create a ball of green smoke.</caption>
 * particles = Entities.addEntity({
 *     type: "ParticleEffect",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -4 })),
 *     lifespan: 5,
 *     emitRate: 10,
 *     emitSpeed: 0.02,
 *     speedSpread: 0.01,
 *     emitAcceleration: { x: 0, y: 0.02, z: 0 },
 *     polarFinish: Math.PI,
 *     textures: "https://content.highfidelity.com/DomainContent/production/Particles/wispy-smoke.png",
 *     particleRadius: 0.1,
 *     color: { red: 0, green: 255, blue: 0 },
 *     alphaFinish: 0,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"PolyLine"</code> {@link Entities.EntityType|EntityType} draws textured, straight lines between a sequence of 
 * points.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-PolyLine
 * @property {Vec3} dimensions=1,1,1 - The dimensions of the entity, i.e., the size of the bounding box that contains the 
 *     lines drawn.
 * @property {Vec3[]} linePoints=[] - The sequence of points to draw lines between. The values are relative to the entity's
 *     position. A maximum of 70 points can be specified. Must be specified in order for the entity to render.
 * @property {Vec3[]} normals=[] - The normal vectors for the line's surface at the <code>linePoints</code>. The values are 
 *     relative to the entity's orientation. Must be specified in order for the entity to render.
 * @property {number[]} strokeWidths=[] - The widths, in m, of the line at the <code>linePoints</code>. Must be specified in 
 *     order for the entity to render.
 * @property {Vec3[]} strokeColors=[] - The base colors of each point, which are multiplied with the color of the texture, going from 0-1.
 *     If strokeColors.length < the number of points, <code>color</code> is used for the remaining points.
 * @property {Color} color=255,255,255 - Used as the color for each point if <code>strokeColors</code> is too short.
 * @property {string} textures="" - The URL of a JPG or PNG texture to use for the lines. If you want transparency, use PNG
 *     format.
 * @property {boolean} isUVModeStretch=true - If <code>true</code>, the texture is stretched to fill the whole line, otherwise 
 *     the texture repeats along the line.
 * @property {bool} glow=false - If <code>true</code>, the alpha of the strokes will drop off farther from the center.
 * @property {bool} faceCamera=false - If <code>true</code>, each line segment will rotate to face the camera.
 * @example <caption>Draw a textured "V".</caption>
 * var entity = Entities.addEntity({
 *     type: "PolyLine",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -5 })),
 *     rotation: MyAvatar.orientation,
 *     linePoints: [
 *         { x: -1, y: 0.5, z: 0 },
 *         { x: 0, y: 0, z: 0 },
 *         { x: 1, y: 0.5, z: 0 }
 *     ],
 *     normals: [
 *         { x: 0, y: 0, z: 1 },
 *         { x: 0, y: 0, z: 1 },
 *         { x: 0, y: 0, z: 1 }
 *     ],
 *     strokeWidths: [ 0.1, 0.1, 0.1 ],
 *     color: { red: 255, green: 0, blue: 0 },  // Use just the red channel from the image.
 *     textures: "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/flowArts/trails.png",
 *     isUVModeStretch: true,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"PolyVox"</code> {@link Entities.EntityType|EntityType} displays a set of textured voxels. 
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * If you have two or more neighboring PolyVox entities of the same size abutting each other, you can display them as joined by
 * configuring their <code>voxelSurfaceStyle</code> and neighbor ID properties.<br />
 * PolyVox entities uses a library from <a href="http://www.volumesoffun.com/">Volumes of Fun</a>. Their
 * <a href="http://www.volumesoffun.com/polyvox-documentation/">library documentation</a> may be useful to read.
 * @typedef {object} Entities.EntityProperties-PolyVox
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity.
 * @property {Vec3} voxelVolumeSize=32,32,32 - Integer number of voxels along each axis of the entity, in the range 
 *     <code>1,1,1</code> to <code>128,128,128</code>. The dimensions of each voxel is 
 *     <code>dimensions / voxelVolumesize</code>.
 * @property {string} voxelData="ABAAEAAQAAAAHgAAEAB42u3BAQ0AAADCoPdPbQ8HFAAAAPBuEAAAAQ==" - Base-64 encoded compressed dump of 
 *     the PolyVox data. This property is typically not used in scripts directly; rather, functions that manipulate a PolyVox 
 *     entity update it.<br />
 *     The size of this property increases with the size and complexity of the PolyVox entity, with the size depending on how 
 *     the particular entity's voxels compress. Because this property value has to fit within a High Fidelity datagram packet 
 *     there is a limit to the size and complexity of a PolyVox entity, and edits which would result in an overflow are 
 *     rejected.
 * @property {Entities.PolyVoxSurfaceStyle} voxelSurfaceStyle=2 - The style of rendering the voxels' surface and how 
 *     neighboring PolyVox entities are joined.
 * @property {string} xTextureURL="" - URL of the texture to map to surfaces perpendicular to the entity's local x-axis. JPG or
 *     PNG format. If no texture is specified the surfaces display white.
 * @property {string} yTextureURL="" - URL of the texture to map to surfaces perpendicular to the entity's local y-axis. JPG or 
 *     PNG format. If no texture is specified the surfaces display white.
 * @property {string} zTextureURL="" - URL of the texture to map to surfaces perpendicular to the entity's local z-axis. JPG or 
 *     PNG format. If no texture is specified the surfaces display white.
 * @property {Uuid} xNNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's -ve local x-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} yNNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's -ve local y-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} zNNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's -ve local z-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} xPNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's +ve local x-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} yPNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's +ve local y-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @property {Uuid} zPNeighborID=Uuid.NULL - ID of the neighboring PolyVox entity in the entity's +ve local z-axis direction, 
 *     if you want them joined. Set to {@link Uuid|Uuid.NULL} if there is none or you don't want to join them.
 * @example <caption>Create a textured PolyVox sphere.</caption>
 * var position = Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.5, z: -8 }));
 * var texture = "http://public.highfidelity.com/cozza13/tuscany/Concrete2.jpg";
 * var polyVox = Entities.addEntity({
 *     type: "PolyVox",
 *     position: position,
 *     dimensions: { x: 2, y: 2, z: 2 },
 *     xTextureURL: texture,
 *     yTextureURL: texture,
 *     zTextureURL: texture,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 * Entities.setVoxelSphere(polyVox, position, 0.8, 255);
 */

/**jsdoc
 * The <code>"Shape"</code> {@link Entities.EntityType|EntityType} displays an entity of a specified <code>shape</code>.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Shape
 * @property {Entities.Shape} shape="Sphere" - The shape of the entity.
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The dimensions of the entity.
 * @property {Color} color=255,255,255 - The color of the entity.
 * @example <caption>Create a cylinder.</caption>
 * var shape = Entities.addEntity({
 *     type: "Shape",
 *     shape: "Cylinder",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
 *     dimensions: { x: 0.4, y: 0.6, z: 0.4 },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"Sphere"</code> {@link Entities.EntityType|EntityType} is the same as the <code>"Shape"</code>
 * {@link Entities.EntityType|EntityType} except that its <code>shape</code> value is always set to <code>"Sphere"</code>
 * when the entity is created. If its <code>shape</code> property value is subsequently changed then the entity's 
 * <code>type</code> will be reported as <code>"Box"</code> if the <code>shape</code> is set to <code>"Cube"</code>, 
 * otherwise it will be reported as <code>"Shape"</code>.
 * @typedef {object} Entities.EntityProperties-Sphere
 */

/**jsdoc
 * The <code>"Text"</code> {@link Entities.EntityType|EntityType} displays a 2D rectangle of text in the domain.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Text
 * @property {Vec3} dimensions=0.1,0.1,0.01 - The dimensions of the entity.
 * @property {string} text="" - The text to display on the face of the entity. Text wraps if necessary to fit. New lines can be
 *     created using <code>\n</code>. Overflowing lines are not displayed.
 * @property {number} lineHeight=0.1 - The height of each line of text (thus determining the font size).
 * @property {Color} textColor=255,255,255 - The color of the text.
 * @property {number} textAlpha=1.0 - The text alpha.
 * @property {Color} backgroundColor=0,0,0 - The color of the background rectangle.
 * @property {number} backgroundAlpha=1.0 - The background alpha.
 * @property {BillboardMode} billboardMode="none" - If <code>"none"</code>, the entity is not billboarded.  If <code>"yaw"</code>, the entity will be
 *     oriented to follow your camera around the y-axis.  If <code>"full"</code> the entity will be oriented to face your camera.  The following deprecated
 *     behavior is also supported: you can also set <code>"faceCamera"</code> to <code>true</code> to set <code>billboardMode</code> to "yaw", and you can set
 *     <code>"isFacingAvatar"</code> to <code>true</code> to set <code>billboardMode</code> to "full".  Setting either to <code>false</code> sets the mode to "none"
 * @property {number} leftMargin=0.0 - The left margin, in meters.
 * @property {number} rightMargin=0.0 - The right margin, in meters.
 * @property {number} topMargin=0.0 - The top margin, in meters.
 * @property {number} bottomMargin=0.0 - The bottom margin, in meters.
 * @example <caption>Create a text entity.</caption>
 * var text = Entities.addEntity({
 *     type: "Text",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
 *     dimensions: { x: 0.6, y: 0.3, z: 0.01 },
 *     lineHeight: 0.12,
 *     text: "Hello\nthere!",
 *     billboardMode: "yaw",
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"Web"</code> {@link Entities.EntityType|EntityType} displays a browsable Web page. Each user views their own copy 
 * of the Web page: if one user navigates to another page on the entity, other users do not see the change; if a video is being 
 * played, users don't see it in sync.
 * The entity has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Web
 * @property {Vec3} dimensions=0.1,0.1,0.01 - The dimensions of the entity.
 * @property {string} sourceUrl="" - The URL of the Web page to display. This value does not change as you or others navigate 
 *     on the Web entity.
 * @property {number} dpi=30 - The resolution to display the page at, in dots per inch. If you convert this to dots per meter 
 *     (multiply by 1 / 0.0254 = 39.3701) then multiply <code>dimensions.x</code> and <code>dimensions.y</code> by that value 
 *     you get the resolution in pixels.
 * @example <caption>Create a Web entity displaying at 1920 x 1080 resolution.</caption>
 * var METERS_TO_INCHES = 39.3701;
 * var entity = Entities.addEntity({
 *     type: "Web",
 *     sourceUrl: "https://highfidelity.com/",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0.75, z: -4 })),
 *     rotation: MyAvatar.orientation,
 *     dimensions: {
 *         x: 3,
 *         y: 3 * 1080 / 1920,
 *         z: 0.01
 *     },
 *     dpi: 1920 / (3 * METERS_TO_INCHES),
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"Zone"</code> {@link Entities.EntityType|EntityType} is a volume of lighting effects and avatar permissions.
 * Avatar interaction events such as {@link Entities.enterEntity} are also often used with a Zone entity.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Zone
 * @property {Vec3} dimensions=0.1,0.1,0.1 - The size of the volume in which the zone's lighting effects and avatar permissions 
 *     have effect.
 *
 * @property {ShapeType} shapeType="box" - The shape of the volume in which the zone's lighting effects and avatar 
 *     permissions have effect. Reverts to the default value if set to <code>"none"</code>, or set to <code>"compound"</code> 
 *     and <code>compoundShapeURL</code> is <code>""</code>.
  * @property {string} compoundShapeURL="" - The OBJ file to use for the compound shape if <code>shapeType</code> is 
 *     <code>"compound"</code>.
 *
 * @property {string} keyLightMode="inherit" - Configures the key light in the zone. Possible values:<br />
 *     <code>"inherit"</code>: The key light from any enclosing zone continues into this zone.<br />
 *     <code>"disabled"</code>: The key light from any enclosing zone and the key light of this zone are disabled in this 
 *         zone.<br />
 *     <code>"enabled"</code>: The key light properties of this zone are enabled, overriding the key light of from any 
 *         enclosing zone.
 * @property {Entities.KeyLight} keyLight - The key light properties of the zone.
 *
 * @property {string} ambientLightMode="inherit" - Configures the ambient light in the zone. Possible values:<br />
 *     <code>"inherit"</code>: The ambient light from any enclosing zone continues into this zone.<br />
 *     <code>"disabled"</code>: The ambient light from any enclosing zone and the ambient light of this zone are disabled in 
 *         this zone.<br />
 *     <code>"enabled"</code>: The ambient light properties of this zone are enabled, overriding the ambient light from any 
 *         enclosing zone.
 * @property {Entities.AmbientLight} ambientLight - The ambient light properties of the zone.
 *
 * @property {string} skyboxMode="inherit" - Configures the skybox displayed in the zone. Possible values:<br />
 *     <code>"inherit"</code>: The skybox from any enclosing zone is dislayed in this zone.<br />
 *     <code>"disabled"</code>: The skybox from any enclosing zone and the skybox of this zone are disabled in this zone.<br />
 *     <code>"enabled"</code>: The skybox properties of this zone are enabled, overriding the skybox from any enclosing zone.
 * @property {Entities.Skybox} skybox - The skybox properties of the zone.
 *
 * @property {string} hazeMode="inherit" - Configures the haze in the zone. Possible values:<br />
 *     <code>"inherit"</code>: The haze from any enclosing zone continues into this zone.<br />
 *     <code>"disabled"</code>: The haze from any enclosing zone and the haze of this zone are disabled in this zone.<br />
 *     <code>"enabled"</code>: The haze properties of this zone are enabled, overriding the haze from any enclosing zone.
 * @property {Entities.Haze} haze - The haze properties of the zone.
 *
 * @property {string} bloomMode="inherit" - Configures the bloom in the zone. Possible values:<br />
 *     <code>"inherit"</code>: The bloom from any enclosing zone continues into this zone.<br />
 *     <code>"disabled"</code>: The bloom from any enclosing zone and the bloom of this zone are disabled in this zone.<br />
 *     <code>"enabled"</code>: The bloom properties of this zone are enabled, overriding the bloom from any enclosing zone.
 * @property {Entities.Bloom} bloom - The bloom properties of the zone.
 *
 * @property {boolean} flyingAllowed=true - If <code>true</code> then visitors can fly in the zone; otherwise they cannot.
 * @property {boolean} ghostingAllowed=true - If <code>true</code> then visitors with avatar collisions turned off will not 
 *     collide with content in the zone; otherwise visitors will always collide with content in the zone.
 
 * @property {string} filterURL="" - The URL of a JavaScript file that filters changes to properties of entities within the 
 *     zone. It is periodically executed for each entity in the zone. It can, for example, be used to not allow changes to 
 *     certain properties.<br />
 * <pre>
 * function filter(properties) {
 *     // Test and edit properties object values,
 *     // e.g., properties.modelURL, as required.
 *     return properties;
 * }
 * </pre>
 *
 * @example <caption>Create a zone that casts a red key light along the x-axis.</caption>
 * var zone = Entities.addEntity({
 *     type: "Zone",
 *     position: MyAvatar.position,
 *     dimensions: { x: 100, y: 100, z: 100 },
 *     keyLightMode: "enabled",
 *     keyLight: {
 *         "color": { "red": 255, "green": 0, "blue": 0 },
 *         "direction": { "x": 1, "y": 0, "z": 0 }
 *     },
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"Image"</code> {@link Entities.EntityType|EntityType} displays an image on a 2D rectangle in the domain.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Image
 * @property {string} imageURL="" - The URL of the image to use.
 * @property {boolean} emissive=false - Whether or not the image should be emissive (unlit).
 * @property {boolean} keepAspectRatio=true - Whether or not the image should maintain its aspect ratio.
 * @property {BillboardMode} billboardMode="none" - If <code>"none"</code>, the entity is not billboarded.  If <code>"yaw"</code>, the entity will be
 *     oriented to follow your camera around the y-axis.  If <code>"full"</code> the entity will be oriented to face your camera.  The following deprecated
 *     behavior is also supported: you can also set <code>"faceCamera"</code> to <code>true</code> to set <code>billboardMode</code> to "yaw", and you can set
 *     <code>"isFacingAvatar"</code> to <code>true</code> to set <code>billboardMode</code> to "full".  Setting either to <code>false</code> sets the mode to "none"
 * @property {Rect} subImage={ x: 0, y: 0, width: -1, height: -1 } - The portion of the image to display. If width or height are -1, defaults to
 *     the full image in that dimension.
 * @property {Color} color=255,255,255 - The color of the image.
 * @property {number} alpha=1 - The alpha of the image.
 * @example <caption>Create a image entity.</caption>
 * var image = Entities.addEntity({
 *     type: "Image",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
 *     dimensions: { x: 0.6, y: 0.3, z: 0.01 },
 *     imageURL: "https://images.pexels.com/photos/1020315/pexels-photo-1020315.jpeg",
 *     billboardMode: "yaw",
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

/**jsdoc
 * The <code>"Grid"</code> {@link Entities.EntityType|EntityType} displays a grid on a 2D plane.
 * It has properties in addition to the common {@link Entities.EntityProperties|EntityProperties}.
 * @typedef {object} Entities.EntityProperties-Grid
 * @property {Color} color=255,255,255 - The color of the grid.
 * @property {number} alpha=1 - The alpha of the grid.
 * @property {boolean} followCamera=true - If <code>true</code>, the grid is always visible even as the camera moves to another
 *     position.
 * @property {number} majorGridEvery=5 - Integer number of <code>minorGridEvery</code> intervals at which to draw a thick grid
 *     line. Minimum value = <code>1</code>.
 * @property {number} minorGridEvery=1 - Real number of meters at which to draw thin grid lines. Minimum value =
 *     <code>0.001</code>.
 * @example <caption>Create a grid entity.</caption>
 * var grid = Entities.addEntity({
 *     type: "Grid",
 *     position: Vec3.sum(MyAvatar.position, Vec3.multiplyQbyV(MyAvatar.orientation, { x: 0, y: 0, z: -5 })),
 *     dimensions: { x: 100.0, y: 100.0, z: 0.01 },
 *     followCamera: false,
 *     majorGridEvery: 4,
 *     minorGridEvery: 0.5,
 *     lifetime: 300  // Delete after 5 minutes.
 * });
 */

QScriptValue EntityItemProperties::copyToScriptValue(QScriptEngine* engine, bool skipDefaults, bool allowUnknownCreateTime,
    bool strictSemantics, EntityPsuedoPropertyFlags psueudoPropertyFlags) const {

    // If strictSemantics is true and skipDefaults is false, then all and only those properties are copied for which the property flag
    // is included in _desiredProperties, or is one of the specially enumerated ALWAYS properties below.
    // (There may be exceptions, but if so, they are bugs.)
    // In all other cases, you are welcome to inspect the code and try to figure out what was intended. I wish you luck. -HRS 1/18/17
    QScriptValue properties = engine->newObject();
    EntityItemProperties defaultEntityProperties;

    const bool psuedoPropertyFlagsActive = psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::FlagsActive);
    // Fix to skip the default return all mechanism, when psuedoPropertyFlagsActive
    const bool psuedoPropertyFlagsButDesiredEmpty = psuedoPropertyFlagsActive && _desiredProperties.isEmpty();

    if (_created == UNKNOWN_CREATED_TIME && !allowUnknownCreateTime) {
        // No entity properties can have been set so return without setting any default, zero property values.
        return properties;
    }

    if (_idSet && (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::ID))) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_ALWAYS(id, _id.toString());
    }
    if (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::Type)) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_ALWAYS(type, EntityTypes::getEntityTypeName(_type));
    }
    if ((!skipDefaults || _lifetime != defaultEntityProperties._lifetime) && !strictSemantics) {
        if (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::Age)) {
            COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(age, getAge()); // gettable, but not settable
        }
        if (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::AgeAsText)) {
            COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(ageAsText, formatSecondsElapsed(getAge())); // gettable, but not settable
        }
    }
    if (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::LastEdited)) {
        properties.setProperty("lastEdited", convertScriptValue(engine, _lastEdited));
    }
    if (!skipDefaults) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DIMENSIONS, naturalDimensions); // gettable, but not settable
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_POSITION, naturalPosition);
    }

    // Core properties
    //COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SIMULATION_OWNER, simulationOwner); // not exposed yet
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_VISIBLE, visible);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_NAME, name);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCKED, locked);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_USER_DATA, userData);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_HREF, href);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DESCRIPTION, description);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_POSITION, position);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DIMENSIONS, dimensions);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ROTATION, rotation);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_REGISTRATION_POINT, registrationPoint);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CREATED, created);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LAST_EDITED_BY, lastEditedBy);
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_ENTITY_HOST_TYPE, entityHostType, getEntityHostTypeAsString());
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_OWNING_AVATAR_ID, owningAvatarID);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_PARENT_ID, parentID);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_PARENT_JOINT_INDEX, parentJointIndex);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_QUERY_AA_CUBE, queryAACube);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CAN_CAST_SHADOW, canCastShadow);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_VISIBLE_IN_SECONDARY_CAMERA, isVisibleInSecondaryCamera);
    _grab.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);

    // Physics
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DENSITY, density);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_VELOCITY, velocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ANGULAR_VELOCITY, angularVelocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAVITY, gravity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ACCELERATION, acceleration);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DAMPING, damping);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ANGULAR_DAMPING, angularDamping);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_RESTITUTION, restitution);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_FRICTION, friction);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LIFETIME, lifetime);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLLISIONLESS, collisionless);
    COPY_PROXY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_COLLISIONLESS, collisionless, ignoreForCollisions, getCollisionless()); // legacy support
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLLISION_MASK, collisionMask);
    COPY_PROXY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_COLLISION_MASK, collisionMask, collidesWith, getCollisionMaskAsString());
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_DYNAMIC, dynamic);
    COPY_PROXY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_DYNAMIC, dynamic, collisionsWillMove, getDynamic()); // legacy support
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COLLISION_SOUND_URL, collisionSoundURL);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ACTION_DATA, actionData);

    // Cloning
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CLONEABLE, cloneable);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CLONE_LIFETIME, cloneLifetime);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CLONE_LIMIT, cloneLimit);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CLONE_DYNAMIC, cloneDynamic);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CLONE_AVATAR_ENTITY, cloneAvatarEntity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CLONE_ORIGIN_ID, cloneOriginID);

    // Scripts
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SCRIPT, script);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SCRIPT_TIMESTAMP, scriptTimestamp);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SERVER_SCRIPTS, serverScripts);

    // Certifiable Properties
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ITEM_NAME, itemName);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ITEM_DESCRIPTION, itemDescription);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ITEM_CATEGORIES, itemCategories);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ITEM_ARTIST, itemArtist);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ITEM_LICENSE, itemLicense);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LIMITED_RUN, limitedRun);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MARKETPLACE_ID, marketplaceID);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EDITION_NUMBER, editionNumber);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ENTITY_INSTANCE_NUMBER, entityInstanceNumber);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CERTIFICATE_ID, certificateID);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_STATIC_CERTIFICATE_VERSION, staticCertificateVersion);

    // Local props for scripts
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCAL_POSITION, localPosition);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCAL_ROTATION, localRotation);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCAL_VELOCITY, localVelocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCAL_ANGULAR_VELOCITY, localAngularVelocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LOCAL_DIMENSIONS, localDimensions);

    // Particles only
    if (_type == EntityTypes::ParticleEffect) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_SHAPE_TYPE, shapeType, getShapeTypeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR, color, u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA, alpha);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_TEXTURES, textures);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MAX_PARTICLES, maxParticles);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LIFESPAN, lifespan);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMITTING_PARTICLES, isEmitting);
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

        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR_SPREAD, colorSpread, u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR_START, colorStart, vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR_FINISH, colorFinish, vec3Color);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA_SPREAD, alphaSpread);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA_START, alphaStart);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA_FINISH, alphaFinish);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMITTER_SHOULD_TRAIL, emitterShouldTrail);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_PARTICLE_SPIN, particleSpin);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SPIN_SPREAD, spinSpread);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SPIN_START, spinStart);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SPIN_FINISH, spinFinish);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_PARTICLE_ROTATE_WITH_ENTITY, rotateWithEntity);
    }

    // Models only
    if (_type == EntityTypes::Model) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_SHAPE_TYPE, shapeType, getShapeTypeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COMPOUND_SHAPE_URL, compoundShapeURL);
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR, color, u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_TEXTURES, textures);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MODEL_URL, modelURL);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_JOINT_ROTATIONS_SET, jointRotationsSet);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_JOINT_ROTATIONS, jointRotations);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_JOINT_TRANSLATIONS_SET, jointTranslationsSet);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_JOINT_TRANSLATIONS, jointTranslations);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_RELAY_PARENT_JOINTS, relayParentJoints);
        if (!psuedoPropertyFlagsButDesiredEmpty) {
            _animation.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);
        }
    }

    // FIXME: Shouldn't provide a shapeType property for Box and Sphere entities.
    if (_type == EntityTypes::Box) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_SHAPE_TYPE, shapeType, QString("Box"));
    }
    if (_type == EntityTypes::Sphere) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_SHAPE_TYPE, shapeType, QString("Sphere"));
    }

    if (_type == EntityTypes::Box || _type == EntityTypes::Sphere || _type == EntityTypes::Shape) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR, color, u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA, alpha);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SHAPE, shape);
    }

    // Lights only
    if (_type == EntityTypes::Light) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR, color, u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_IS_SPOTLIGHT, isSpotlight);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_INTENSITY, intensity);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EXPONENT, exponent);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_CUTOFF, cutoff);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_FALLOFF_RADIUS, falloffRadius);
    }

    // Text only
    if (_type == EntityTypes::Text) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_TEXT, text);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LINE_HEIGHT, lineHeight);
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_TYPED(PROP_TEXT_COLOR, textColor, getTextColor(), u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_TEXT_ALPHA, textAlpha);
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_TYPED(PROP_BACKGROUND_COLOR, backgroundColor, getBackgroundColor(), u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_BACKGROUND_ALPHA, backgroundAlpha);
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_BILLBOARD_MODE, billboardMode, getBillboardModeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LEFT_MARGIN, leftMargin);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_RIGHT_MARGIN, rightMargin);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_TOP_MARGIN, topMargin);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_BOTTOM_MARGIN, bottomMargin);
    }

    // Zones only
    if (_type == EntityTypes::Zone) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_SHAPE_TYPE, shapeType, getShapeTypeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_COMPOUND_SHAPE_URL, compoundShapeURL);

        if (!psuedoPropertyFlagsButDesiredEmpty) {
            _keyLight.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);
            _ambientLight.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);
            _skybox.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);
            _haze.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);
            _bloom.copyToScriptValue(_desiredProperties, properties, engine, skipDefaults, defaultEntityProperties);
        }

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_FLYING_ALLOWED, flyingAllowed);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_GHOSTING_ALLOWED, ghostingAllowed);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_FILTER_URL, filterURL);

        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_KEY_LIGHT_MODE, keyLightMode, getKeyLightModeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_AMBIENT_LIGHT_MODE, ambientLightMode, getAmbientLightModeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_SKYBOX_MODE, skyboxMode, getSkyboxModeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_HAZE_MODE, hazeMode, getHazeModeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_BLOOM_MODE, bloomMode, getBloomModeAsString());
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

    // Lines
    if (_type == EntityTypes::Line) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR, color, u8vec3Color);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LINE_POINTS, linePoints);
    }

    // Polylines
    if (_type == EntityTypes::PolyLine) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR, color, u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_TEXTURES, textures);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LINE_POINTS, linePoints);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_STROKE_WIDTHS, strokeWidths);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_STROKE_NORMALS, normals);
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_STROKE_COLORS, strokeColors, qVectorVec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_IS_UV_MODE_STRETCH, isUVModeStretch);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LINE_GLOW, glow);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_LINE_FACE_CAMERA, faceCamera);
    }

    // Materials
    if (_type == EntityTypes::Material) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MATERIAL_URL, materialURL);
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_MATERIAL_MAPPING_MODE, materialMappingMode, getMaterialMappingModeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MATERIAL_PRIORITY, priority);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_PARENT_MATERIAL_NAME, parentMaterialName);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MATERIAL_MAPPING_POS, materialMappingPos);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MATERIAL_MAPPING_SCALE, materialMappingScale);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MATERIAL_MAPPING_ROT, materialMappingRot);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MATERIAL_DATA, materialData);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MATERIAL_REPEAT, materialRepeat);
    }

    // Image only
    if (_type == EntityTypes::Image) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR, color, u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA, alpha);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_IMAGE_URL, imageURL);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_EMISSIVE, emissive);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_KEEP_ASPECT_RATIO, keepAspectRatio);
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(PROP_BILLBOARD_MODE, billboardMode, getBillboardModeAsString());
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_SUB_IMAGE, subImage);

        // Handle conversions to old 'textures' property from "imageURL"
        if (((!psuedoPropertyFlagsButDesiredEmpty && _desiredProperties.isEmpty()) || _desiredProperties.getHasProperty(PROP_IMAGE_URL)) &&
                (!skipDefaults || defaultEntityProperties._imageURL != _imageURL)) {
            QScriptValue textures = engine->newObject();
            textures.setProperty("tex.picture", _imageURL);
            properties.setProperty("textures", textures);
        }
    }

    // Grid only
    if (_type == EntityTypes::Grid) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_TYPED(PROP_COLOR, color, u8vec3Color);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_ALPHA, alpha);

        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_GRID_FOLLOW_CAMERA, followCamera);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MAJOR_GRID_EVERY, majorGridEvery);
        COPY_PROPERTY_TO_QSCRIPTVALUE(PROP_MINOR_GRID_EVERY, minorGridEvery);
    }

    /**jsdoc
     * The axis-aligned bounding box of an entity.
     * @typedef {object} Entities.BoundingBox
     * @property {Vec3} brn - The bottom right near (minimum axes values) corner of the AA box.
     * @property {Vec3} tfl - The top far left (maximum axes values) corner of the AA box.
     * @property {Vec3} center - The center of the AA box.
     * @property {Vec3} dimensions - The dimensions of the AA box.
     */
    if (!skipDefaults && !strictSemantics &&
        (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::BoundingBox))) {

        AABox aaBox = getAABox();
        QScriptValue boundingBox = engine->newObject();
        QScriptValue bottomRightNear = vec3ToScriptValue(engine, aaBox.getCorner());
        QScriptValue topFarLeft = vec3ToScriptValue(engine, aaBox.calcTopFarLeft());
        QScriptValue center = vec3ToScriptValue(engine, aaBox.calcCenter());
        QScriptValue boundingBoxDimensions = vec3ToScriptValue(engine, aaBox.getDimensions());
        boundingBox.setProperty("brn", bottomRightNear);
        boundingBox.setProperty("tfl", topFarLeft);
        boundingBox.setProperty("center", center);
        boundingBox.setProperty("dimensions", boundingBoxDimensions);
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(boundingBox, boundingBox); // gettable, but not settable
    }

    QString textureNamesStr = QJsonDocument::fromVariant(_textureNames).toJson();
    if (!skipDefaults && !strictSemantics && (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::OriginalTextures))) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(originalTextures, textureNamesStr); // gettable, but not settable
    }

    // Rendering info
    if (!skipDefaults && !strictSemantics &&
        (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::RenderInfo))) {

        QScriptValue renderInfo = engine->newObject();

        /**jsdoc
         * Information on how an entity is rendered. Properties are only filled in for <code>Model</code> entities; other 
         * entity types have an empty object, <code>{}</code>.
         * @typedef {object} Entities.RenderInfo
         * @property {number} verticesCount - The number of vertices in the entity.
         * @property {number} texturesCount  - The number of textures in the entity.
         * @property {number} textureSize - The total size of the textures in the entity, in bytes.
         * @property {boolean} hasTransparent - Is <code>true</code> if any of the textures has transparency.
         * @property {number} drawCalls - The number of draw calls required to render the entity.
         */
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

    if (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::ClientOnly)) {
        properties.setProperty("clientOnly", convertScriptValue(engine, getEntityHostType() == entity::HostType::AVATAR));
    }
    if (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::AvatarEntity)) {
        properties.setProperty("avatarEntity", convertScriptValue(engine, getEntityHostType() == entity::HostType::AVATAR));
    }
    if (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::LocalEntity)) {
        properties.setProperty("localEntity", convertScriptValue(engine, getEntityHostType() == entity::HostType::LOCAL));
    }

    if (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::FaceCamera)) {
        properties.setProperty("faceCamera", convertScriptValue(engine, getBillboardMode() == BillboardMode::YAW));
    }
    if (!psuedoPropertyFlagsActive || psueudoPropertyFlags.test(EntityPsuedoPropertyFlag::IsFacingAvatar)) {
        properties.setProperty("isFacingAvatar", convertScriptValue(engine, getBillboardMode() == BillboardMode::FULL));
    }

    return properties;
}

void EntityItemProperties::copyFromScriptValue(const QScriptValue& object, bool honorReadOnly) {
    QScriptValue typeScriptValue = object.property("type");
    if (typeScriptValue.isValid()) {
        setType(typeScriptValue.toVariant().toString());
    }

    // Core
    if (!honorReadOnly) {
        // not handled yet
        // COPY_PROPERTY_FROM_QSCRIPTVALUE(simulationOwner, SimulationOwner, setSimulationOwner);
    }
    COPY_PROPERTY_FROM_QSCRIPTVALUE(visible, bool, setVisible);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(name, QString, setName);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(locked, bool, setLocked);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(userData, QString, setUserData);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(href, QString, setHref);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(description, QString, setDescription);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(position, vec3, setPosition);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(dimensions, vec3, setDimensions);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(rotation, quat, setRotation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(registrationPoint, vec3, setRegistrationPoint);
    if (!honorReadOnly) {
        COPY_PROPERTY_FROM_QSCRIPTVALUE(created, quint64, setCreated);
        COPY_PROPERTY_FROM_QSCRIPTVALUE(lastEditedBy, QUuid, setLastEditedBy);
        COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(entityHostType, EntityHostType);
        COPY_PROPERTY_FROM_QSCRIPTVALUE(owningAvatarID, QUuid, setOwningAvatarID);
    }
    COPY_PROPERTY_FROM_QSCRIPTVALUE(parentID, QUuid, setParentID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(parentJointIndex, quint16, setParentJointIndex);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(queryAACube, AACube, setQueryAACube); // TODO: should scripts be able to set this?
    COPY_PROPERTY_FROM_QSCRIPTVALUE(canCastShadow, bool, setCanCastShadow);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(isVisibleInSecondaryCamera, bool, setIsVisibleInSecondaryCamera);
    _grab.copyFromScriptValue(object, _defaultSettings);

    // Physics
    COPY_PROPERTY_FROM_QSCRIPTVALUE(density, float, setDensity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(velocity, vec3, setVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(angularVelocity, vec3, setAngularVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(gravity, vec3, setGravity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(acceleration, vec3, setAcceleration);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(damping, float, setDamping);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(angularDamping, float, setAngularDamping);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(restitution, float, setRestitution);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(friction, float, setFriction);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(lifetime, float, setLifetime);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(collisionless, bool, setCollisionless);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(ignoreForCollisions, bool, setCollisionless, getCollisionless); // legacy support
    COPY_PROPERTY_FROM_QSCRIPTVALUE(collisionMask, uint16_t, setCollisionMask);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(collidesWith, CollisionMask);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_GETTER(collisionsWillMove, bool, setDynamic, getDynamic); // legacy support
    COPY_PROPERTY_FROM_QSCRIPTVALUE(dynamic, bool, setDynamic);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(collisionSoundURL, QString, setCollisionSoundURL);
    if (!honorReadOnly) {
        COPY_PROPERTY_FROM_QSCRIPTVALUE(actionData, QByteArray, setActionData);
    }

    // Cloning
    COPY_PROPERTY_FROM_QSCRIPTVALUE(cloneable, bool, setCloneable);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(cloneLifetime, float, setCloneLifetime);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(cloneLimit, float, setCloneLimit);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(cloneDynamic, bool, setCloneDynamic);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(cloneAvatarEntity, bool, setCloneAvatarEntity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(cloneOriginID, QUuid, setCloneOriginID);

    // Scripts
    COPY_PROPERTY_FROM_QSCRIPTVALUE(script, QString, setScript);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(scriptTimestamp, quint64, setScriptTimestamp);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(serverScripts, QString, setServerScripts);

    // Certifiable Properties
    COPY_PROPERTY_FROM_QSCRIPTVALUE(itemName, QString, setItemName);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(itemDescription, QString, setItemDescription);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(itemCategories, QString, setItemCategories);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(itemArtist, QString, setItemArtist);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(itemLicense, QString, setItemLicense);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(limitedRun, quint32, setLimitedRun);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(marketplaceID, QString, setMarketplaceID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(editionNumber, quint32, setEditionNumber);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(entityInstanceNumber, quint32, setEntityInstanceNumber);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(certificateID, QString, setCertificateID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(staticCertificateVersion, quint32, setStaticCertificateVersion);

    // Script location data
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localPosition, vec3, setLocalPosition);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localRotation, quat, setLocalRotation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localVelocity, vec3, setLocalVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localAngularVelocity, vec3, setLocalAngularVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localDimensions, vec3, setLocalDimensions);

    // Common
    COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(shapeType, ShapeType);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(compoundShapeURL, QString, setCompoundShapeURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(color, u8vec3Color, setColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(alpha, float, setAlpha);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(textures, QString, setTextures);

    // Particles
    COPY_PROPERTY_FROM_QSCRIPTVALUE(maxParticles, quint32, setMaxParticles);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(lifespan, float, setLifespan);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(isEmitting, bool, setIsEmitting);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitRate, float, setEmitRate);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitSpeed, float, setEmitSpeed);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(speedSpread, float, setSpeedSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitOrientation, quat, setEmitOrientation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitDimensions, vec3, setEmitDimensions);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitRadiusStart, float, setEmitRadiusStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(polarStart, float, setPolarStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(polarFinish, float, setPolarFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(azimuthStart, float, setAzimuthStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(azimuthFinish, float, setAzimuthFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitAcceleration, vec3, setEmitAcceleration);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(accelerationSpread, vec3, setAccelerationSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(particleRadius, float, setParticleRadius);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(radiusSpread, float, setRadiusSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(radiusStart, float, setRadiusStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(radiusFinish, float, setRadiusFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(colorSpread, u8vec3Color, setColorSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(colorStart, vec3Color, setColorStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(colorFinish, vec3Color, setColorFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(alphaSpread, float, setAlphaSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(alphaStart, float, setAlphaStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(alphaFinish, float, setAlphaFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitterShouldTrail, bool, setEmitterShouldTrail);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(particleSpin, float, setParticleSpin);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(spinSpread, float, setSpinSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(spinStart, float, setSpinStart);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(spinFinish, float, setSpinFinish);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(rotateWithEntity, bool, setRotateWithEntity);

    // Model
    COPY_PROPERTY_FROM_QSCRIPTVALUE(modelURL, QString, setModelURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(jointRotationsSet, qVectorBool, setJointRotationsSet);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(jointRotations, qVectorQuat, setJointRotations);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(jointTranslationsSet, qVectorBool, setJointTranslationsSet);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(jointTranslations, qVectorVec3, setJointTranslations);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(relayParentJoints, bool, setRelayParentJoints);
    _animation.copyFromScriptValue(object, _defaultSettings);

    // Light
    COPY_PROPERTY_FROM_QSCRIPTVALUE(isSpotlight, bool, setIsSpotlight);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(intensity, float, setIntensity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(exponent, float, setExponent);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(cutoff, float, setCutoff);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(falloffRadius, float, setFalloffRadius);

    // Text
    COPY_PROPERTY_FROM_QSCRIPTVALUE(text, QString, setText);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(lineHeight, float, setLineHeight);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(textColor, u8vec3Color, setTextColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(textAlpha, float, setTextAlpha);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(backgroundColor, u8vec3Color, setBackgroundColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(backgroundAlpha, float, setBackgroundAlpha);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(billboardMode, BillboardMode);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(leftMargin, float, setLeftMargin);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(rightMargin, float, setRightMargin);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(topMargin, float, setTopMargin);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(bottomMargin, float, setBottomMargin);

    // Zone
    _keyLight.copyFromScriptValue(object, _defaultSettings);
    _ambientLight.copyFromScriptValue(object, _defaultSettings);
    _skybox.copyFromScriptValue(object, _defaultSettings);
    _haze.copyFromScriptValue(object, _defaultSettings);
    _bloom.copyFromScriptValue(object, _defaultSettings);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(flyingAllowed, bool, setFlyingAllowed);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(ghostingAllowed, bool, setGhostingAllowed);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(filterURL, QString, setFilterURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(keyLightMode, KeyLightMode);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(ambientLightMode, AmbientLightMode);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(skyboxMode, SkyboxMode);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(hazeMode, HazeMode);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(bloomMode, BloomMode);

    // Polyvox
    COPY_PROPERTY_FROM_QSCRIPTVALUE(voxelVolumeSize, vec3, setVoxelVolumeSize);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(voxelData, QByteArray, setVoxelData);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(voxelSurfaceStyle, uint16_t, setVoxelSurfaceStyle);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(xTextureURL, QString, setXTextureURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(yTextureURL, QString, setYTextureURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(zTextureURL, QString, setZTextureURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(xNNeighborID, EntityItemID, setXNNeighborID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(yNNeighborID, EntityItemID, setYNNeighborID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(zNNeighborID, EntityItemID, setZNNeighborID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(xPNeighborID, EntityItemID, setXPNeighborID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(yPNeighborID, EntityItemID, setYPNeighborID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(zPNeighborID, EntityItemID, setZPNeighborID);

    // Web
    COPY_PROPERTY_FROM_QSCRIPTVALUE(sourceUrl, QString, setSourceUrl);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(dpi, uint16_t, setDPI);

    // Polyline
    COPY_PROPERTY_FROM_QSCRIPTVALUE(linePoints, qVectorVec3, setLinePoints);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(strokeWidths, qVectorFloat, setStrokeWidths);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(normals, qVectorVec3, setNormals);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(strokeColors, qVectorVec3, setStrokeColors);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(isUVModeStretch, bool, setIsUVModeStretch);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(glow, bool, setGlow);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(faceCamera, bool, setFaceCamera);

    // Shape
    COPY_PROPERTY_FROM_QSCRIPTVALUE(shape, QString, setShape);

    // Material
    COPY_PROPERTY_FROM_QSCRIPTVALUE(materialURL, QString, setMaterialURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_ENUM(materialMappingMode, MaterialMappingMode);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(priority, quint16, setPriority);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(parentMaterialName, QString, setParentMaterialName);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(materialMappingPos, vec2, setMaterialMappingPos);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(materialMappingScale, vec2, setMaterialMappingScale);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(materialMappingRot, float, setMaterialMappingRot);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(materialData, QString, setMaterialData);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(materialRepeat, bool, setMaterialRepeat);

    // Image
    COPY_PROPERTY_FROM_QSCRIPTVALUE(imageURL, QString, setImageURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emissive, bool, setEmissive);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(keepAspectRatio, bool, setKeepAspectRatio);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(subImage, QRect, setSubImage);

    // Grid
    COPY_PROPERTY_FROM_QSCRIPTVALUE(followCamera, bool, setFollowCamera);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(majorGridEvery, uint32_t, setMajorGridEvery);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(minorGridEvery, float, setMinorGridEvery);

    // Handle conversions from old 'textures' property to "imageURL"
    {
        QScriptValue V = object.property("textures");
        if (_type == EntityTypes::Image && V.isValid() && !object.property("imageURL").isValid()) {
            bool isValid = false;
            QString textures = QString_convertFromScriptValue(V, isValid);
            if (isValid) {
                QVariantMap texturesMap = parseTexturesToMap(textures, QVariantMap());
                auto texPicture = texturesMap.find("tex.picture");
                if (texPicture != texturesMap.end()) {
                    auto imageURL = texPicture.value().toString();
                    if (_defaultSettings || imageURL != _imageURL) {
                        setImageURL(imageURL);
                    }
                }
            }
        }
    }

    // Handle old "faceCamera" and "isFacingAvatar" props
    {
        QScriptValue P = object.property("faceCamera");
        if (P.isValid() && !object.property("billboardMode").isValid()) {
            bool newValue = P.toVariant().toBool();
            bool oldValue = getBillboardMode() == BillboardMode::YAW;
            if (_defaultSettings || newValue != oldValue) {
                setBillboardMode(newValue ? BillboardMode::YAW : BillboardMode::NONE);
            }
        }
    }
    {
        QScriptValue P = object.property("isFacingAvatar");
        if (P.isValid() && !object.property("billboardMode").isValid() && !object.property("faceCamera").isValid()) {
            bool newValue = P.toVariant().toBool();
            bool oldValue = getBillboardMode() == BillboardMode::FULL;
            if (_defaultSettings || newValue != oldValue) {
                setBillboardMode(newValue ? BillboardMode::FULL : BillboardMode::NONE);
            }
        }
    }

    _lastEdited = usecTimestampNow();
}

void EntityItemProperties::copyFromJSONString(QScriptEngine& scriptEngine, const QString& jsonString) {
    // DANGER: this method is expensive
    QJsonDocument propertiesDoc = QJsonDocument::fromJson(jsonString.toUtf8());
    QJsonObject propertiesObj = propertiesDoc.object();
    QVariant propertiesVariant(propertiesObj);
    QVariantMap propertiesMap = propertiesVariant.toMap();
    QScriptValue propertiesScriptValue = variantMapToScriptValue(propertiesMap, scriptEngine);
    bool honorReadOnly = true;
    copyFromScriptValue(propertiesScriptValue, honorReadOnly);
}


void EntityItemProperties::merge(const EntityItemProperties& other) {
    // Core
    COPY_PROPERTY_IF_CHANGED(simulationOwner);
    COPY_PROPERTY_IF_CHANGED(visible);
    COPY_PROPERTY_IF_CHANGED(name);
    COPY_PROPERTY_IF_CHANGED(locked);
    COPY_PROPERTY_IF_CHANGED(userData);
    COPY_PROPERTY_IF_CHANGED(href);
    COPY_PROPERTY_IF_CHANGED(description);
    COPY_PROPERTY_IF_CHANGED(position);
    COPY_PROPERTY_IF_CHANGED(dimensions);
    COPY_PROPERTY_IF_CHANGED(rotation);
    COPY_PROPERTY_IF_CHANGED(registrationPoint);
    COPY_PROPERTY_IF_CHANGED(created);
    COPY_PROPERTY_IF_CHANGED(lastEditedBy);
    COPY_PROPERTY_IF_CHANGED(entityHostType);
    COPY_PROPERTY_IF_CHANGED(owningAvatarID);
    COPY_PROPERTY_IF_CHANGED(parentID);
    COPY_PROPERTY_IF_CHANGED(parentJointIndex);
    COPY_PROPERTY_IF_CHANGED(queryAACube);
    COPY_PROPERTY_IF_CHANGED(canCastShadow);
    COPY_PROPERTY_IF_CHANGED(isVisibleInSecondaryCamera);
    _grab.merge(other._grab);

    // Physics
    COPY_PROPERTY_IF_CHANGED(density);
    COPY_PROPERTY_IF_CHANGED(velocity);
    COPY_PROPERTY_IF_CHANGED(angularVelocity);
    COPY_PROPERTY_IF_CHANGED(gravity);
    COPY_PROPERTY_IF_CHANGED(acceleration);
    COPY_PROPERTY_IF_CHANGED(damping);
    COPY_PROPERTY_IF_CHANGED(angularDamping);
    COPY_PROPERTY_IF_CHANGED(restitution);
    COPY_PROPERTY_IF_CHANGED(friction);
    COPY_PROPERTY_IF_CHANGED(lifetime);
    COPY_PROPERTY_IF_CHANGED(collisionless);
    COPY_PROPERTY_IF_CHANGED(collisionMask);
    COPY_PROPERTY_IF_CHANGED(dynamic);
    COPY_PROPERTY_IF_CHANGED(collisionSoundURL);
    COPY_PROPERTY_IF_CHANGED(actionData);

    // Cloning
    COPY_PROPERTY_IF_CHANGED(cloneable);
    COPY_PROPERTY_IF_CHANGED(cloneLifetime);
    COPY_PROPERTY_IF_CHANGED(cloneLimit);
    COPY_PROPERTY_IF_CHANGED(cloneDynamic);
    COPY_PROPERTY_IF_CHANGED(cloneAvatarEntity);
    COPY_PROPERTY_IF_CHANGED(cloneOriginID);

    // Scripts
    COPY_PROPERTY_IF_CHANGED(script);
    COPY_PROPERTY_IF_CHANGED(scriptTimestamp);
    COPY_PROPERTY_IF_CHANGED(serverScripts);

    // Certifiable Properties
    COPY_PROPERTY_IF_CHANGED(itemName);
    COPY_PROPERTY_IF_CHANGED(itemDescription);
    COPY_PROPERTY_IF_CHANGED(itemCategories);
    COPY_PROPERTY_IF_CHANGED(itemArtist);
    COPY_PROPERTY_IF_CHANGED(itemLicense);
    COPY_PROPERTY_IF_CHANGED(limitedRun);
    COPY_PROPERTY_IF_CHANGED(marketplaceID);
    COPY_PROPERTY_IF_CHANGED(editionNumber);
    COPY_PROPERTY_IF_CHANGED(entityInstanceNumber);
    COPY_PROPERTY_IF_CHANGED(certificateID);
    COPY_PROPERTY_IF_CHANGED(staticCertificateVersion);

    // Local props for scripts
    COPY_PROPERTY_IF_CHANGED(localPosition);
    COPY_PROPERTY_IF_CHANGED(localRotation);
    COPY_PROPERTY_IF_CHANGED(localVelocity);
    COPY_PROPERTY_IF_CHANGED(localAngularVelocity);
    COPY_PROPERTY_IF_CHANGED(localDimensions);

    // Common
    COPY_PROPERTY_IF_CHANGED(shapeType);
    COPY_PROPERTY_IF_CHANGED(compoundShapeURL);
    COPY_PROPERTY_IF_CHANGED(color);
    COPY_PROPERTY_IF_CHANGED(alpha);
    COPY_PROPERTY_IF_CHANGED(textures);

    // Particles
    COPY_PROPERTY_IF_CHANGED(maxParticles);
    COPY_PROPERTY_IF_CHANGED(lifespan);
    COPY_PROPERTY_IF_CHANGED(isEmitting);
    COPY_PROPERTY_IF_CHANGED(emitRate);
    COPY_PROPERTY_IF_CHANGED(emitSpeed);
    COPY_PROPERTY_IF_CHANGED(speedSpread);
    COPY_PROPERTY_IF_CHANGED(emitOrientation);
    COPY_PROPERTY_IF_CHANGED(emitDimensions);
    COPY_PROPERTY_IF_CHANGED(emitRadiusStart);
    COPY_PROPERTY_IF_CHANGED(polarStart);
    COPY_PROPERTY_IF_CHANGED(polarFinish);
    COPY_PROPERTY_IF_CHANGED(azimuthStart);
    COPY_PROPERTY_IF_CHANGED(azimuthFinish);
    COPY_PROPERTY_IF_CHANGED(emitAcceleration);
    COPY_PROPERTY_IF_CHANGED(accelerationSpread);
    COPY_PROPERTY_IF_CHANGED(particleRadius);
    COPY_PROPERTY_IF_CHANGED(radiusSpread);
    COPY_PROPERTY_IF_CHANGED(radiusStart);
    COPY_PROPERTY_IF_CHANGED(radiusFinish);
    COPY_PROPERTY_IF_CHANGED(colorSpread);
    COPY_PROPERTY_IF_CHANGED(colorStart);
    COPY_PROPERTY_IF_CHANGED(colorFinish);
    COPY_PROPERTY_IF_CHANGED(alphaSpread);
    COPY_PROPERTY_IF_CHANGED(alphaStart);
    COPY_PROPERTY_IF_CHANGED(alphaFinish);
    COPY_PROPERTY_IF_CHANGED(emitterShouldTrail);
    COPY_PROPERTY_IF_CHANGED(particleSpin);
    COPY_PROPERTY_IF_CHANGED(spinSpread);
    COPY_PROPERTY_IF_CHANGED(spinStart);
    COPY_PROPERTY_IF_CHANGED(spinFinish);
    COPY_PROPERTY_IF_CHANGED(rotateWithEntity);

    // Model
    COPY_PROPERTY_IF_CHANGED(modelURL);
    COPY_PROPERTY_IF_CHANGED(jointRotationsSet);
    COPY_PROPERTY_IF_CHANGED(jointRotations);
    COPY_PROPERTY_IF_CHANGED(jointTranslationsSet);
    COPY_PROPERTY_IF_CHANGED(jointTranslations);
    COPY_PROPERTY_IF_CHANGED(relayParentJoints);
    _animation.merge(other._animation);

    // Light
    COPY_PROPERTY_IF_CHANGED(isSpotlight);
    COPY_PROPERTY_IF_CHANGED(intensity);
    COPY_PROPERTY_IF_CHANGED(exponent);
    COPY_PROPERTY_IF_CHANGED(cutoff);
    COPY_PROPERTY_IF_CHANGED(falloffRadius);

    // Text
    COPY_PROPERTY_IF_CHANGED(text);
    COPY_PROPERTY_IF_CHANGED(lineHeight);
    COPY_PROPERTY_IF_CHANGED(textColor);
    COPY_PROPERTY_IF_CHANGED(textAlpha);
    COPY_PROPERTY_IF_CHANGED(backgroundColor);
    COPY_PROPERTY_IF_CHANGED(backgroundAlpha);
    COPY_PROPERTY_IF_CHANGED(billboardMode);
    COPY_PROPERTY_IF_CHANGED(leftMargin);
    COPY_PROPERTY_IF_CHANGED(rightMargin);
    COPY_PROPERTY_IF_CHANGED(topMargin);
    COPY_PROPERTY_IF_CHANGED(bottomMargin);

    // Zone
    _keyLight.merge(other._keyLight);
    _ambientLight.merge(other._ambientLight);
    _skybox.merge(other._skybox);
    _haze.merge(other._haze);
    _bloom.merge(other._bloom);
    COPY_PROPERTY_IF_CHANGED(flyingAllowed);
    COPY_PROPERTY_IF_CHANGED(ghostingAllowed);
    COPY_PROPERTY_IF_CHANGED(filterURL);
    COPY_PROPERTY_IF_CHANGED(keyLightMode);
    COPY_PROPERTY_IF_CHANGED(ambientLightMode);
    COPY_PROPERTY_IF_CHANGED(skyboxMode);
    COPY_PROPERTY_IF_CHANGED(hazeMode);
    COPY_PROPERTY_IF_CHANGED(bloomMode);

    // Polyvox
    COPY_PROPERTY_IF_CHANGED(voxelVolumeSize);
    COPY_PROPERTY_IF_CHANGED(voxelData);
    COPY_PROPERTY_IF_CHANGED(voxelSurfaceStyle);
    COPY_PROPERTY_IF_CHANGED(xTextureURL);
    COPY_PROPERTY_IF_CHANGED(yTextureURL);
    COPY_PROPERTY_IF_CHANGED(zTextureURL);
    COPY_PROPERTY_IF_CHANGED(xNNeighborID);
    COPY_PROPERTY_IF_CHANGED(yNNeighborID);
    COPY_PROPERTY_IF_CHANGED(zNNeighborID);
    COPY_PROPERTY_IF_CHANGED(xPNeighborID);
    COPY_PROPERTY_IF_CHANGED(yPNeighborID);
    COPY_PROPERTY_IF_CHANGED(zPNeighborID);

    // Web
    COPY_PROPERTY_IF_CHANGED(sourceUrl);
    COPY_PROPERTY_IF_CHANGED(dpi);

    // Polyline
    COPY_PROPERTY_IF_CHANGED(linePoints);
    COPY_PROPERTY_IF_CHANGED(strokeWidths);
    COPY_PROPERTY_IF_CHANGED(normals);
    COPY_PROPERTY_IF_CHANGED(strokeColors);
    COPY_PROPERTY_IF_CHANGED(isUVModeStretch);
    COPY_PROPERTY_IF_CHANGED(glow);
    COPY_PROPERTY_IF_CHANGED(faceCamera);

    // Shape
    COPY_PROPERTY_IF_CHANGED(shape);

    // Material
    COPY_PROPERTY_IF_CHANGED(materialURL);
    COPY_PROPERTY_IF_CHANGED(materialMappingMode);
    COPY_PROPERTY_IF_CHANGED(priority);
    COPY_PROPERTY_IF_CHANGED(parentMaterialName);
    COPY_PROPERTY_IF_CHANGED(materialMappingPos);
    COPY_PROPERTY_IF_CHANGED(materialMappingScale);
    COPY_PROPERTY_IF_CHANGED(materialMappingRot);
    COPY_PROPERTY_IF_CHANGED(materialData);
    COPY_PROPERTY_IF_CHANGED(materialRepeat);

    // Image
    COPY_PROPERTY_IF_CHANGED(imageURL);
    COPY_PROPERTY_IF_CHANGED(emissive);
    COPY_PROPERTY_IF_CHANGED(keepAspectRatio);
    COPY_PROPERTY_IF_CHANGED(subImage);

    // Grid
    COPY_PROPERTY_IF_CHANGED(followCamera);
    COPY_PROPERTY_IF_CHANGED(majorGridEvery);
    COPY_PROPERTY_IF_CHANGED(minorGridEvery);

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
}

void EntityPropertyFlagsFromScriptValue(const QScriptValue& object, EntityPropertyFlags& flags) {
    EntityItemProperties::entityPropertyFlagsFromScriptValue(object, flags);
}


QScriptValue EntityItemProperties::entityPropertyFlagsToScriptValue(QScriptEngine* engine, const EntityPropertyFlags& flags) {
    QScriptValue result = engine->newObject();
    return result;
}

void EntityItemProperties::entityPropertyFlagsFromScriptValue(const QScriptValue& object, EntityPropertyFlags& flags) {
    if (object.isString()) {
        EntityPropertyInfo propertyInfo;
        if (getPropertyInfo(object.toString(), propertyInfo)) {
            flags << propertyInfo.propertyEnum;
        }
    }
    else if (object.isArray()) {
        quint32 length = object.property("length").toInt32();
        for (quint32 i = 0; i < length; i++) {
            QString propertyName = object.property(i).toString();
            EntityPropertyInfo propertyInfo;
            if (getPropertyInfo(propertyName, propertyInfo)) {
                flags << propertyInfo.propertyEnum;
            }
        }
    }
}

static QHash<QString, EntityPropertyInfo> _propertyInfos;
static QHash<EntityPropertyList, QString> _enumsToPropertyStrings;

bool EntityItemProperties::getPropertyInfo(const QString& propertyName, EntityPropertyInfo& propertyInfo) {

    static std::once_flag initMap;

    std::call_once(initMap, []() {
        // Core
        ADD_PROPERTY_TO_MAP(PROP_SIMULATION_OWNER, SimulationOwner, simulationOwner, SimulationOwner);
        ADD_PROPERTY_TO_MAP(PROP_VISIBLE, Visible, visible, bool);
        ADD_PROPERTY_TO_MAP(PROP_NAME, Name, name, QString);
        ADD_PROPERTY_TO_MAP(PROP_LOCKED, Locked, locked, bool);
        ADD_PROPERTY_TO_MAP(PROP_USER_DATA, UserData, userData, QString);
        ADD_PROPERTY_TO_MAP(PROP_HREF, Href, href, QString);
        ADD_PROPERTY_TO_MAP(PROP_DESCRIPTION, Description, description, QString);
        ADD_PROPERTY_TO_MAP(PROP_POSITION, Position, position, vec3);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_DIMENSIONS, Dimensions, dimensions, vec3, ENTITY_ITEM_MIN_DIMENSION, FLT_MAX);
        ADD_PROPERTY_TO_MAP(PROP_ROTATION, Rotation, rotation, quat);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_REGISTRATION_POINT, RegistrationPoint, registrationPoint, vec3, 
                                       ENTITY_ITEM_MIN_REGISTRATION_POINT, ENTITY_ITEM_MAX_REGISTRATION_POINT);
        ADD_PROPERTY_TO_MAP(PROP_CREATED, Created, created, quint64);
        ADD_PROPERTY_TO_MAP(PROP_LAST_EDITED_BY, LastEditedBy, lastEditedBy, QUuid);
        ADD_PROPERTY_TO_MAP(PROP_ENTITY_HOST_TYPE, EntityHostType, entityHostType, entity::HostType);
        ADD_PROPERTY_TO_MAP(PROP_OWNING_AVATAR_ID, OwningAvatarID, owningAvatarID, QUuid);
        ADD_PROPERTY_TO_MAP(PROP_PARENT_ID, ParentID, parentID, QUuid);
        ADD_PROPERTY_TO_MAP(PROP_PARENT_JOINT_INDEX, ParentJointIndex, parentJointIndex, uint16_t);
        ADD_PROPERTY_TO_MAP(PROP_QUERY_AA_CUBE, QueryAACube, queryAACube, AACube);
        ADD_PROPERTY_TO_MAP(PROP_CAN_CAST_SHADOW, CanCastShadow, canCastShadow, bool);
        ADD_PROPERTY_TO_MAP(PROP_VISIBLE_IN_SECONDARY_CAMERA, IsVisibleInSecondaryCamera, isVisibleInSecondaryCamera, bool);
        { // Grab
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_GRABBABLE, Grab, grab, Grabbable, grabbable);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_KINEMATIC, Grab, grab, GrabKinematic, grabKinematic);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_FOLLOWS_CONTROLLER, Grab, grab, GrabFollowsController, grabFollowsController);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_TRIGGERABLE, Grab, grab, Triggerable, triggerable);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_EQUIPPABLE, Grab, grab, Equippable, equippable);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_DELEGATE_TO_PARENT, Grab, grab, GrabDelegateToParent, grabDelegateToParent);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, Grab, grab,
                                      EquippableLeftPosition, equippableLeftPosition);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, Grab, grab,
                                      EquippableLeftRotation, equippableLeftRotation);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, Grab, grab,
                                      EquippableRightPosition, equippableRightPosition);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, Grab, grab,
                                      EquippableRightRotation, equippableRightRotation);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_EQUIPPABLE_INDICATOR_URL, Grab, grab,
                                      EquippableIndicatorURL, equippableIndicatorURL);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_EQUIPPABLE_INDICATOR_SCALE, Grab, grab,
                                      EquippableIndicatorScale, equippableIndicatorScale);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_GRAB_EQUIPPABLE_INDICATOR_OFFSET, Grab, grab,
                                      EquippableIndicatorOffset, equippableIndicatorOffset);
        }

        // Physics
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_DENSITY, Density, density, float, 
                                       ENTITY_ITEM_MIN_DENSITY, ENTITY_ITEM_MAX_DENSITY);
        ADD_PROPERTY_TO_MAP(PROP_VELOCITY, Velocity, velocity, vec3);
        ADD_PROPERTY_TO_MAP(PROP_ANGULAR_VELOCITY, AngularVelocity, angularVelocity, vec3);
        ADD_PROPERTY_TO_MAP(PROP_GRAVITY, Gravity, gravity, vec3);
        ADD_PROPERTY_TO_MAP(PROP_ACCELERATION, Acceleration, acceleration, vec3);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_DAMPING, Damping, damping, float, 
                                       ENTITY_ITEM_MIN_DAMPING, ENTITY_ITEM_MAX_DAMPING);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_ANGULAR_DAMPING, AngularDamping, angularDamping, float, 
                                       ENTITY_ITEM_MIN_DAMPING, ENTITY_ITEM_MAX_DAMPING);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_RESTITUTION, Restitution, restitution, float, 
                                       ENTITY_ITEM_MIN_RESTITUTION, ENTITY_ITEM_MAX_RESTITUTION);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_FRICTION, Friction, friction, float, 
                                       ENTITY_ITEM_MIN_FRICTION, ENTITY_ITEM_MAX_FRICTION);
        ADD_PROPERTY_TO_MAP(PROP_LIFETIME, Lifetime, lifetime, float);
        ADD_PROPERTY_TO_MAP(PROP_COLLISIONLESS, Collisionless, collisionless, bool);
        ADD_PROPERTY_TO_MAP(PROP_COLLISIONLESS, unused, ignoreForCollisions, unused); // legacy support
        ADD_PROPERTY_TO_MAP(PROP_COLLISION_MASK, unused, collisionMask, unused);
        ADD_PROPERTY_TO_MAP(PROP_COLLISION_MASK, unused, collidesWith, unused);
        ADD_PROPERTY_TO_MAP(PROP_DYNAMIC, unused, collisionsWillMove, unused); // legacy support
        ADD_PROPERTY_TO_MAP(PROP_DYNAMIC, unused, dynamic, unused);
        ADD_PROPERTY_TO_MAP(PROP_COLLISION_SOUND_URL, CollisionSoundURL, collisionSoundURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_ACTION_DATA, ActionData, actionData, QByteArray);

        // Cloning
        ADD_PROPERTY_TO_MAP(PROP_CLONEABLE, Cloneable, cloneable, bool);
        ADD_PROPERTY_TO_MAP(PROP_CLONE_LIFETIME, CloneLifetime, cloneLifetime, float);
        ADD_PROPERTY_TO_MAP(PROP_CLONE_LIMIT, CloneLimit, cloneLimit, float);
        ADD_PROPERTY_TO_MAP(PROP_CLONE_DYNAMIC, CloneDynamic, cloneDynamic, bool);
        ADD_PROPERTY_TO_MAP(PROP_CLONE_AVATAR_ENTITY, CloneAvatarEntity, cloneAvatarEntity, bool);
        ADD_PROPERTY_TO_MAP(PROP_CLONE_ORIGIN_ID, CloneOriginID, cloneOriginID, QUuid);

        // Scripts
        ADD_PROPERTY_TO_MAP(PROP_SCRIPT, Script, script, QString);
        ADD_PROPERTY_TO_MAP(PROP_SCRIPT_TIMESTAMP, ScriptTimestamp, scriptTimestamp, quint64);
        ADD_PROPERTY_TO_MAP(PROP_SERVER_SCRIPTS, ServerScripts, serverScripts, QString);

        // Certifiable Properties
        ADD_PROPERTY_TO_MAP(PROP_ITEM_NAME, ItemName, itemName, QString);
        ADD_PROPERTY_TO_MAP(PROP_ITEM_DESCRIPTION, ItemDescription, itemDescription, QString);
        ADD_PROPERTY_TO_MAP(PROP_ITEM_CATEGORIES, ItemCategories, itemCategories, QString);
        ADD_PROPERTY_TO_MAP(PROP_ITEM_ARTIST, ItemArtist, itemArtist, QString);
        ADD_PROPERTY_TO_MAP(PROP_ITEM_LICENSE, ItemLicense, itemLicense, QString);
        ADD_PROPERTY_TO_MAP(PROP_LIMITED_RUN, LimitedRun, limitedRun, quint32);
        ADD_PROPERTY_TO_MAP(PROP_MARKETPLACE_ID, MarketplaceID, marketplaceID, QString);
        ADD_PROPERTY_TO_MAP(PROP_EDITION_NUMBER, EditionNumber, editionNumber, quint32);
        ADD_PROPERTY_TO_MAP(PROP_ENTITY_INSTANCE_NUMBER, EntityInstanceNumber, entityInstanceNumber, quint32);
        ADD_PROPERTY_TO_MAP(PROP_CERTIFICATE_ID, CertificateID, certificateID, QString);
        ADD_PROPERTY_TO_MAP(PROP_STATIC_CERTIFICATE_VERSION, StaticCertificateVersion, staticCertificateVersion, quint32);

        // Local script props
        ADD_PROPERTY_TO_MAP(PROP_LOCAL_POSITION, LocalPosition, localPosition, vec3);
        ADD_PROPERTY_TO_MAP(PROP_LOCAL_ROTATION, LocalRotation, localRotation, quat);
        ADD_PROPERTY_TO_MAP(PROP_LOCAL_VELOCITY, LocalVelocity, localVelocity, vec3);
        ADD_PROPERTY_TO_MAP(PROP_LOCAL_ANGULAR_VELOCITY, LocalAngularVelocity, localAngularVelocity, vec3);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_LOCAL_DIMENSIONS, LocalDimensions, localDimensions, vec3, 
                                       ENTITY_ITEM_MIN_DIMENSION, FLT_MAX);

        // Common
        ADD_PROPERTY_TO_MAP(PROP_SHAPE_TYPE, ShapeType, shapeType, ShapeType);
        ADD_PROPERTY_TO_MAP(PROP_COMPOUND_SHAPE_URL, CompoundShapeURL, compoundShapeURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_COLOR, Color, color, u8vec3Color);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_ALPHA, Alpha, alpha, float, particle::MINIMUM_ALPHA, particle::MAXIMUM_ALPHA);
        ADD_PROPERTY_TO_MAP(PROP_TEXTURES, Textures, textures, QString);

        // Particles
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_MAX_PARTICLES, MaxParticles, maxParticles, quint32, 
                                       particle::MINIMUM_MAX_PARTICLES, particle::MAXIMUM_MAX_PARTICLES);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_LIFESPAN, Lifespan, lifespan, float, 
                                       particle::MINIMUM_LIFESPAN, particle::MAXIMUM_LIFESPAN);
        ADD_PROPERTY_TO_MAP(PROP_EMITTING_PARTICLES, IsEmitting, isEmitting, bool);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_EMIT_RATE, EmitRate, emitRate, float, 
                                       particle::MINIMUM_EMIT_RATE, particle::MAXIMUM_EMIT_RATE);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_EMIT_SPEED, EmitSpeed, emitSpeed, vec3, 
                                       particle::MINIMUM_EMIT_SPEED, particle::MAXIMUM_EMIT_SPEED);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_SPEED_SPREAD, SpeedSpread, speedSpread, vec3, 
                                       particle::MINIMUM_EMIT_SPEED, particle::MAXIMUM_EMIT_SPEED);
        ADD_PROPERTY_TO_MAP(PROP_EMIT_ORIENTATION, EmitOrientation, emitOrientation, quat);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_EMIT_DIMENSIONS, EmitDimensions, emitDimensions, vec3, 
                                       particle::MINIMUM_EMIT_DIMENSION, particle::MAXIMUM_EMIT_DIMENSION);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_EMIT_RADIUS_START, EmitRadiusStart, emitRadiusStart, float, 
                                       particle::MINIMUM_EMIT_RADIUS_START, particle::MAXIMUM_EMIT_RADIUS_START);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_POLAR_START, EmitPolarStart, polarStart, float, 
                                       particle::MINIMUM_POLAR, particle::MAXIMUM_POLAR);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_POLAR_FINISH, EmitPolarFinish, polarFinish, float, 
                                       particle::MINIMUM_POLAR, particle::MAXIMUM_POLAR);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_AZIMUTH_START, EmitAzimuthStart, azimuthStart, float, 
                                       particle::MINIMUM_AZIMUTH, particle::MAXIMUM_AZIMUTH);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_AZIMUTH_FINISH, EmitAzimuthFinish, azimuthFinish, float, 
                                       particle::MINIMUM_AZIMUTH, particle::MAXIMUM_AZIMUTH);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_EMIT_ACCELERATION, EmitAcceleration, emitAcceleration, vec3, 
                                       particle::MINIMUM_EMIT_ACCELERATION, particle::MAXIMUM_EMIT_ACCELERATION);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_ACCELERATION_SPREAD, AccelerationSpread, accelerationSpread, vec3, 
                                       particle::MINIMUM_ACCELERATION_SPREAD, particle::MAXIMUM_ACCELERATION_SPREAD);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_PARTICLE_RADIUS, ParticleRadius, particleRadius, float, 
                                       particle::MINIMUM_PARTICLE_RADIUS, particle::MAXIMUM_PARTICLE_RADIUS);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_RADIUS_SPREAD, RadiusSpread, radiusSpread, float, 
                                       particle::MINIMUM_PARTICLE_RADIUS, particle::MAXIMUM_PARTICLE_RADIUS);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_RADIUS_START, RadiusStart, radiusStart, float, 
                                       particle::MINIMUM_PARTICLE_RADIUS, particle::MAXIMUM_PARTICLE_RADIUS);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_RADIUS_FINISH, RadiusFinish, radiusFinish, float, 
                                       particle::MINIMUM_PARTICLE_RADIUS, particle::MAXIMUM_PARTICLE_RADIUS);
        ADD_PROPERTY_TO_MAP(PROP_COLOR_SPREAD, ColorSpread, colorSpread, u8vec3Color);
        ADD_PROPERTY_TO_MAP(PROP_COLOR_START, ColorStart, colorStart, vec3Color);
        ADD_PROPERTY_TO_MAP(PROP_COLOR_FINISH, ColorFinish, colorFinish, vec3Color);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_ALPHA_SPREAD, AlphaSpread, alphaSpread, float, 
                                       particle::MINIMUM_ALPHA, particle::MAXIMUM_ALPHA);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_ALPHA_START, AlphaStart, alphaStart, float, 
                                       particle::MINIMUM_ALPHA, particle::MAXIMUM_ALPHA);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_ALPHA_FINISH, AlphaFinish, alphaFinish, float, 
                                       particle::MINIMUM_ALPHA, particle::MAXIMUM_ALPHA);
        ADD_PROPERTY_TO_MAP(PROP_EMITTER_SHOULD_TRAIL, EmitterShouldTrail, emitterShouldTrail, bool);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_PARTICLE_SPIN, ParticleSpin, particleSpin, float, 
                                       particle::MINIMUM_PARTICLE_SPIN, particle::MAXIMUM_PARTICLE_SPIN);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_SPIN_SPREAD, SpinSpread, spinSpread, float, 
                                       particle::MINIMUM_PARTICLE_SPIN, particle::MAXIMUM_PARTICLE_SPIN);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_SPIN_START, SpinStart, spinStart, float, 
                                       particle::MINIMUM_PARTICLE_SPIN, particle::MAXIMUM_PARTICLE_SPIN);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_SPIN_FINISH, SpinFinish, spinFinish, float, 
                                       particle::MINIMUM_PARTICLE_SPIN, particle::MAXIMUM_PARTICLE_SPIN);
        ADD_PROPERTY_TO_MAP(PROP_PARTICLE_ROTATE_WITH_ENTITY, RotateWithEntity, rotateWithEntity, float);

        // Model
        ADD_PROPERTY_TO_MAP(PROP_MODEL_URL, ModelURL, modelURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_JOINT_ROTATIONS_SET, JointRotationsSet, jointRotationsSet, QVector<bool>);
        ADD_PROPERTY_TO_MAP(PROP_JOINT_ROTATIONS, JointRotations, jointRotations, QVector<quat>);
        ADD_PROPERTY_TO_MAP(PROP_JOINT_TRANSLATIONS_SET, JointTranslationsSet, jointTranslationsSet, QVector<bool>);
        ADD_PROPERTY_TO_MAP(PROP_JOINT_TRANSLATIONS, JointTranslations, jointTranslations, QVector<vec3>);
        ADD_PROPERTY_TO_MAP(PROP_RELAY_PARENT_JOINTS, RelayParentJoints, relayParentJoints, bool);
        { // Animation
            ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_URL, Animation, animation, URL, url);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_ALLOW_TRANSLATION, Animation, animation, AllowTranslation, allowTranslation);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_FPS, Animation, animation, FPS, fps);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_FRAME_INDEX, Animation, animation, CurrentFrame, currentFrame);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_PLAYING, Animation, animation, Running, running);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_LOOP, Animation, animation, Loop, loop);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_FIRST_FRAME, Animation, animation, FirstFrame, firstFrame);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_LAST_FRAME, Animation, animation, LastFrame, lastFrame);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_ANIMATION_HOLD, Animation, animation, Hold, hold);
        }

        // Light
        ADD_PROPERTY_TO_MAP(PROP_IS_SPOTLIGHT, IsSpotlight, isSpotlight, bool);
        ADD_PROPERTY_TO_MAP(PROP_INTENSITY, Intensity, intensity, float);
        ADD_PROPERTY_TO_MAP(PROP_EXPONENT, Exponent, exponent, float);
        ADD_PROPERTY_TO_MAP_WITH_RANGE(PROP_CUTOFF, Cutoff, cutoff, float, 
                                       LightEntityItem::MIN_CUTOFF, LightEntityItem::MAX_CUTOFF);
        ADD_PROPERTY_TO_MAP(PROP_FALLOFF_RADIUS, FalloffRadius, falloffRadius, float);

        // Text
        ADD_PROPERTY_TO_MAP(PROP_TEXT, Text, text, QString);
        ADD_PROPERTY_TO_MAP(PROP_LINE_HEIGHT, LineHeight, lineHeight, float);
        ADD_PROPERTY_TO_MAP(PROP_TEXT_COLOR, TextColor, textColor, u8vec3Color);
        ADD_PROPERTY_TO_MAP(PROP_TEXT_ALPHA, TextAlpha, textAlpha, float);
        ADD_PROPERTY_TO_MAP(PROP_BACKGROUND_COLOR, BackgroundColor, backgroundColor, u8vec3Color);
        ADD_PROPERTY_TO_MAP(PROP_BACKGROUND_ALPHA, BackgroundAlpha, backgroundAlpha, float);
        ADD_PROPERTY_TO_MAP(PROP_BILLBOARD_MODE, BillboardMode, billboardMode, BillboardMode);
        ADD_PROPERTY_TO_MAP(PROP_LEFT_MARGIN, LeftMargin, leftMargin, float);
        ADD_PROPERTY_TO_MAP(PROP_RIGHT_MARGIN, RightMargin, rightMargin, float);
        ADD_PROPERTY_TO_MAP(PROP_TOP_MARGIN, TopMargin, topMargin, float);
        ADD_PROPERTY_TO_MAP(PROP_BOTTOM_MARGIN, BottomMargin, bottomMargin, float);

        // Zone
        { // Keylight
            ADD_GROUP_PROPERTY_TO_MAP(PROP_KEYLIGHT_COLOR, KeyLight, keyLight, Color, color);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_KEYLIGHT_INTENSITY, KeyLight, keyLight, Intensity, intensity);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_KEYLIGHT_DIRECTION, KeyLight, keylight, Direction, direction);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_KEYLIGHT_CAST_SHADOW, KeyLight, keyLight, CastShadows, castShadows);
        }
        { // Ambient light
            ADD_GROUP_PROPERTY_TO_MAP(PROP_AMBIENT_LIGHT_INTENSITY, AmbientLight, ambientLight, Intensity, intensity);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_AMBIENT_LIGHT_URL, AmbientLight, ambientLight, URL, url);
        }
        { // Skybox
            ADD_GROUP_PROPERTY_TO_MAP(PROP_SKYBOX_COLOR, Skybox, skybox, Color, color);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_SKYBOX_URL, Skybox, skybox, URL, url);
        }
        { // Haze
            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_RANGE, Haze, haze, HazeRange, hazeRange);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_COLOR, Haze, haze, HazeColor, hazeColor);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_GLARE_COLOR, Haze, haze, HazeGlareColor, hazeGlareColor);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_ENABLE_GLARE, Haze, haze, HazeEnableGlare, hazeEnableGlare);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_GLARE_ANGLE, Haze, haze, HazeGlareAngle, hazeGlareAngle);

            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_ALTITUDE_EFFECT, Haze, haze, HazeAltitudeEffect, hazeAltitudeEfect);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_CEILING, Haze, haze, HazeCeiling, hazeCeiling);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_BASE_REF, Haze, haze, HazeBaseRef, hazeBaseRef);

            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_BACKGROUND_BLEND, Haze, haze, HazeBackgroundBlend, hazeBackgroundBlend);

            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_ATTENUATE_KEYLIGHT, Haze, haze, HazeAttenuateKeyLight, hazeAttenuateKeyLight);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_KEYLIGHT_RANGE, Haze, haze, HazeKeyLightRange, hazeKeyLightRange);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_HAZE_KEYLIGHT_ALTITUDE, Haze, haze, HazeKeyLightAltitude, hazeKeyLightAltitude);
        }
        { // Bloom
            ADD_GROUP_PROPERTY_TO_MAP(PROP_BLOOM_INTENSITY, Bloom, bloom, BloomIntensity, bloomIntensity);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_BLOOM_THRESHOLD, Bloom, bloom, BloomThreshold, bloomThreshold);
            ADD_GROUP_PROPERTY_TO_MAP(PROP_BLOOM_SIZE, Bloom, bloom, BloomSize, bloomSize);
        }
        ADD_PROPERTY_TO_MAP(PROP_FLYING_ALLOWED, FlyingAllowed, flyingAllowed, bool);
        ADD_PROPERTY_TO_MAP(PROP_GHOSTING_ALLOWED, GhostingAllowed, ghostingAllowed, bool);
        ADD_PROPERTY_TO_MAP(PROP_FILTER_URL, FilterURL, filterURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_KEY_LIGHT_MODE, KeyLightMode, keyLightMode, uint32_t);
        ADD_PROPERTY_TO_MAP(PROP_AMBIENT_LIGHT_MODE, AmbientLightMode, ambientLightMode, uint32_t);
        ADD_PROPERTY_TO_MAP(PROP_SKYBOX_MODE, SkyboxMode, skyboxMode, uint32_t);
        ADD_PROPERTY_TO_MAP(PROP_HAZE_MODE, HazeMode, hazeMode, uint32_t);
        ADD_PROPERTY_TO_MAP(PROP_BLOOM_MODE, BloomMode, bloomMode, uint32_t);

        // Polyvox
        ADD_PROPERTY_TO_MAP(PROP_VOXEL_VOLUME_SIZE, VoxelVolumeSize, voxelVolumeSize, vec3);
        ADD_PROPERTY_TO_MAP(PROP_VOXEL_DATA, VoxelData, voxelData, QByteArray);
        ADD_PROPERTY_TO_MAP(PROP_VOXEL_SURFACE_STYLE, VoxelSurfaceStyle, voxelSurfaceStyle, uint16_t);
        ADD_PROPERTY_TO_MAP(PROP_X_TEXTURE_URL, XTextureURL, xTextureURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_Y_TEXTURE_URL, YTextureURL, yTextureURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_Z_TEXTURE_URL, ZTextureURL, zTextureURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_X_N_NEIGHBOR_ID, XNNeighborID, xNNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_Y_N_NEIGHBOR_ID, YNNeighborID, yNNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_Z_N_NEIGHBOR_ID, ZNNeighborID, zNNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_X_P_NEIGHBOR_ID, XPNeighborID, xPNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_Y_P_NEIGHBOR_ID, YPNeighborID, yPNeighborID, EntityItemID);
        ADD_PROPERTY_TO_MAP(PROP_Z_P_NEIGHBOR_ID, ZPNeighborID, zPNeighborID, EntityItemID);

        // Web
        ADD_PROPERTY_TO_MAP(PROP_SOURCE_URL, SourceUrl, sourceUrl, QString);
        ADD_PROPERTY_TO_MAP(PROP_DPI, DPI, dpi, uint16_t);

        // Polyline
        ADD_PROPERTY_TO_MAP(PROP_LINE_POINTS, LinePoints, linePoints, QVector<vec3>);
        ADD_PROPERTY_TO_MAP(PROP_STROKE_WIDTHS, StrokeWidths, strokeWidths, QVector<float>);
        ADD_PROPERTY_TO_MAP(PROP_STROKE_NORMALS, Normals, normals, QVector<vec3>);
        ADD_PROPERTY_TO_MAP(PROP_STROKE_COLORS, StrokeColors, strokeColors, QVector<vec3>);
        ADD_PROPERTY_TO_MAP(PROP_IS_UV_MODE_STRETCH, IsUVModeStretch, isUVModeStretch, QVector<float>);
        ADD_PROPERTY_TO_MAP(PROP_LINE_GLOW, Glow, glow, bool);
        ADD_PROPERTY_TO_MAP(PROP_LINE_FACE_CAMERA, FaceCamera, faceCamera, bool);

        // Shape
        ADD_PROPERTY_TO_MAP(PROP_SHAPE, Shape, shape, QString);

        // Material
        ADD_PROPERTY_TO_MAP(PROP_MATERIAL_URL, MaterialURL, materialURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_MATERIAL_MAPPING_MODE, MaterialMappingMode, materialMappingMode, MaterialMappingMode);
        ADD_PROPERTY_TO_MAP(PROP_MATERIAL_PRIORITY, Priority, priority, quint16);
        ADD_PROPERTY_TO_MAP(PROP_PARENT_MATERIAL_NAME, ParentMaterialName, parentMaterialName, QString);
        ADD_PROPERTY_TO_MAP(PROP_MATERIAL_MAPPING_POS, MaterialMappingPos, materialMappingPos, vec2);
        ADD_PROPERTY_TO_MAP(PROP_MATERIAL_MAPPING_SCALE, MaterialMappingScale, materialMappingScale, vec2);
        ADD_PROPERTY_TO_MAP(PROP_MATERIAL_MAPPING_ROT, MaterialMappingRot, materialMappingRot, float);
        ADD_PROPERTY_TO_MAP(PROP_MATERIAL_DATA, MaterialData, materialData, QString);
        ADD_PROPERTY_TO_MAP(PROP_MATERIAL_REPEAT, MaterialRepeat, materialRepeat, bool);

        // Image
        ADD_PROPERTY_TO_MAP(PROP_IMAGE_URL, ImageURL, imageURL, QString);
        ADD_PROPERTY_TO_MAP(PROP_EMISSIVE, Emissive, emissive, bool);
        ADD_PROPERTY_TO_MAP(PROP_KEEP_ASPECT_RATIO, KeepAspectRatio, keepAspectRatio, bool);
        ADD_PROPERTY_TO_MAP(PROP_SUB_IMAGE, SubImage, subImage, QRect);

        // Grid
        ADD_PROPERTY_TO_MAP(PROP_GRID_FOLLOW_CAMERA, FollowCamera, followCamera, bool);
        ADD_PROPERTY_TO_MAP(PROP_MAJOR_GRID_EVERY, MajorGridEvery, majorGridEvery, uint32_t);
        ADD_PROPERTY_TO_MAP(PROP_MINOR_GRID_EVERY, MinorGridEvery, minorGridEvery, float);
    });

    auto iter = _propertyInfos.find(propertyName);
    if (iter != _propertyInfos.end()) {
        propertyInfo = *iter;
        return true;
    }

    return false;
}

QScriptValue EntityPropertyInfoToScriptValue(QScriptEngine* engine, const EntityPropertyInfo& propertyInfo) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("propertyEnum", propertyInfo.propertyEnum);
    obj.setProperty("minimum", propertyInfo.minimum.toString());
    obj.setProperty("maximum", propertyInfo.maximum.toString());
    return obj;
}

void EntityPropertyInfoFromScriptValue(const QScriptValue& object, EntityPropertyInfo& propertyInfo) {
    propertyInfo.propertyEnum = (EntityPropertyList)object.property("propertyEnum").toVariant().toUInt();
    propertyInfo.minimum = object.property("minimum").toVariant();
    propertyInfo.maximum = object.property("maximum").toVariant();
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
OctreeElement::AppendState EntityItemProperties::encodeEntityEditPacket(PacketType command, EntityItemID id, const EntityItemProperties& properties,
                QByteArray& buffer, EntityPropertyFlags requestedProperties, EntityPropertyFlags& didntFitProperties) {

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
    vec3 rootPosition(0);
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
        EntityPropertyFlags propertiesDidntFit = requestedProperties;

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

        bool headerFits = successIDFits && successTypeFits && successLastEditedFits &&
            successLastUpdatedFits && successPropertyFlagsFits;

        int startOfEntityItemData = packetData->getUncompressedByteOffset();

        if (headerFits) {
            bool successPropertyFits;
            propertyFlags -= PROP_LAST_ITEM; // clear the last item for now, we may or may not set it as the actual item

            // These items would go here once supported....
            //      PROP_PAGED_PROPERTY,
            //      PROP_CUSTOM_PROPERTIES_INCLUDED,


            APPEND_ENTITY_PROPERTY(PROP_SIMULATION_OWNER, properties._simulationOwner.toByteArray());
            APPEND_ENTITY_PROPERTY(PROP_VISIBLE, properties.getVisible());
            APPEND_ENTITY_PROPERTY(PROP_NAME, properties.getName());
            APPEND_ENTITY_PROPERTY(PROP_LOCKED, properties.getLocked());
            APPEND_ENTITY_PROPERTY(PROP_USER_DATA, properties.getUserData());
            APPEND_ENTITY_PROPERTY(PROP_HREF, properties.getHref());
            APPEND_ENTITY_PROPERTY(PROP_DESCRIPTION, properties.getDescription());
            APPEND_ENTITY_PROPERTY(PROP_POSITION, properties.getPosition());
            APPEND_ENTITY_PROPERTY(PROP_DIMENSIONS, properties.getDimensions());
            APPEND_ENTITY_PROPERTY(PROP_ROTATION, properties.getRotation());
            APPEND_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, properties.getRegistrationPoint());
            APPEND_ENTITY_PROPERTY(PROP_CREATED, properties.getCreated());
            APPEND_ENTITY_PROPERTY(PROP_LAST_EDITED_BY, properties.getLastEditedBy());
            // APPEND_ENTITY_PROPERTY(PROP_ENTITY_HOST_TYPE, (uint32_t)properties.getEntityHostType());              // not sent over the wire
            // APPEND_ENTITY_PROPERTY(PROP_OWNING_AVATAR_ID, properties.getOwningAvatarID());                        // not sent over the wire
            APPEND_ENTITY_PROPERTY(PROP_PARENT_ID, properties.getParentID());
            APPEND_ENTITY_PROPERTY(PROP_PARENT_JOINT_INDEX, properties.getParentJointIndex());
            APPEND_ENTITY_PROPERTY(PROP_QUERY_AA_CUBE, properties.getQueryAACube());
            APPEND_ENTITY_PROPERTY(PROP_CAN_CAST_SHADOW, properties.getCanCastShadow());
            // APPEND_ENTITY_PROPERTY(PROP_VISIBLE_IN_SECONDARY_CAMERA, properties.getIsVisibleInSecondaryCamera()); // not sent over the wire
            _staticGrab.setProperties(properties);
            _staticGrab.appendToEditPacket(packetData, requestedProperties, propertyFlags,
                                           propertiesDidntFit, propertyCount, appendState);

            // Physics
            APPEND_ENTITY_PROPERTY(PROP_DENSITY, properties.getDensity());
            APPEND_ENTITY_PROPERTY(PROP_VELOCITY, properties.getVelocity());
            APPEND_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, properties.getAngularVelocity());
            APPEND_ENTITY_PROPERTY(PROP_GRAVITY, properties.getGravity());
            APPEND_ENTITY_PROPERTY(PROP_ACCELERATION, properties.getAcceleration());
            APPEND_ENTITY_PROPERTY(PROP_DAMPING, properties.getDamping());
            APPEND_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, properties.getAngularDamping());
            APPEND_ENTITY_PROPERTY(PROP_RESTITUTION, properties.getRestitution());
            APPEND_ENTITY_PROPERTY(PROP_FRICTION, properties.getFriction());
            APPEND_ENTITY_PROPERTY(PROP_LIFETIME, properties.getLifetime());
            APPEND_ENTITY_PROPERTY(PROP_COLLISIONLESS, properties.getCollisionless());
            APPEND_ENTITY_PROPERTY(PROP_COLLISION_MASK, properties.getCollisionMask());
            APPEND_ENTITY_PROPERTY(PROP_DYNAMIC, properties.getDynamic());
            APPEND_ENTITY_PROPERTY(PROP_COLLISION_SOUND_URL, properties.getCollisionSoundURL());
            APPEND_ENTITY_PROPERTY(PROP_ACTION_DATA, properties.getActionData());

            // Cloning
            APPEND_ENTITY_PROPERTY(PROP_CLONEABLE, properties.getCloneable());
            APPEND_ENTITY_PROPERTY(PROP_CLONE_LIFETIME, properties.getCloneLifetime());
            APPEND_ENTITY_PROPERTY(PROP_CLONE_LIMIT, properties.getCloneLimit());
            APPEND_ENTITY_PROPERTY(PROP_CLONE_DYNAMIC, properties.getCloneDynamic());
            APPEND_ENTITY_PROPERTY(PROP_CLONE_AVATAR_ENTITY, properties.getCloneAvatarEntity());
            APPEND_ENTITY_PROPERTY(PROP_CLONE_ORIGIN_ID, properties.getCloneOriginID());

            // Scripts
            APPEND_ENTITY_PROPERTY(PROP_SCRIPT, properties.getScript());
            APPEND_ENTITY_PROPERTY(PROP_SCRIPT_TIMESTAMP, properties.getScriptTimestamp());
            APPEND_ENTITY_PROPERTY(PROP_SERVER_SCRIPTS, properties.getServerScripts());

            // Certifiable Properties
            APPEND_ENTITY_PROPERTY(PROP_ITEM_NAME, properties.getItemName());
            APPEND_ENTITY_PROPERTY(PROP_ITEM_DESCRIPTION, properties.getItemDescription());
            APPEND_ENTITY_PROPERTY(PROP_ITEM_CATEGORIES, properties.getItemCategories());
            APPEND_ENTITY_PROPERTY(PROP_ITEM_ARTIST, properties.getItemArtist());
            APPEND_ENTITY_PROPERTY(PROP_ITEM_LICENSE, properties.getItemLicense());
            APPEND_ENTITY_PROPERTY(PROP_LIMITED_RUN, properties.getLimitedRun());
            APPEND_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, properties.getMarketplaceID());
            APPEND_ENTITY_PROPERTY(PROP_EDITION_NUMBER, properties.getEditionNumber());
            APPEND_ENTITY_PROPERTY(PROP_ENTITY_INSTANCE_NUMBER, properties.getEntityInstanceNumber());
            APPEND_ENTITY_PROPERTY(PROP_CERTIFICATE_ID, properties.getCertificateID());
            APPEND_ENTITY_PROPERTY(PROP_STATIC_CERTIFICATE_VERSION, properties.getStaticCertificateVersion());

            if (properties.getType() == EntityTypes::ParticleEffect) {
                APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)(properties.getShapeType()));
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
                APPEND_ENTITY_PROPERTY(PROP_ALPHA, properties.getAlpha());
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

                APPEND_ENTITY_PROPERTY(PROP_PARTICLE_SPIN, properties.getParticleSpin());
                APPEND_ENTITY_PROPERTY(PROP_SPIN_SPREAD, properties.getSpinSpread());
                APPEND_ENTITY_PROPERTY(PROP_SPIN_START, properties.getSpinStart());
                APPEND_ENTITY_PROPERTY(PROP_SPIN_FINISH, properties.getSpinFinish());
                APPEND_ENTITY_PROPERTY(PROP_PARTICLE_ROTATE_WITH_ENTITY, properties.getRotateWithEntity())
            }

            if (properties.getType() == EntityTypes::Model) {
                APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)(properties.getShapeType()));
                APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, properties.getCompoundShapeURL());
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
                APPEND_ENTITY_PROPERTY(PROP_TEXTURES, properties.getTextures());

                APPEND_ENTITY_PROPERTY(PROP_MODEL_URL, properties.getModelURL());
                APPEND_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS_SET, properties.getJointRotationsSet());
                APPEND_ENTITY_PROPERTY(PROP_JOINT_ROTATIONS, properties.getJointRotations());
                APPEND_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS_SET, properties.getJointTranslationsSet());
                APPEND_ENTITY_PROPERTY(PROP_JOINT_TRANSLATIONS, properties.getJointTranslations());
                APPEND_ENTITY_PROPERTY(PROP_RELAY_PARENT_JOINTS, properties.getRelayParentJoints());

                _staticAnimation.setProperties(properties);
                _staticAnimation.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);
            }

            if (properties.getType() == EntityTypes::Light) {
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
                APPEND_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, properties.getIsSpotlight());
                APPEND_ENTITY_PROPERTY(PROP_INTENSITY, properties.getIntensity());
                APPEND_ENTITY_PROPERTY(PROP_EXPONENT, properties.getExponent());
                APPEND_ENTITY_PROPERTY(PROP_CUTOFF, properties.getCutoff());
                APPEND_ENTITY_PROPERTY(PROP_FALLOFF_RADIUS, properties.getFalloffRadius());
            }

            if (properties.getType() == EntityTypes::Text) {
                APPEND_ENTITY_PROPERTY(PROP_TEXT, properties.getText());
                APPEND_ENTITY_PROPERTY(PROP_LINE_HEIGHT, properties.getLineHeight());
                APPEND_ENTITY_PROPERTY(PROP_TEXT_COLOR, properties.getTextColor());
                APPEND_ENTITY_PROPERTY(PROP_TEXT_ALPHA, properties.getTextAlpha());
                APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, properties.getBackgroundColor());
                APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_ALPHA, properties.getBackgroundAlpha());
                APPEND_ENTITY_PROPERTY(PROP_BILLBOARD_MODE, (uint32_t)properties.getBillboardMode());
                APPEND_ENTITY_PROPERTY(PROP_LEFT_MARGIN, properties.getLeftMargin());
                APPEND_ENTITY_PROPERTY(PROP_RIGHT_MARGIN, properties.getRightMargin());
                APPEND_ENTITY_PROPERTY(PROP_TOP_MARGIN, properties.getTopMargin());
                APPEND_ENTITY_PROPERTY(PROP_BOTTOM_MARGIN, properties.getBottomMargin());
            }

            if (properties.getType() == EntityTypes::Zone) {
                APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)properties.getShapeType());
                APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, properties.getCompoundShapeURL());

                _staticKeyLight.setProperties(properties);
                _staticKeyLight.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);

                _staticAmbientLight.setProperties(properties);
                _staticAmbientLight.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);

                _staticSkybox.setProperties(properties);
                _staticSkybox.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);

                _staticHaze.setProperties(properties);
                _staticHaze.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);

                _staticBloom.setProperties(properties);
                _staticBloom.appendToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit, propertyCount, appendState);

                APPEND_ENTITY_PROPERTY(PROP_FLYING_ALLOWED, properties.getFlyingAllowed());
                APPEND_ENTITY_PROPERTY(PROP_GHOSTING_ALLOWED, properties.getGhostingAllowed());
                APPEND_ENTITY_PROPERTY(PROP_FILTER_URL, properties.getFilterURL());

                APPEND_ENTITY_PROPERTY(PROP_KEY_LIGHT_MODE, (uint32_t)properties.getKeyLightMode());
                APPEND_ENTITY_PROPERTY(PROP_AMBIENT_LIGHT_MODE, (uint32_t)properties.getAmbientLightMode());
                APPEND_ENTITY_PROPERTY(PROP_SKYBOX_MODE, (uint32_t)properties.getSkyboxMode());
                APPEND_ENTITY_PROPERTY(PROP_HAZE_MODE, (uint32_t)properties.getHazeMode());
                APPEND_ENTITY_PROPERTY(PROP_BLOOM_MODE, (uint32_t)properties.getBloomMode());
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

            if (properties.getType() == EntityTypes::Web) {
                APPEND_ENTITY_PROPERTY(PROP_SOURCE_URL, properties.getSourceUrl());
                APPEND_ENTITY_PROPERTY(PROP_DPI, properties.getDPI());
            }

            if (properties.getType() == EntityTypes::Line) {
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());

                APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, properties.getLinePoints());
            }

            if (properties.getType() == EntityTypes::PolyLine) {
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
                APPEND_ENTITY_PROPERTY(PROP_TEXTURES, properties.getTextures());

                APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, properties.getLinePoints());
                APPEND_ENTITY_PROPERTY(PROP_STROKE_WIDTHS, properties.getStrokeWidths());
                APPEND_ENTITY_PROPERTY(PROP_STROKE_NORMALS, properties.getPackedNormals());
                APPEND_ENTITY_PROPERTY(PROP_STROKE_COLORS, properties.getPackedStrokeColors());
                APPEND_ENTITY_PROPERTY(PROP_IS_UV_MODE_STRETCH, properties.getIsUVModeStretch());
                APPEND_ENTITY_PROPERTY(PROP_LINE_GLOW, properties.getGlow());
                APPEND_ENTITY_PROPERTY(PROP_LINE_FACE_CAMERA, properties.getFaceCamera());
            }

            // NOTE: Spheres and Boxes are just special cases of Shape, and they need to include their PROP_SHAPE
            // when encoding/decoding edits because otherwise they can't polymorph to other shape types
            if (properties.getType() == EntityTypes::Shape ||
                properties.getType() == EntityTypes::Box ||
                properties.getType() == EntityTypes::Sphere) {
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
                APPEND_ENTITY_PROPERTY(PROP_ALPHA, properties.getAlpha());
                APPEND_ENTITY_PROPERTY(PROP_SHAPE, properties.getShape());
            }

            // Materials
            if (properties.getType() == EntityTypes::Material) {
                APPEND_ENTITY_PROPERTY(PROP_MATERIAL_URL, properties.getMaterialURL());
                APPEND_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_MODE, (uint32_t)properties.getMaterialMappingMode());
                APPEND_ENTITY_PROPERTY(PROP_MATERIAL_PRIORITY, properties.getPriority());
                APPEND_ENTITY_PROPERTY(PROP_PARENT_MATERIAL_NAME, properties.getParentMaterialName());
                APPEND_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_POS, properties.getMaterialMappingPos());
                APPEND_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_SCALE, properties.getMaterialMappingScale());
                APPEND_ENTITY_PROPERTY(PROP_MATERIAL_MAPPING_ROT, properties.getMaterialMappingRot());
                APPEND_ENTITY_PROPERTY(PROP_MATERIAL_DATA, properties.getMaterialData());
                APPEND_ENTITY_PROPERTY(PROP_MATERIAL_REPEAT, properties.getMaterialRepeat());
            }

            // Image
            if (properties.getType() == EntityTypes::Image) {
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
                APPEND_ENTITY_PROPERTY(PROP_ALPHA, properties.getAlpha());

                APPEND_ENTITY_PROPERTY(PROP_IMAGE_URL, properties.getImageURL());
                APPEND_ENTITY_PROPERTY(PROP_EMISSIVE, properties.getEmissive());
                APPEND_ENTITY_PROPERTY(PROP_KEEP_ASPECT_RATIO, properties.getKeepAspectRatio());
                APPEND_ENTITY_PROPERTY(PROP_BILLBOARD_MODE, (uint32_t)properties.getBillboardMode());
                APPEND_ENTITY_PROPERTY(PROP_SUB_IMAGE, properties.getSubImage());
            }

            // Grid
            if (properties.getType() == EntityTypes::Grid) {
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
                APPEND_ENTITY_PROPERTY(PROP_ALPHA, properties.getAlpha());

                APPEND_ENTITY_PROPERTY(PROP_GRID_FOLLOW_CAMERA, properties.getFollowCamera());
                APPEND_ENTITY_PROPERTY(PROP_MAJOR_GRID_EVERY, properties.getMajorGridEvery());
                APPEND_ENTITY_PROPERTY(PROP_MINOR_GRID_EVERY, properties.getMinorGridEvery());
            }
        }

        if (propertyCount > 0) {
            int endOfEntityItemData = packetData->getUncompressedByteOffset();

            encodedPropertyFlags = propertyFlags;
            int newPropertyFlagsLength = encodedPropertyFlags.length();
            packetData->updatePriorBytes(propertyFlagsOffset, (const unsigned char*)encodedPropertyFlags.constData(),
                                         encodedPropertyFlags.length());

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
            didntFitProperties = propertiesDidntFit;
        }

        packetData->endSubTree();

        const char* finalizedData = reinterpret_cast<const char*>(packetData->getFinalizedData());
        int finalizedSize = packetData->getFinalizedSize();

        if (finalizedSize <= buffer.size()) {
            buffer.replace(0, finalizedSize, finalizedData, finalizedSize);
            buffer.resize(finalizedSize);
        } else {
            qCDebug(entities) << "ERROR - encoded edit message doesn't fit in output buffer.";
            appendState = OctreeElement::NONE; // if we got here, then we didn't include the item
            // maybe we should assert!!!
        }
    } else {
        packetData->discardSubTree();
    }

    return appendState;
}

QByteArray EntityItemProperties::getPackedNormals() const {
    return packNormals(getNormals());
}

QByteArray EntityItemProperties::packNormals(const QVector<vec3>& normals) const {
    int normalsSize = normals.size();
    QByteArray packedNormals = QByteArray(normalsSize * 6 + 1, '0');
    // add size of the array
    packedNormals[0] = ((uint8_t)normalsSize);

    int index = 1;
    for (int i = 0; i < normalsSize; i++) {
        int numBytes = packFloatVec3ToSignedTwoByteFixed((unsigned char*)packedNormals.data() + index, normals[i], 15);
        index += numBytes;
    }
    return packedNormals;
}

QByteArray EntityItemProperties::getPackedStrokeColors() const {
    return packStrokeColors(getStrokeColors());
}
QByteArray EntityItemProperties::packStrokeColors(const QVector<vec3>& strokeColors) const {
    int strokeColorsSize = strokeColors.size();
    QByteArray packedStrokeColors = QByteArray(strokeColorsSize * 3 + 1, '0');

    // add size of the array
    packedStrokeColors[0] = ((uint8_t)strokeColorsSize);


    for (int i = 0; i < strokeColorsSize; i++) {
        // add the color to the QByteArray
        packedStrokeColors[i * 3 + 1] = strokeColors[i].x * 255;
        packedStrokeColors[i * 3 + 2] = strokeColors[i].y * 255;
        packedStrokeColors[i * 3 + 3] = strokeColors[i].z * 255;
    }
    return packedStrokeColors;
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
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VISIBLE, bool, setVisible);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_NAME, QString, setName);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LOCKED, bool, setLocked);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_USER_DATA, QString, setUserData);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_HREF, QString, setHref);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DESCRIPTION, QString, setDescription);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_POSITION, vec3, setPosition);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DIMENSIONS, vec3, setDimensions);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ROTATION, quat, setRotation);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_REGISTRATION_POINT, vec3, setRegistrationPoint);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CREATED, quint64, setCreated);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LAST_EDITED_BY, QUuid, setLastEditedBy);
    // READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ENTITY_HOST_TYPE, entity::HostType, setEntityHostType);            // not sent over the wire
    // READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_OWNING_AVATAR_ID, QUuid, setOwningAvatarID);                       // not sent over the wire
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARENT_ID, QUuid, setParentID);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARENT_JOINT_INDEX, quint16, setParentJointIndex);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_QUERY_AA_CUBE, AACube, setQueryAACube);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CAN_CAST_SHADOW, bool, setCanCastShadow);
    // READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VISIBLE_IN_SECONDARY_CAMERA, bool, setIsVisibleInSecondaryCamera); // not sent over the wire
    properties.getGrab().decodeFromEditPacket(propertyFlags, dataAt, processedBytes);

    // Physics
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DENSITY, float, setDensity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VELOCITY, vec3, setVelocity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANGULAR_VELOCITY, vec3, setAngularVelocity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_GRAVITY, vec3, setGravity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ACCELERATION, vec3, setAcceleration);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DAMPING, float, setDamping);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANGULAR_DAMPING, float, setAngularDamping);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RESTITUTION, float, setRestitution);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_FRICTION, float, setFriction);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LIFETIME, float, setLifetime);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLLISIONLESS, bool, setCollisionless);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLLISION_MASK, uint16_t, setCollisionMask);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DYNAMIC, bool, setDynamic);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLLISION_SOUND_URL, QString, setCollisionSoundURL);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ACTION_DATA, QByteArray, setActionData);

    // Cloning
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CLONEABLE, bool, setCloneable);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CLONE_LIFETIME, float, setCloneLifetime);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CLONE_LIMIT, float, setCloneLimit);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CLONE_DYNAMIC, bool, setCloneDynamic);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CLONE_AVATAR_ENTITY, bool, setCloneAvatarEntity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CLONE_ORIGIN_ID, QUuid, setCloneOriginID);

    // Scripts
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SCRIPT, QString, setScript);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SCRIPT_TIMESTAMP, quint64, setScriptTimestamp);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SERVER_SCRIPTS, QString, setServerScripts);

    // Certifiable Properties
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ITEM_NAME, QString, setItemName);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ITEM_DESCRIPTION, QString, setItemDescription);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ITEM_CATEGORIES, QString, setItemCategories);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ITEM_ARTIST, QString, setItemArtist);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ITEM_LICENSE, QString, setItemLicense);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LIMITED_RUN, quint32, setLimitedRun);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MARKETPLACE_ID, QString, setMarketplaceID);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EDITION_NUMBER, quint32, setEditionNumber);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ENTITY_INSTANCE_NUMBER, quint32, setEntityInstanceNumber);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CERTIFICATE_ID, QString, setCertificateID);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_STATIC_CERTIFICATE_VERSION, quint32, setStaticCertificateVersion);

    if (properties.getType() == EntityTypes::ParticleEffect) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SHAPE_TYPE, ShapeType, setShapeType);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, u8vec3Color, setColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA, float, setAlpha);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXTURES, QString, setTextures);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MAX_PARTICLES, quint32, setMaxParticles);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LIFESPAN, float, setLifespan);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMITTING_PARTICLES, bool, setIsEmitting);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_RATE, float, setEmitRate);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_SPEED, float, setEmitSpeed);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SPEED_SPREAD, float, setSpeedSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_ORIENTATION, quat, setEmitOrientation);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_DIMENSIONS, vec3, setEmitDimensions);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_RADIUS_START, float, setEmitRadiusStart);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_POLAR_START, float, setPolarStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_POLAR_FINISH, float, setPolarFinish);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_AZIMUTH_START, float, setAzimuthStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_AZIMUTH_FINISH, float, setAzimuthFinish);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_ACCELERATION, vec3, setEmitAcceleration);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ACCELERATION_SPREAD, vec3, setAccelerationSpread);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARTICLE_RADIUS, float, setParticleRadius);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RADIUS_SPREAD, float, setRadiusSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RADIUS_START, float, setRadiusStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RADIUS_FINISH, float, setRadiusFinish);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR_SPREAD, u8vec3Color, setColorSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR_START, vec3Color, setColorStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR_FINISH, vec3Color, setColorFinish);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA_SPREAD, float, setAlphaSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA_START, float, setAlphaStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA_FINISH, float, setAlphaFinish);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMITTER_SHOULD_TRAIL, bool, setEmitterShouldTrail);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARTICLE_SPIN, float, setParticleSpin);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SPIN_SPREAD, float, setSpinSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SPIN_START, float, setSpinStart);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SPIN_FINISH, float, setSpinFinish);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARTICLE_ROTATE_WITH_ENTITY, bool, setRotateWithEntity);
    }

    if (properties.getType() == EntityTypes::Model) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SHAPE_TYPE, ShapeType, setShapeType);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, u8vec3Color, setColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXTURES, QString, setTextures);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MODEL_URL, QString, setModelURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_JOINT_ROTATIONS_SET, QVector<bool>, setJointRotationsSet);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_JOINT_ROTATIONS, QVector<quat>, setJointRotations);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_JOINT_TRANSLATIONS_SET, QVector<bool>, setJointTranslationsSet);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_JOINT_TRANSLATIONS, QVector<vec3>, setJointTranslations);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RELAY_PARENT_JOINTS, bool, setRelayParentJoints);

        properties.getAnimation().decodeFromEditPacket(propertyFlags, dataAt, processedBytes);
    }

    if (properties.getType() == EntityTypes::Light) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, u8vec3Color, setColor);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_IS_SPOTLIGHT, bool, setIsSpotlight);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_INTENSITY, float, setIntensity);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EXPONENT, float, setExponent);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CUTOFF, float, setCutoff);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_FALLOFF_RADIUS, float, setFalloffRadius);
    }

    if (properties.getType() == EntityTypes::Text) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXT, QString, setText);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_HEIGHT, float, setLineHeight);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXT_COLOR, u8vec3Color, setTextColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXT_ALPHA, float, setTextAlpha);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_BACKGROUND_COLOR, u8vec3Color, setBackgroundColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_BACKGROUND_ALPHA, float, setBackgroundAlpha);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_BILLBOARD_MODE, BillboardMode, setBillboardMode);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LEFT_MARGIN, float, setLeftMargin);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_RIGHT_MARGIN, float, setRightMargin);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TOP_MARGIN, float, setTopMargin);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_BOTTOM_MARGIN, float, setBottomMargin);
    }

    if (properties.getType() == EntityTypes::Zone) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SHAPE_TYPE, ShapeType, setShapeType);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);

        properties.getKeyLight().decodeFromEditPacket(propertyFlags, dataAt, processedBytes);
        properties.getAmbientLight().decodeFromEditPacket(propertyFlags, dataAt, processedBytes);
        properties.getSkybox().decodeFromEditPacket(propertyFlags, dataAt, processedBytes);
        properties.getHaze().decodeFromEditPacket(propertyFlags, dataAt, processedBytes);
        properties.getBloom().decodeFromEditPacket(propertyFlags, dataAt, processedBytes);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_FLYING_ALLOWED, bool, setFlyingAllowed);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_GHOSTING_ALLOWED, bool, setGhostingAllowed);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_FILTER_URL, QString, setFilterURL);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_KEY_LIGHT_MODE, uint32_t, setKeyLightMode);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_AMBIENT_LIGHT_MODE, uint32_t, setAmbientLightMode);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SKYBOX_MODE, uint32_t, setSkyboxMode);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_HAZE_MODE, uint32_t, setHazeMode);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_BLOOM_MODE, uint32_t, setBloomMode);
    }

    if (properties.getType() == EntityTypes::PolyVox) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VOXEL_VOLUME_SIZE, vec3, setVoxelVolumeSize);
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

    if (properties.getType() == EntityTypes::Web) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SOURCE_URL, QString, setSourceUrl);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DPI, uint16_t, setDPI);
    }

    if (properties.getType() == EntityTypes::Line) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, u8vec3Color, setColor);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_POINTS, QVector<vec3>, setLinePoints);
    }

    if (properties.getType() == EntityTypes::PolyLine) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, u8vec3Color, setColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXTURES, QString, setTextures);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_POINTS, QVector<vec3>, setLinePoints);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_STROKE_WIDTHS, QVector<float>, setStrokeWidths);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_STROKE_NORMALS, QByteArray, setPackedNormals);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_STROKE_COLORS, QByteArray, setPackedStrokeColors);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_IS_UV_MODE_STRETCH, bool, setIsUVModeStretch);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_GLOW, bool, setGlow);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_FACE_CAMERA, bool, setFaceCamera);
    }

    // NOTE: Spheres and Boxes are just special cases of Shape, and they need to include their PROP_SHAPE
    // when encoding/decoding edits because otherwise they can't polymorph to other shape types
    if (properties.getType() == EntityTypes::Shape ||
        properties.getType() == EntityTypes::Box ||
        properties.getType() == EntityTypes::Sphere) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, u8vec3Color, setColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA, float, setAlpha);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SHAPE, QString, setShape);
    }

    // Materials
    if (properties.getType() == EntityTypes::Material) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MATERIAL_URL, QString, setMaterialURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MATERIAL_MAPPING_MODE, MaterialMappingMode, setMaterialMappingMode);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MATERIAL_PRIORITY, quint16, setPriority);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARENT_MATERIAL_NAME, QString, setParentMaterialName);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MATERIAL_MAPPING_POS, vec2, setMaterialMappingPos);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MATERIAL_MAPPING_SCALE, vec2, setMaterialMappingScale);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MATERIAL_MAPPING_ROT, float, setMaterialMappingRot);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MATERIAL_DATA, QString, setMaterialData);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MATERIAL_REPEAT, bool, setMaterialRepeat);
    }

    // Image
    if (properties.getType() == EntityTypes::Image) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, u8vec3Color, setColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA, float, setAlpha);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_IMAGE_URL, QString, setImageURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMISSIVE, bool, setEmissive);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_KEEP_ASPECT_RATIO, bool, setKeepAspectRatio);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_BILLBOARD_MODE, BillboardMode, setBillboardMode);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SUB_IMAGE, QRect, setSubImage);
    }

    // Grid
    if (properties.getType() == EntityTypes::Grid) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, u8vec3Color, setColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ALPHA, float, setAlpha);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_GRID_FOLLOW_CAMERA, bool, setFollowCamera);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MAJOR_GRID_EVERY, uint32_t, setMajorGridEvery);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MINOR_GRID_EVERY, float, setMinorGridEvery);
    }

    return valid;
}

void EntityItemProperties::setPackedNormals(const QByteArray& value) {
    setNormals(unpackNormals(value));
}

QVector<vec3> EntityItemProperties::unpackNormals(const QByteArray& normals) {
    // the size of the vector is packed first
    QVector<vec3> unpackedNormals = QVector<vec3>((int)normals[0]);

    if ((int)normals[0] == normals.size() / 6) {
        int j = 0;
        for (int i = 1; i < normals.size();) {
            vec3 aux = vec3();
            i += unpackFloatVec3FromSignedTwoByteFixed((unsigned char*)normals.data() + i, aux, 15);
            unpackedNormals[j] = aux;
            j++;
        }
    } else {
        qCDebug(entities) << "WARNING - Expected received size for normals does not match. Expected: " << (int)normals[0]
                          << " Received: " << (normals.size() / 6);
    }
    return unpackedNormals;
}

void EntityItemProperties::setPackedStrokeColors(const QByteArray& value) {
    setStrokeColors(unpackStrokeColors(value));
}

QVector<vec3> EntityItemProperties::unpackStrokeColors(const QByteArray& strokeColors) {
    // the size of the vector is packed first
    QVector<vec3> unpackedStrokeColors = QVector<vec3>((int)strokeColors[0]);

    if ((int)strokeColors[0] == strokeColors.size() / 3) {
        int j = 0;
        for (int i = 1; i < strokeColors.size();) {

            float r = (uint8_t)strokeColors[i++] / 255.0f;
            float g = (uint8_t)strokeColors[i++] / 255.0f;
            float b = (uint8_t)strokeColors[i++] / 255.0f;
            unpackedStrokeColors[j++] = vec3(r, g, b);
        }
    } else {
        qCDebug(entities) << "WARNING - Expected received size for stroke colors does not match. Expected: "
            << (int)strokeColors[0] << " Received: " << (strokeColors.size() / 3);
    }

    return unpackedStrokeColors;
}

// NOTE: This version will only encode the portion of the edit message immediately following the
// header it does not include the send times and sequence number because that is handled by the
// edit packet sender...
bool EntityItemProperties::encodeEraseEntityMessage(const EntityItemID& entityItemID, QByteArray& buffer) {

    char* copyAt = buffer.data();
    uint16_t numberOfIds = 1; // only one entity ID in this message

    int outputLength = 0;

    if (buffer.size() < (int)(sizeof(numberOfIds) + NUM_BYTES_RFC4122_UUID)) {
        qCDebug(entities) << "ERROR - encodeEraseEntityMessage() called with buffer that is too small!";
        return false;
    }

    memcpy(copyAt, &numberOfIds, sizeof(numberOfIds));
    copyAt += sizeof(numberOfIds);
    outputLength = sizeof(numberOfIds);

    memcpy(copyAt, entityItemID.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
    outputLength += NUM_BYTES_RFC4122_UUID;

    buffer.resize(outputLength);

    return true;
}

bool EntityItemProperties::encodeCloneEntityMessage(const EntityItemID& entityIDToClone, const EntityItemID& newEntityID, QByteArray& buffer) {
    char* copyAt = buffer.data();
    int outputLength = 0;

    if (buffer.size() < (int)(NUM_BYTES_RFC4122_UUID * 2)) {
        qCDebug(entities) << "ERROR - encodeCloneEntityMessage() called with buffer that is too small!";
        return false;
    }

    memcpy(copyAt, entityIDToClone.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
    copyAt += NUM_BYTES_RFC4122_UUID;
    outputLength += NUM_BYTES_RFC4122_UUID;

    memcpy(copyAt, newEntityID.toRfc4122().constData(), NUM_BYTES_RFC4122_UUID);
    outputLength += NUM_BYTES_RFC4122_UUID;

    buffer.resize(outputLength);

    return true;
}

bool EntityItemProperties::decodeCloneEntityMessage(const QByteArray& buffer, int& processedBytes, EntityItemID& entityIDToClone, EntityItemID& newEntityID) {
    const unsigned char* packetData = (const unsigned char*)buffer.constData();
    const unsigned char* dataAt = packetData;
    size_t packetLength = buffer.size();
    processedBytes = 0;

    if (NUM_BYTES_RFC4122_UUID * 2 > packetLength) {
        qCDebug(entities) << "EntityItemProperties::processEraseMessageDetails().... bailing because not enough bytes in buffer";
        return false; // bail to prevent buffer overflow
    }

    QByteArray encodedID = buffer.mid((int)processedBytes, NUM_BYTES_RFC4122_UUID);
    entityIDToClone = QUuid::fromRfc4122(encodedID);
    dataAt += encodedID.size();
    processedBytes += encodedID.size();

    encodedID = buffer.mid((int)processedBytes, NUM_BYTES_RFC4122_UUID);
    newEntityID = QUuid::fromRfc4122(encodedID);
    dataAt += encodedID.size();
    processedBytes += encodedID.size();

    return true;
}

void EntityItemProperties::markAllChanged() {
    // Core
    _simulationOwnerChanged = true;
    _visibleChanged = true;
    _nameChanged = true;
    _lockedChanged = true;
    _userDataChanged = true;
    _hrefChanged = true;
    _descriptionChanged = true;
    _positionChanged = true;
    _dimensionsChanged = true;
    _rotationChanged = true;
    _registrationPointChanged = true;
    _createdChanged = true;
    _lastEditedByChanged = true;
    _entityHostTypeChanged = true;
    _owningAvatarIDChanged = true;
    _parentIDChanged = true;
    _parentJointIndexChanged = true;
    _queryAACubeChanged = true;
    _canCastShadowChanged = true;
    _isVisibleInSecondaryCameraChanged = true;
    _grab.markAllChanged();

    // Physics
    _densityChanged = true;
    _velocityChanged = true;
    _angularVelocityChanged = true;
    _gravityChanged = true;
    _accelerationChanged = true;
    _dampingChanged = true;
    _angularDampingChanged = true;
    _restitutionChanged = true;
    _frictionChanged = true;
    _lifetimeChanged = true;
    _collisionlessChanged = true;
    _collisionMaskChanged = true;
    _dynamicChanged = true;
    _collisionSoundURLChanged = true;
    _actionDataChanged = true;

    // Cloning
    _cloneableChanged = true;
    _cloneLifetimeChanged = true;
    _cloneLimitChanged = true;
    _cloneDynamicChanged = true;
    _cloneAvatarEntityChanged = true;
    _cloneOriginIDChanged = true;

    // Scripts
    _scriptChanged = true;
    _scriptTimestampChanged = true;
    _serverScriptsChanged = true;

    // Certifiable Properties
    _itemNameChanged = true;
    _itemDescriptionChanged = true;
    _itemCategoriesChanged = true;
    _itemArtistChanged = true;
    _itemLicenseChanged = true;
    _limitedRunChanged = true;
    _marketplaceIDChanged = true;
    _editionNumberChanged = true;
    _entityInstanceNumberChanged = true;
    _certificateIDChanged = true;
    _staticCertificateVersionChanged = true;

    // Common
    _shapeTypeChanged = true;
    _colorChanged = true;
    _alphaChanged = true;
    _texturesChanged = true;
    _compoundShapeURLChanged = true;

    // Particles
    _maxParticlesChanged = true;
    _lifespanChanged = true;
    _isEmittingChanged = true;
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
    _radiusStartChanged = true;
    _radiusFinishChanged = true;
    _colorSpreadChanged = true;
    _colorStartChanged = true;
    _colorFinishChanged = true;
    _alphaSpreadChanged = true;
    _alphaStartChanged = true;
    _alphaFinishChanged = true;
    _emitterShouldTrailChanged = true;
    _particleSpinChanged = true;
    _spinStartChanged = true;
    _spinFinishChanged = true;
    _spinSpreadChanged = true;
    _rotateWithEntityChanged = true;

    // Model
    _modelURLChanged = true;
    _jointRotationsSetChanged = true;
    _jointRotationsChanged = true;
    _jointTranslationsSetChanged = true;
    _jointTranslationsChanged = true;
    _relayParentJointsChanged = true;
    _animation.markAllChanged();

    // Light
    _isSpotlightChanged = true;
    _intensityChanged = true;
    _exponentChanged = true;
    _cutoffChanged = true;
    _falloffRadiusChanged = true;

    // Text
    _textChanged = true;
    _lineHeightChanged = true;
    _textColorChanged = true;
    _textAlphaChanged = true;
    _backgroundColorChanged = true;
    _backgroundAlphaChanged = true;
    _billboardModeChanged = true;
    _leftMarginChanged = true;
    _rightMarginChanged = true;
    _topMarginChanged = true;
    _bottomMarginChanged = true;

    // Zone
    _keyLight.markAllChanged();
    _ambientLight.markAllChanged();
    _skybox.markAllChanged();
    _haze.markAllChanged();
    _bloom.markAllChanged();
    _flyingAllowedChanged = true;
    _ghostingAllowedChanged = true;
    _filterURLChanged = true;
    _keyLightModeChanged = true;
    _ambientLightModeChanged = true;
    _skyboxModeChanged = true;
    _hazeModeChanged = true;
    _bloomModeChanged = true;

    // Polyvox
    _voxelVolumeSizeChanged = true;
    _voxelDataChanged = true;
    _voxelSurfaceStyleChanged = true;
    _xTextureURLChanged = true;
    _yTextureURLChanged = true;
    _zTextureURLChanged = true;
    _xNNeighborIDChanged = true;
    _yNNeighborIDChanged = true;
    _zNNeighborIDChanged = true;
    _xPNeighborIDChanged = true;
    _yPNeighborIDChanged = true;
    _zPNeighborIDChanged = true;

    // Web
    _sourceUrlChanged = true;
    _dpiChanged = true;

    // Polyline
    _linePointsChanged = true;
    _strokeWidthsChanged = true;
    _normalsChanged = true;
    _strokeColorsChanged = true;
    _isUVModeStretchChanged = true;
    _glowChanged = true;
    _faceCameraChanged = true;

    // Shape
    _shapeChanged = true;

    // Material
    _materialURLChanged = true;
    _materialMappingModeChanged = true;
    _priorityChanged = true;
    _parentMaterialNameChanged = true;
    _materialMappingPosChanged = true;
    _materialMappingScaleChanged = true;
    _materialMappingRotChanged = true;
    _materialDataChanged = true;
    _materialRepeatChanged = true;

    // Image
    _imageURLChanged = true;
    _emissiveChanged = true;
    _keepAspectRatioChanged = true;
    _subImageChanged = true;

    // Grid
    _followCameraChanged = true;
    _majorGridEveryChanged = true;
    _minorGridEveryChanged = true;
}

// The minimum bounding box for the entity.
AABox EntityItemProperties::getAABox() const {

    // _position represents the position of the registration point.
    vec3 registrationRemainder = vec3(1.0f) - _registrationPoint;

    vec3 unrotatedMinRelativeToEntity = -(_dimensions * _registrationPoint);
    vec3 unrotatedMaxRelativeToEntity = _dimensions * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(_rotation);

    // shift the extents to be relative to the position/registration point
    rotatedExtentsRelativeToRegistrationPoint.shiftBy(_position);

    return AABox(rotatedExtentsRelativeToRegistrationPoint);
}

bool EntityItemProperties::hasTransformOrVelocityChanges() const {
    return _positionChanged || _localPositionChanged
        || _rotationChanged || _localRotationChanged
        || _velocityChanged || _localVelocityChanged
        || _angularVelocityChanged || _localAngularVelocityChanged
        || _accelerationChanged;
}

void EntityItemProperties::clearTransformOrVelocityChanges() {
    _positionChanged = false;
    _localPositionChanged = false;
    _rotationChanged = false;
    _localRotationChanged = false;
    _velocityChanged = false;
    _localVelocityChanged = false;
    _angularVelocityChanged = false;
    _localAngularVelocityChanged = false;
    _accelerationChanged = false;
}

bool EntityItemProperties::hasMiscPhysicsChanges() const {
    return _gravityChanged || _dimensionsChanged || _densityChanged || _frictionChanged
        || _restitutionChanged || _dampingChanged || _angularDampingChanged || _registrationPointChanged ||
        _compoundShapeURLChanged || _dynamicChanged || _collisionlessChanged || _collisionMaskChanged;
}

bool EntityItemProperties::hasSimulationRestrictedChanges() const {
    return _positionChanged || _localPositionChanged
        || _rotationChanged || _localRotationChanged
        || _velocityChanged || _localVelocityChanged
        || _localDimensionsChanged || _dimensionsChanged
        || _angularVelocityChanged || _localAngularVelocityChanged
        || _accelerationChanged
        || _parentIDChanged || _parentJointIndexChanged;
}

void EntityItemProperties::copySimulationRestrictedProperties(const EntityItemPointer& entity) {
    if (!_parentIDChanged) {
        setParentID(entity->getParentID());
    }
    if (!_parentJointIndexChanged) {
        setParentJointIndex(entity->getParentJointIndex());
    }
    if (!_localPositionChanged && !_positionChanged) {
        setPosition(entity->getWorldPosition());
    }
    if (!_localRotationChanged && !_rotationChanged) {
        setRotation(entity->getWorldOrientation());
    }
    if (!_localVelocityChanged && !_velocityChanged) {
        setVelocity(entity->getWorldVelocity());
    }
    if (!_localAngularVelocityChanged && !_angularVelocityChanged) {
        setAngularVelocity(entity->getWorldAngularVelocity());
    }
    if (!_accelerationChanged) {
        setAcceleration(entity->getAcceleration());
    }
    if (!_localDimensionsChanged && !_dimensionsChanged) {
        setDimensions(entity->getScaledDimensions());
    }
}

void EntityItemProperties::clearSimulationRestrictedProperties() {
    _positionChanged = false;
    _localPositionChanged = false;
    _rotationChanged = false;
    _localRotationChanged = false;
    _velocityChanged = false;
    _localVelocityChanged = false;
    _angularVelocityChanged = false;
    _localAngularVelocityChanged = false;
    _accelerationChanged = false;
    _parentIDChanged = false;
    _parentJointIndexChanged = false;
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

uint8_t EntityItemProperties::computeSimulationBidPriority() const {
    uint8_t priority = 0;
    if (_parentIDChanged || _parentJointIndexChanged) {
        // we need higher simulation ownership priority to chang parenting info
        priority = SCRIPT_GRAB_SIMULATION_PRIORITY;
    } else if (_positionChanged || _localPositionChanged
            || _rotationChanged || _localRotationChanged
            || _velocityChanged || _localVelocityChanged
            || _angularVelocityChanged || _localAngularVelocityChanged) {
        priority = SCRIPT_POKE_SIMULATION_PRIORITY;
    }
    return priority;
}

QList<QString> EntityItemProperties::listChangedProperties() {
    QList<QString> out;

    // Core
    if (simulationOwnerChanged()) {
        out += "simulationOwner";
    }
    if (visibleChanged()) {
        out += "visible";
    }
    if (nameChanged()) {
        out += "name";
    }
    if (lockedChanged()) {
        out += "locked";
    }
    if (userDataChanged()) {
        out += "userData";
    }
    if (hrefChanged()) {
        out += "href";
    }
    if (descriptionChanged()) {
        out += "description";
    }
    if (containsPositionChange()) {
        out += "position";
    }
    if (dimensionsChanged()) {
        out += "dimensions";
    }
    if (rotationChanged()) {
        out += "rotation";
    }
    if (registrationPointChanged()) {
        out += "registrationPoint";
    }
    if (createdChanged()) {
        out += "created";
    }
    if (lastEditedByChanged()) {
        out += "lastEditedBy";
    }
    if (entityHostTypeChanged()) {
        out += "entityHostType";
    }
    if (owningAvatarIDChanged()) {
        out += "owningAvatarID";
    }
    if (parentIDChanged()) {
        out += "parentID";
    }
    if (parentJointIndexChanged()) {
        out += "parentJointIndex";
    }
    if (queryAACubeChanged()) {
        out += "queryAACube";
    }
    if (canCastShadowChanged()) {
        out += "canCastShadow";
    }
    if (isVisibleInSecondaryCameraChanged()) {
        out += "isVisibleInSecondaryCamera";
    }
    getGrab().listChangedProperties(out);

    // Physics
    if (densityChanged()) {
        out += "density";
    }
    if (velocityChanged()) {
        out += "velocity";
    }
    if (angularVelocityChanged()) {
        out += "angularVelocity";
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
    if (angularDampingChanged()) {
        out += "angularDamping";
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
    if (collisionlessChanged()) {
        out += "collisionless";
    }
    if (collisionMaskChanged()) {
        out += "collisionMask";
    }
    if (dynamicChanged()) {
        out += "dynamic";
    }
    if (collisionSoundURLChanged()) {
        out += "collisionSoundURL";
    }
    if (actionDataChanged()) {
        out += "actionData";
    }

    // Cloning
    if (cloneableChanged()) {
        out += "cloneable";
    }
    if (cloneLifetimeChanged()) {
        out += "cloneLifetime";
    }
    if (cloneLimitChanged()) {
        out += "cloneLimit";
    }
    if (cloneDynamicChanged()) {
        out += "cloneDynamic";
    }
    if (cloneAvatarEntityChanged()) {
        out += "cloneAvatarEntity";
    }
    if (cloneOriginIDChanged()) {
        out += "cloneOriginID";
    }

    // Scripts
    if (scriptChanged()) {
        out += "script";
    }
    if (scriptTimestampChanged()) {
        out += "scriptTimestamp";
    }
    if (serverScriptsChanged()) {
        out += "serverScripts";
    }

    // Certifiable Properties
    if (itemNameChanged()) {
        out += "itemName";
    }
    if (itemDescriptionChanged()) {
        out += "itemDescription";
    }
    if (itemCategoriesChanged()) {
        out += "itemCategories";
    }
    if (itemArtistChanged()) {
        out += "itemArtist";
    }
    if (itemLicenseChanged()) {
        out += "itemLicense";
    }
    if (limitedRunChanged()) {
        out += "limitedRun";
    }
    if (marketplaceIDChanged()) {
        out += "marketplaceID";
    }
    if (editionNumberChanged()) {
        out += "editionNumber";
    }
    if (entityInstanceNumberChanged()) {
        out += "entityInstanceNumber";
    }
    if (certificateIDChanged()) {
        out += "certificateID";
    }
    if (staticCertificateVersionChanged()) {
        out += "staticCertificateVersion";
    }

    // Common
    if (shapeTypeChanged()) {
        out += "shapeType";
    }
    if (compoundShapeURLChanged()) {
        out += "compoundShapeURL";
    }
    if (colorChanged()) {
        out += "color";
    }
    if (alphaChanged()) {
        out += "alpha";
    }
    if (texturesChanged()) {
        out += "textures";
    }

    // Particles
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
    if (colorSpreadChanged()) {
        out += "colorSpread";
    }
    if (colorStartChanged()) {
        out += "colorStart";
    }
    if (colorFinishChanged()) {
        out += "colorFinish";
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
    if (particleSpinChanged()) {
        out += "particleSpin";
    }
    if (spinSpreadChanged()) {
        out += "spinSpread";
    }
    if (spinStartChanged()) {
        out += "spinStart";
    }
    if (spinFinishChanged()) {
        out += "spinFinish";
    }
    if (rotateWithEntityChanged()) {
        out += "rotateWithEntity";
    }

    // Model
    if (modelURLChanged()) {
        out += "modelURL";
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
    if (relayParentJointsChanged()) {
        out += "relayParentJoints";
    }
    getAnimation().listChangedProperties(out);

    // Light
    if (isSpotlightChanged()) {
        out += "isSpotlight";
    }
    if (intensityChanged()) {
        out += "intensity";
    }
    if (exponentChanged()) {
        out += "exponent";
    }
    if (cutoffChanged()) {
        out += "cutoff";
    }
    if (falloffRadiusChanged()) {
        out += "falloffRadius";
    }

    // Text
    if (textChanged()) {
        out += "text";
    }
    if (lineHeightChanged()) {
        out += "lineHeight";
    }
    if (textColorChanged()) {
        out += "textColor";
    }
    if (textAlphaChanged()) {
        out += "textAlpha";
    }
    if (backgroundColorChanged()) {
        out += "backgroundColor";
    }
    if (backgroundAlphaChanged()) {
        out += "backgroundAlpha";
    }
    if (billboardModeChanged()) {
        out += "billboardMode";
    }
    if (leftMarginChanged()) {
        out += "leftMargin";
    }
    if (rightMarginChanged()) {
        out += "rightMargin";
    }
    if (topMarginChanged()) {
        out += "topMargin";
    }
    if (bottomMarginChanged()) {
        out += "bottomMargin";
    }

    // Zone
    getKeyLight().listChangedProperties(out);
    getAmbientLight().listChangedProperties(out);
    getSkybox().listChangedProperties(out);
    getHaze().listChangedProperties(out);
    getBloom().listChangedProperties(out);
    if (flyingAllowedChanged()) {
        out += "flyingAllowed";
    }
    if (ghostingAllowedChanged()) {
        out += "ghostingAllowed";
    }
    if (filterURLChanged()) {
        out += "filterURL";
    }
    if (keyLightModeChanged()) {
        out += "keyLightMode";
    }
    if (ambientLightModeChanged()) {
        out += "ambientLightMode";
    }
    if (skyboxModeChanged()) {
        out += "skyboxMode";
    }
    if (hazeModeChanged()) {
        out += "hazeMode";
    }
    if (bloomModeChanged()) {
        out += "bloomMode";
    }

    // Polyvox
    if (voxelVolumeSizeChanged()) {
        out += "voxelVolumeSize";
    }
    if (voxelDataChanged()) {
        out += "voxelData";
    }
    if (voxelSurfaceStyleChanged()) {
        out += "voxelSurfaceStyle";
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

    // Web
    if (sourceUrlChanged()) {
        out += "sourceUrl";
    }
    if (dpiChanged()) {
        out += "dpi";
    }

    // Polyline
    if (linePointsChanged()) {
        out += "linePoints";
    }
    if (strokeWidthsChanged()) {
        out += "strokeWidths";
    }
    if (normalsChanged()) {
        out += "normals";
    }
    if (strokeColorsChanged()) {
        out += "strokeColors";
    }
    if (isUVModeStretchChanged()) {
        out += "isUVModeStretch";
    }
    if (glowChanged()) {
        out += "glow";
    }
    if (faceCameraChanged()) {
        out += "faceCamera";
    }

    // Shape
    if (shapeChanged()) {
        out += "shape";
    }

    // Material
    if (materialURLChanged()) {
        out += "materialURL";
    }
    if (materialMappingModeChanged()) {
        out += "materialMappingMode";
    }
    if (priorityChanged()) {
        out += "priority";
    }
    if (parentMaterialNameChanged()) {
        out += "parentMaterialName";
    }
    if (materialMappingPosChanged()) {
        out += "materialMappingPos";
    }
    if (materialMappingScaleChanged()) {
        out += "materialMappingScale";
    }
    if (materialMappingRotChanged()) {
        out += "materialMappingRot";
    }
    if (materialDataChanged()) {
        out += "materialData";
    }
    if (materialRepeatChanged()) {
        out += "materialRepeat";
    }

    // Image
    if (imageURLChanged()) {
        out += "imageURL";
    }
    if (emissiveChanged()) {
        out += "emissive";
    }
    if (keepAspectRatioChanged()) {
        out += "keepAspectRatio";
    }
    if (subImageChanged()) {
        out += "subImage";
    }

    // Grid
    if (followCameraChanged()) {
        out += "followCamera";
    }
    if (majorGridEveryChanged()) {
        out += "majorGridEvery";
    }
    if (minorGridEveryChanged()) {
        out += "minorGridEvery";
    }

    return out;
}

bool EntityItemProperties::transformChanged() const {
    return positionChanged() || rotationChanged() ||
        localPositionChanged() || localRotationChanged();
}

bool EntityItemProperties::getScalesWithParent() const {
    // keep this logic the same as in EntityItem::getScalesWithParent
    bool scalesWithParent { false };
    if (parentIDChanged()) {
        bool success;
        SpatiallyNestablePointer parent = SpatiallyNestable::findByID(getParentID(), success);
        if (success && parent) {
            bool avatarAncestor = (parent->getNestableType() == NestableType::Avatar ||
                                   parent->hasAncestorOfType(NestableType::Avatar));
            scalesWithParent = getEntityHostType() == entity::HostType::AVATAR && avatarAncestor;
        }
    }
    return scalesWithParent;
}

bool EntityItemProperties::parentRelatedPropertyChanged() const {
    return positionChanged() || rotationChanged() ||
        localPositionChanged() || localRotationChanged() ||
        localDimensionsChanged() ||
        parentIDChanged() || parentJointIndexChanged();
}

bool EntityItemProperties::queryAACubeRelatedPropertyChanged() const {
    return parentRelatedPropertyChanged() || dimensionsChanged();
}

bool EntityItemProperties::grabbingRelatedPropertyChanged() const {
    const GrabPropertyGroup& grabProperties = getGrab();
    return grabProperties.triggerableChanged() || grabProperties.grabbableChanged() ||
        grabProperties.grabFollowsControllerChanged() || grabProperties.grabKinematicChanged() ||
        grabProperties.equippableChanged() || grabProperties.equippableLeftPositionChanged() ||
        grabProperties.equippableRightPositionChanged() || grabProperties.equippableLeftRotationChanged() ||
        grabProperties.equippableRightRotationChanged() || grabProperties.equippableIndicatorURLChanged() ||
        grabProperties.equippableIndicatorScaleChanged() || grabProperties.equippableIndicatorOffsetChanged();
}

// Checking Certifiable Properties
#define ADD_STRING_PROPERTY(n, N) if (!get##N().isEmpty()) json[#n] = get##N()
#define ADD_ENUM_PROPERTY(n, N) json[#n] = get##N##AsString()
#define ADD_INT_PROPERTY(n, N) if (get##N() != 0) json[#n] = (get##N() == (quint32) -1) ? -1.0 : ((double) get##N())
QByteArray EntityItemProperties::getStaticCertificateJSON() const {
    // Produce a compact json of every non-default static certificate property, with the property names in alphabetical order.
    // The static certificate properties include all an only those properties that cannot be changed without altering the identity
    // of the entity as reviewed during the certification submission.

    QJsonObject json;

    quint32 staticCertificateVersion = getStaticCertificateVersion();

    if (!getAnimation().getURL().isEmpty()) {
        json["animationURL"] = getAnimation().getURL();
    }
    ADD_STRING_PROPERTY(collisionSoundURL, CollisionSoundURL);
    ADD_STRING_PROPERTY(compoundShapeURL, CompoundShapeURL);
    ADD_INT_PROPERTY(editionNumber, EditionNumber);
    ADD_INT_PROPERTY(entityInstanceNumber, EntityInstanceNumber);
    ADD_STRING_PROPERTY(itemArtist, ItemArtist);
    ADD_STRING_PROPERTY(itemCategories, ItemCategories);
    ADD_STRING_PROPERTY(itemDescription, ItemDescription);
    ADD_STRING_PROPERTY(itemLicenseUrl, ItemLicense);
    ADD_STRING_PROPERTY(itemName, ItemName);
    ADD_INT_PROPERTY(limitedRun, LimitedRun);
    ADD_STRING_PROPERTY(marketplaceID, MarketplaceID);
    ADD_STRING_PROPERTY(modelURL, ModelURL);
    ADD_STRING_PROPERTY(script, Script);
    if (staticCertificateVersion >= 1) {
        ADD_STRING_PROPERTY(serverScripts, ServerScripts);
    }
    ADD_ENUM_PROPERTY(shapeType, ShapeType);
    ADD_INT_PROPERTY(staticCertificateVersion, StaticCertificateVersion);
    json["type"] = EntityTypes::getEntityTypeName(getType());

    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}
QByteArray EntityItemProperties::getStaticCertificateHash() const {
    return QCryptographicHash::hash(getStaticCertificateJSON(), QCryptographicHash::Sha256);
}

// FIXME: This is largely copied from EntityItemProperties::verifyStaticCertificateProperties, which should be refactored to use this.
// I also don't like the nested-if style, but for this step I'm deliberately preserving the similarity.
bool EntityItemProperties::verifySignature(const QString& publicKey, const QByteArray& digestByteArray, const QByteArray& signatureByteArray) {

    if (digestByteArray.isEmpty()) {
        return false;
    }

    auto keyByteArray = publicKey.toUtf8();
    auto key = keyByteArray.constData();
    int keyLength = publicKey.length();

    BIO *bio = BIO_new_mem_buf((void*)key, keyLength);
    EVP_PKEY* evp_key = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    if (evp_key) {
        EC_KEY* ec = EVP_PKEY_get1_EC_KEY(evp_key);
        if (ec) {
            const unsigned char* digest = reinterpret_cast<const unsigned char*>(digestByteArray.constData());
            int digestLength = digestByteArray.length();

            const unsigned char* signature = reinterpret_cast<const unsigned char*>(signatureByteArray.constData());
            int signatureLength = signatureByteArray.length();

            ERR_clear_error();
            // ECSDA verification prototype: note that type is currently ignored
            // int ECDSA_verify(int type, const unsigned char *dgst, int dgstlen,
            // const unsigned char *sig, int siglen, EC_KEY *eckey);
            int answer = ECDSA_verify(0,
                digest,
                digestLength,
                signature,
                signatureLength,
                ec);
            long error = ERR_get_error();
            if (error != 0 || answer == -1) {
                qCWarning(entities) << "ERROR while verifying signature!"
                    << "\nKey:" << publicKey << "\nutf8 Key Length:" << keyLength
                    << "\nDigest:" << digest << "\nDigest Length:" << digestLength
                    << "\nSignature:" << signature << "\nSignature Length:" << signatureLength;
                while (error != 0) {
                    const char* error_str = ERR_error_string(error, NULL);
                    qCWarning(entities) << "EC error:" << error_str;
                    error = ERR_get_error();
                }
            }
            EC_KEY_free(ec);
            if (bio) {
                BIO_free(bio);
            }
            if (evp_key) {
                EVP_PKEY_free(evp_key);
            }
            return (answer == 1);
        } else {
            if (bio) {
                BIO_free(bio);
            }
            if (evp_key) {
                EVP_PKEY_free(evp_key);
            }
            long error = ERR_get_error();
            const char* error_str = ERR_error_string(error, NULL);
            qCWarning(entities) << "Failed to verify signature! key" << publicKey << " EC key error:" << error_str;
            return false;
        }
    } else {
        if (bio) {
            BIO_free(bio);
        }
        long error = ERR_get_error();
        const char* error_str = ERR_error_string(error, NULL);
        qCWarning(entities) << "Failed to verify signature! key" << publicKey << " EC PEM error:" << error_str;
        return false;
    }
}

bool EntityItemProperties::verifyStaticCertificateProperties() {
    // True IFF a non-empty certificateID matches the static certificate json.
    // I.e., if we can verify that the certificateID was produced by High Fidelity signing the static certificate hash.
    return verifySignature(EntityItem::_marketplacePublicKey, getStaticCertificateHash(), QByteArray::fromBase64(getCertificateID().toUtf8()));
}

void EntityItemProperties::convertToCloneProperties(const EntityItemID& entityIDToClone) {
    setName(getName() + "-clone-" + entityIDToClone.toString());
    setLocked(false);
    setParentID(QUuid());
    setParentJointIndex(-1);
    setLifetime(getCloneLifetime());
    setDynamic(getCloneDynamic());
    if (getEntityHostType() != entity::HostType::LOCAL) {
        setEntityHostType(getCloneAvatarEntity() ? entity::HostType::AVATAR : entity::HostType::DOMAIN);
    } else {
        // Local Entities clone as local entities
        setEntityHostType(entity::HostType::LOCAL);
        setCollisionless(true);
    }
    setCreated(usecTimestampNow());
    setLastEdited(usecTimestampNow());
    setCloneable(ENTITY_ITEM_DEFAULT_CLONEABLE);
    setCloneLifetime(ENTITY_ITEM_DEFAULT_CLONE_LIFETIME);
    setCloneLimit(ENTITY_ITEM_DEFAULT_CLONE_LIMIT);
    setCloneDynamic(ENTITY_ITEM_DEFAULT_CLONE_DYNAMIC);
    setCloneAvatarEntity(ENTITY_ITEM_DEFAULT_CLONE_AVATAR_ENTITY);
}

bool EntityItemProperties::blobToProperties(QScriptEngine& scriptEngine, const QByteArray& blob, EntityItemProperties& properties) {
    // DANGER: this method is NOT efficient.
    // begin recipe for converting unfortunately-formatted-binary-blob to EntityItemProperties
    QJsonDocument jsonProperties = QJsonDocument::fromBinaryData(blob);
    if (!jsonProperties.isObject()) {
        qCDebug(entities) << "bad avatarEntityData json" << QString(blob.toHex());
        return false;
    }
    QVariant variant = jsonProperties.toVariant();
    QVariantMap variantMap = variant.toMap();
    QScriptValue scriptValue = variantMapToScriptValue(variantMap, scriptEngine);
    EntityItemPropertiesFromScriptValueHonorReadOnly(scriptValue, properties);
    // end recipe
    return true;
}

void EntityItemProperties::propertiesToBlob(QScriptEngine& scriptEngine, const QUuid& myAvatarID, const EntityItemProperties& properties, QByteArray& blob) {
    // DANGER: this method is NOT efficient.
    // begin recipe for extracting unfortunately-formatted-binary-blob from EntityItem
    QScriptValue scriptValue = EntityItemNonDefaultPropertiesToScriptValue(&scriptEngine, properties);
    QVariant variantProperties = scriptValue.toVariant();
    QJsonDocument jsonProperties = QJsonDocument::fromVariant(variantProperties);
    // the ID of the parent/avatar changes from session to session.  use a special UUID to indicate the avatar
    QJsonObject jsonObject = jsonProperties.object();
    if (jsonObject.contains("parentID")) {
        if (QUuid(jsonObject["parentID"].toString()) == myAvatarID) {
            jsonObject["parentID"] = AVATAR_SELF_ID.toString();
        }
    }
    jsonProperties = QJsonDocument(jsonObject);
    blob = jsonProperties.toBinaryData();
    // end recipe
}

QDebug& operator<<(QDebug& dbg, const EntityPropertyFlags& f) {
    QString result = "[ ";

    for (int i = 0; i < PROP_AFTER_LAST_ITEM; i++) {
        auto prop = EntityPropertyList(i);
        if (f.getHasProperty(prop)) {
            result = result + _enumsToPropertyStrings[prop] + " ";
        }
    }

    result += "]";
    dbg.nospace() << result;
    return dbg;
}
