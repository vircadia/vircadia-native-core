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
#include "EntityItemPropertiesDefaults.h"
#include "ModelEntityItem.h"
#include "ParticleEffectEntityItem.h"
#include "TextEntityItem.h"
#include "ZoneEntityItem.h"
#include "PolyVoxEntityItem.h"
#include "LineEntityItem.h"
#include "PolyLineEntityItem.h"

AtmospherePropertyGroup EntityItemProperties::_staticAtmosphere;
SkyboxPropertyGroup EntityItemProperties::_staticSkybox;
StagePropertyGroup EntityItemProperties::_staticStage;

EntityPropertyList PROP_LAST_ITEM = (EntityPropertyList)(PROP_AFTER_LAST_ITEM - 1);

EntityItemProperties::EntityItemProperties() :

CONSTRUCT_PROPERTY(visible, ENTITY_ITEM_DEFAULT_VISIBLE),
CONSTRUCT_PROPERTY(position, 0.0f),
CONSTRUCT_PROPERTY(dimensions, ENTITY_ITEM_DEFAULT_DIMENSIONS),
CONSTRUCT_PROPERTY(rotation, ENTITY_ITEM_DEFAULT_ROTATION),
CONSTRUCT_PROPERTY(density, ENTITY_ITEM_DEFAULT_DENSITY),
CONSTRUCT_PROPERTY(velocity, ENTITY_ITEM_DEFAULT_VELOCITY),
CONSTRUCT_PROPERTY(gravity, ENTITY_ITEM_DEFAULT_GRAVITY),
CONSTRUCT_PROPERTY(acceleration, ENTITY_ITEM_DEFAULT_ACCELERATION),
CONSTRUCT_PROPERTY(damping, ENTITY_ITEM_DEFAULT_DAMPING),
CONSTRUCT_PROPERTY(restitution, ENTITY_ITEM_DEFAULT_RESTITUTION),
CONSTRUCT_PROPERTY(friction, ENTITY_ITEM_DEFAULT_FRICTION),
CONSTRUCT_PROPERTY(lifetime, ENTITY_ITEM_DEFAULT_LIFETIME),
CONSTRUCT_PROPERTY(created, UNKNOWN_CREATED_TIME),
CONSTRUCT_PROPERTY(script, ENTITY_ITEM_DEFAULT_SCRIPT),
CONSTRUCT_PROPERTY(scriptTimestamp, ENTITY_ITEM_DEFAULT_SCRIPT_TIMESTAMP),
CONSTRUCT_PROPERTY(collisionSoundURL, ENTITY_ITEM_DEFAULT_COLLISION_SOUND_URL),
CONSTRUCT_PROPERTY(color, ),
CONSTRUCT_PROPERTY(modelURL, ""),
CONSTRUCT_PROPERTY(compoundShapeURL, ""),
CONSTRUCT_PROPERTY(animationURL, ""),
CONSTRUCT_PROPERTY(animationFPS, ModelEntityItem::DEFAULT_ANIMATION_FPS),
CONSTRUCT_PROPERTY(animationFrameIndex, ModelEntityItem::DEFAULT_ANIMATION_FRAME_INDEX),
CONSTRUCT_PROPERTY(animationIsPlaying, ModelEntityItem::DEFAULT_ANIMATION_IS_PLAYING),
CONSTRUCT_PROPERTY(registrationPoint, ENTITY_ITEM_DEFAULT_REGISTRATION_POINT),
CONSTRUCT_PROPERTY(angularVelocity, ENTITY_ITEM_DEFAULT_ANGULAR_VELOCITY),
CONSTRUCT_PROPERTY(angularDamping, ENTITY_ITEM_DEFAULT_ANGULAR_DAMPING),
CONSTRUCT_PROPERTY(ignoreForCollisions, ENTITY_ITEM_DEFAULT_IGNORE_FOR_COLLISIONS),
CONSTRUCT_PROPERTY(collisionsWillMove, ENTITY_ITEM_DEFAULT_COLLISIONS_WILL_MOVE),
CONSTRUCT_PROPERTY(isSpotlight, false),
CONSTRUCT_PROPERTY(intensity, 1.0f),
CONSTRUCT_PROPERTY(exponent, 0.0f),
CONSTRUCT_PROPERTY(cutoff, ENTITY_ITEM_DEFAULT_CUTOFF),
CONSTRUCT_PROPERTY(locked, ENTITY_ITEM_DEFAULT_LOCKED),
CONSTRUCT_PROPERTY(textures, ""),
CONSTRUCT_PROPERTY(animationSettings, "{\"firstFrame\":0,\"fps\":30,\"frameIndex\":0,\"hold\":false,"
                   "\"lastFrame\":100000,\"loop\":false,\"running\":false,\"startAutomatically\":false}"),
CONSTRUCT_PROPERTY(userData, ENTITY_ITEM_DEFAULT_USER_DATA),
CONSTRUCT_PROPERTY(simulationOwner, SimulationOwner()),
CONSTRUCT_PROPERTY(text, TextEntityItem::DEFAULT_TEXT),
CONSTRUCT_PROPERTY(lineHeight, TextEntityItem::DEFAULT_LINE_HEIGHT),
CONSTRUCT_PROPERTY(textColor, TextEntityItem::DEFAULT_TEXT_COLOR),
CONSTRUCT_PROPERTY(backgroundColor, TextEntityItem::DEFAULT_BACKGROUND_COLOR),
CONSTRUCT_PROPERTY(shapeType, SHAPE_TYPE_NONE),
CONSTRUCT_PROPERTY(maxParticles, ParticleEffectEntityItem::DEFAULT_MAX_PARTICLES),
CONSTRUCT_PROPERTY(lifespan, ParticleEffectEntityItem::DEFAULT_LIFESPAN),
CONSTRUCT_PROPERTY(emitRate, ParticleEffectEntityItem::DEFAULT_EMIT_RATE),
CONSTRUCT_PROPERTY(emitVelocity, ParticleEffectEntityItem::DEFAULT_EMIT_VELOCITY),
CONSTRUCT_PROPERTY(velocitySpread, ParticleEffectEntityItem::DEFAULT_VELOCITY_SPREAD),
CONSTRUCT_PROPERTY(emitAcceleration, ParticleEffectEntityItem::DEFAULT_EMIT_ACCELERATION),
CONSTRUCT_PROPERTY(accelerationSpread, ParticleEffectEntityItem::DEFAULT_ACCELERATION_SPREAD),
CONSTRUCT_PROPERTY(particleRadius, ParticleEffectEntityItem::DEFAULT_PARTICLE_RADIUS),
CONSTRUCT_PROPERTY(marketplaceID, ENTITY_ITEM_DEFAULT_MARKETPLACE_ID),
CONSTRUCT_PROPERTY(keyLightColor, ZoneEntityItem::DEFAULT_KEYLIGHT_COLOR),
CONSTRUCT_PROPERTY(keyLightIntensity, ZoneEntityItem::DEFAULT_KEYLIGHT_INTENSITY),
CONSTRUCT_PROPERTY(keyLightAmbientIntensity, ZoneEntityItem::DEFAULT_KEYLIGHT_AMBIENT_INTENSITY),
CONSTRUCT_PROPERTY(keyLightDirection, ZoneEntityItem::DEFAULT_KEYLIGHT_DIRECTION),
CONSTRUCT_PROPERTY(voxelVolumeSize, PolyVoxEntityItem::DEFAULT_VOXEL_VOLUME_SIZE),
CONSTRUCT_PROPERTY(voxelData, PolyVoxEntityItem::DEFAULT_VOXEL_DATA),
CONSTRUCT_PROPERTY(voxelSurfaceStyle, PolyVoxEntityItem::DEFAULT_VOXEL_SURFACE_STYLE),
CONSTRUCT_PROPERTY(name, ENTITY_ITEM_DEFAULT_NAME),
CONSTRUCT_PROPERTY(backgroundMode, BACKGROUND_MODE_INHERIT),
CONSTRUCT_PROPERTY(sourceUrl, ""),
CONSTRUCT_PROPERTY(lineWidth, LineEntityItem::DEFAULT_LINE_WIDTH),
CONSTRUCT_PROPERTY(linePoints, QVector<glm::vec3>()),
CONSTRUCT_PROPERTY(faceCamera, TextEntityItem::DEFAULT_FACE_CAMERA),
CONSTRUCT_PROPERTY(actionData, QByteArray()),
CONSTRUCT_PROPERTY(normals, QVector<glm::vec3>()),
CONSTRUCT_PROPERTY(strokeWidths, QVector<float>()),
CONSTRUCT_PROPERTY(xTextureURL, ""),
CONSTRUCT_PROPERTY(yTextureURL, ""),
CONSTRUCT_PROPERTY(zTextureURL, ""),

_id(UNKNOWN_ENTITY_ID),
_idSet(false),
_lastEdited(0),
_type(EntityTypes::Unknown),

_glowLevel(0.0f),
_localRenderAlpha(1.0f),

_glowLevelChanged(false),
_localRenderAlphaChanged(false),

_defaultSettings(true),
_naturalDimensions(1.0f, 1.0f, 1.0f),
_naturalPosition(0.0f, 0.0f, 0.0f)
{
}

EntityItemProperties::~EntityItemProperties() {
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

bool EntityItemProperties::animationSettingsChanged() const {
    return _animationSettingsChanged;
}

void EntityItemProperties::setAnimationSettings(const QString& value) {
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, frameIndex, or running, those values will be parsed out and
    // will over ride the regular animation settings

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();
    if (settingsMap.contains("fps")) {
        float fps = settingsMap["fps"].toFloat();
        setAnimationFPS(fps);
    }

    if (settingsMap.contains("frameIndex")) {
        float frameIndex = settingsMap["frameIndex"].toFloat();
        setAnimationFrameIndex(frameIndex);
    }

    if (settingsMap.contains("running")) {
        bool running = settingsMap["running"].toBool();
        setAnimationIsPlaying(running);
    }

    _animationSettings = value;
    _animationSettingsChanged = true;
}

QString EntityItemProperties::getAnimationSettings() const {
    // the animations setting is a JSON string that may contain various animation settings.
    // if it includes fps, frameIndex, or running, those values will be parsed out and
    // will over ride the regular animation settings
    QString value = _animationSettings;

    QJsonDocument settingsAsJson = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject settingsAsJsonObject = settingsAsJson.object();
    QVariantMap settingsMap = settingsAsJsonObject.toVariantMap();

    QVariant fpsValue(getAnimationFPS());
    settingsMap["fps"] = fpsValue;

    QVariant frameIndexValue(getAnimationFrameIndex());
    settingsMap["frameIndex"] = frameIndexValue;

    QVariant runningValue(getAnimationIsPlaying());
    settingsMap["running"] = runningValue;

    settingsAsJsonObject = QJsonObject::fromVariantMap(settingsMap);
    QJsonDocument newDocument(settingsAsJsonObject);
    QByteArray jsonByteArray = newDocument.toJson(QJsonDocument::Compact);
    QString jsonByteString(jsonByteArray);
    return jsonByteString;
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

    getAtmosphere().debugDump();
    getSkybox().debugDump();

    qCDebug(entities) << "   changed properties...";
    EntityPropertyFlags props = getChangedProperties();
    props.debugDumpBits();
}

void EntityItemProperties::setLastEdited(quint64 usecTime) {
    _lastEdited = usecTime > _created ? usecTime : _created;
}

const char* shapeTypeNames[] = {"none", "box", "sphere", "ellipsoid", "plane", "compound", "capsule-x",
    "capsule-y", "capsule-z", "cylinder-x", "cylinder-y", "cylinder-z"};

QHash<QString, ShapeType> stringToShapeTypeLookup;

void addShapeType(ShapeType type) {
    stringToShapeTypeLookup[shapeTypeNames[type]] = type;
}

void buildStringToShapeTypeLookup() {
    addShapeType(SHAPE_TYPE_NONE);
    addShapeType(SHAPE_TYPE_BOX);
    addShapeType(SHAPE_TYPE_SPHERE);
    addShapeType(SHAPE_TYPE_ELLIPSOID);
    addShapeType(SHAPE_TYPE_PLANE);
    addShapeType(SHAPE_TYPE_COMPOUND);
    addShapeType(SHAPE_TYPE_CAPSULE_X);
    addShapeType(SHAPE_TYPE_CAPSULE_Y);
    addShapeType(SHAPE_TYPE_CAPSULE_Z);
    addShapeType(SHAPE_TYPE_CYLINDER_X);
    addShapeType(SHAPE_TYPE_CYLINDER_Y);
    addShapeType(SHAPE_TYPE_CYLINDER_Z);
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

const char* backgroundModeNames[] = {"inherit", "atmosphere", "skybox" };

QHash<QString, BackgroundMode> stringToBackgroundModeLookup;

void addBackgroundMode(BackgroundMode type) {
    stringToBackgroundModeLookup[backgroundModeNames[type]] = type;
}

void buildStringToBackgroundModeLookup() {
    addBackgroundMode(BACKGROUND_MODE_INHERIT);
    addBackgroundMode(BACKGROUND_MODE_ATMOSPHERE);
    addBackgroundMode(BACKGROUND_MODE_SKYBOX);
}

QString EntityItemProperties::getBackgroundModeAsString() const {
    if (_backgroundMode < sizeof(backgroundModeNames) / sizeof(char *))
        return QString(backgroundModeNames[_backgroundMode]);
    return QString(backgroundModeNames[BACKGROUND_MODE_INHERIT]);
}

QString EntityItemProperties::getBackgroundModeString(BackgroundMode mode) {
    if (mode < sizeof(backgroundModeNames) / sizeof(char *))
        return QString(backgroundModeNames[mode]);
    return QString(backgroundModeNames[BACKGROUND_MODE_INHERIT]);
}

void EntityItemProperties::setBackgroundModeFromString(const QString& backgroundMode) {
    if (stringToBackgroundModeLookup.empty()) {
        buildStringToBackgroundModeLookup();
    }
    auto backgroundModeItr = stringToBackgroundModeLookup.find(backgroundMode.toLower());
    if (backgroundModeItr != stringToBackgroundModeLookup.end()) {
        _backgroundMode = backgroundModeItr.value();
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
    CHECK_PROPERTY_CHANGE(PROP_MODEL_URL, modelURL);
    CHECK_PROPERTY_CHANGE(PROP_COMPOUND_SHAPE_URL, compoundShapeURL);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_URL, animationURL);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_PLAYING, animationIsPlaying);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_FRAME_INDEX, animationFrameIndex);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_FPS, animationFPS);
    CHECK_PROPERTY_CHANGE(PROP_ANIMATION_SETTINGS, animationSettings);
    CHECK_PROPERTY_CHANGE(PROP_VISIBLE, visible);
    CHECK_PROPERTY_CHANGE(PROP_REGISTRATION_POINT, registrationPoint);
    CHECK_PROPERTY_CHANGE(PROP_ANGULAR_VELOCITY, angularVelocity);
    CHECK_PROPERTY_CHANGE(PROP_ANGULAR_DAMPING, angularDamping);
    CHECK_PROPERTY_CHANGE(PROP_IGNORE_FOR_COLLISIONS, ignoreForCollisions);
    CHECK_PROPERTY_CHANGE(PROP_COLLISIONS_WILL_MOVE, collisionsWillMove);
    CHECK_PROPERTY_CHANGE(PROP_IS_SPOTLIGHT, isSpotlight);
    CHECK_PROPERTY_CHANGE(PROP_INTENSITY, intensity);
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
    CHECK_PROPERTY_CHANGE(PROP_MAX_PARTICLES, maxParticles);
    CHECK_PROPERTY_CHANGE(PROP_LIFESPAN, lifespan);
    CHECK_PROPERTY_CHANGE(PROP_EMIT_RATE, emitRate);
    CHECK_PROPERTY_CHANGE(PROP_EMIT_VELOCITY, emitVelocity);
    CHECK_PROPERTY_CHANGE(PROP_VELOCITY_SPREAD, velocitySpread);
    CHECK_PROPERTY_CHANGE(PROP_EMIT_ACCELERATION, emitAcceleration);
    CHECK_PROPERTY_CHANGE(PROP_ACCELERATION_SPREAD, accelerationSpread);
    CHECK_PROPERTY_CHANGE(PROP_PARTICLE_RADIUS, particleRadius);
    CHECK_PROPERTY_CHANGE(PROP_MARKETPLACE_ID, marketplaceID);
    CHECK_PROPERTY_CHANGE(PROP_NAME, name);
    CHECK_PROPERTY_CHANGE(PROP_KEYLIGHT_COLOR, keyLightColor);
    CHECK_PROPERTY_CHANGE(PROP_KEYLIGHT_INTENSITY, keyLightIntensity);
    CHECK_PROPERTY_CHANGE(PROP_KEYLIGHT_AMBIENT_INTENSITY, keyLightAmbientIntensity);
    CHECK_PROPERTY_CHANGE(PROP_KEYLIGHT_DIRECTION, keyLightDirection);
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

    changedProperties += _stage.getChangedProperties();
    changedProperties += _atmosphere.getChangedProperties();
    changedProperties += _skybox.getChangedProperties();

    return changedProperties;
}

QScriptValue EntityItemProperties::copyToScriptValue(QScriptEngine* engine, bool skipDefaults) const {
    QScriptValue properties = engine->newObject();
    EntityItemProperties defaultEntityProperties;

    if (_idSet) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(id, _id.toString());
    }

    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(type, EntityTypes::getEntityTypeName(_type));
    COPY_PROPERTY_TO_QSCRIPTVALUE(position);
    COPY_PROPERTY_TO_QSCRIPTVALUE(dimensions);
    if (!skipDefaults) {
        COPY_PROPERTY_TO_QSCRIPTVALUE(naturalDimensions); // gettable, but not settable
        COPY_PROPERTY_TO_QSCRIPTVALUE(naturalPosition);
    }
    COPY_PROPERTY_TO_QSCRIPTVALUE(rotation);
    COPY_PROPERTY_TO_QSCRIPTVALUE(velocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(gravity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(acceleration);
    COPY_PROPERTY_TO_QSCRIPTVALUE(damping);
    COPY_PROPERTY_TO_QSCRIPTVALUE(restitution);
    COPY_PROPERTY_TO_QSCRIPTVALUE(friction);
    COPY_PROPERTY_TO_QSCRIPTVALUE(density);
    COPY_PROPERTY_TO_QSCRIPTVALUE(lifetime);

    if (!skipDefaults || _lifetime != defaultEntityProperties._lifetime) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(age, getAge()); // gettable, but not settable
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(ageAsText, formatSecondsElapsed(getAge())); // gettable, but not settable
    }

    auto created = QDateTime::fromMSecsSinceEpoch(getCreated() / 1000.0f, Qt::UTC); // usec per msec
    created.setTimeSpec(Qt::OffsetFromUTC);
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(created, created.toString(Qt::ISODate));

    COPY_PROPERTY_TO_QSCRIPTVALUE(script);
    COPY_PROPERTY_TO_QSCRIPTVALUE(scriptTimestamp);
    COPY_PROPERTY_TO_QSCRIPTVALUE(registrationPoint);
    COPY_PROPERTY_TO_QSCRIPTVALUE(angularVelocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(angularDamping);
    COPY_PROPERTY_TO_QSCRIPTVALUE(visible);
    COPY_PROPERTY_TO_QSCRIPTVALUE(color);
    COPY_PROPERTY_TO_QSCRIPTVALUE(modelURL);
    COPY_PROPERTY_TO_QSCRIPTVALUE(compoundShapeURL);
    COPY_PROPERTY_TO_QSCRIPTVALUE(animationURL);
    COPY_PROPERTY_TO_QSCRIPTVALUE(animationIsPlaying);
    COPY_PROPERTY_TO_QSCRIPTVALUE(animationFPS);
    COPY_PROPERTY_TO_QSCRIPTVALUE(animationFrameIndex);
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(animationSettings, getAnimationSettings());
    COPY_PROPERTY_TO_QSCRIPTVALUE(glowLevel);
    COPY_PROPERTY_TO_QSCRIPTVALUE(localRenderAlpha);
    COPY_PROPERTY_TO_QSCRIPTVALUE(ignoreForCollisions);
    COPY_PROPERTY_TO_QSCRIPTVALUE(collisionsWillMove);
    COPY_PROPERTY_TO_QSCRIPTVALUE(isSpotlight);
    COPY_PROPERTY_TO_QSCRIPTVALUE(intensity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(exponent);
    COPY_PROPERTY_TO_QSCRIPTVALUE(cutoff);
    COPY_PROPERTY_TO_QSCRIPTVALUE(locked);
    COPY_PROPERTY_TO_QSCRIPTVALUE(textures);
    COPY_PROPERTY_TO_QSCRIPTVALUE(userData);
    //COPY_PROPERTY_TO_QSCRIPTVALUE(simulationOwner); // TODO: expose this for JSON saves?
    COPY_PROPERTY_TO_QSCRIPTVALUE(text);
    COPY_PROPERTY_TO_QSCRIPTVALUE(lineHeight);
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(textColor, getTextColor());
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(backgroundColor, getBackgroundColor());
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(shapeType, getShapeTypeAsString());
    COPY_PROPERTY_TO_QSCRIPTVALUE(maxParticles);
    COPY_PROPERTY_TO_QSCRIPTVALUE(lifespan);
    COPY_PROPERTY_TO_QSCRIPTVALUE(emitRate);
    COPY_PROPERTY_TO_QSCRIPTVALUE(emitVelocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(velocitySpread);
    COPY_PROPERTY_TO_QSCRIPTVALUE(emitAcceleration);
    COPY_PROPERTY_TO_QSCRIPTVALUE(accelerationSpread);
    COPY_PROPERTY_TO_QSCRIPTVALUE(particleRadius);
    COPY_PROPERTY_TO_QSCRIPTVALUE(marketplaceID);
    COPY_PROPERTY_TO_QSCRIPTVALUE(name);
    COPY_PROPERTY_TO_QSCRIPTVALUE(collisionSoundURL);

    COPY_PROPERTY_TO_QSCRIPTVALUE(keyLightColor);
    COPY_PROPERTY_TO_QSCRIPTVALUE(keyLightIntensity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(keyLightAmbientIntensity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(keyLightDirection);
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(backgroundMode, getBackgroundModeAsString());
    COPY_PROPERTY_TO_QSCRIPTVALUE(sourceUrl);
    COPY_PROPERTY_TO_QSCRIPTVALUE(voxelVolumeSize);
    COPY_PROPERTY_TO_QSCRIPTVALUE(voxelData);
    COPY_PROPERTY_TO_QSCRIPTVALUE(voxelSurfaceStyle);
    COPY_PROPERTY_TO_QSCRIPTVALUE(lineWidth);
    COPY_PROPERTY_TO_QSCRIPTVALUE(linePoints);
    COPY_PROPERTY_TO_QSCRIPTVALUE(href);
    COPY_PROPERTY_TO_QSCRIPTVALUE(description);
    COPY_PROPERTY_TO_QSCRIPTVALUE(faceCamera);
    COPY_PROPERTY_TO_QSCRIPTVALUE(actionData);
    COPY_PROPERTY_TO_QSCRIPTVALUE(normals);
    COPY_PROPERTY_TO_QSCRIPTVALUE(strokeWidths);

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
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(sittingPoints, sittingPoints); // gettable, but not settable
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

    QString textureNamesList = _textureNames.join(",\n");
    if (!skipDefaults) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER_NO_SKIP(originalTextures, textureNamesList); // gettable, but not settable
    }

    _stage.copyToScriptValue(properties, engine, skipDefaults, defaultEntityProperties);
    _atmosphere.copyToScriptValue(properties, engine, skipDefaults, defaultEntityProperties);
    _skybox.copyToScriptValue(properties, engine, skipDefaults, defaultEntityProperties);

    COPY_PROPERTY_TO_QSCRIPTVALUE(xTextureURL);
    COPY_PROPERTY_TO_QSCRIPTVALUE(yTextureURL);
    COPY_PROPERTY_TO_QSCRIPTVALUE(zTextureURL);

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
    COPY_PROPERTY_FROM_QSCRIPTVALUE(modelURL, QString, setModelURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(compoundShapeURL, QString, setCompoundShapeURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(animationURL, QString, setAnimationURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(animationIsPlaying, bool, setAnimationIsPlaying);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(animationFPS, float, setAnimationFPS);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(animationFrameIndex, float, setAnimationFrameIndex);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(animationSettings, QString, setAnimationSettings);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(glowLevel, float, setGlowLevel);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(localRenderAlpha, float, setLocalRenderAlpha);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(ignoreForCollisions, bool, setIgnoreForCollisions);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(collisionsWillMove, bool, setCollisionsWillMove);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(isSpotlight, bool, setIsSpotlight);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(intensity, float, setIntensity);
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
    COPY_PROPERTY_FROM_QSCRIPTVALUE(maxParticles, float, setMaxParticles);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(lifespan, float, setLifespan);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitRate, float, setEmitRate);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitVelocity, glmVec3, setEmitVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(velocitySpread, glmVec3, setVelocitySpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(emitAcceleration, glmVec3, setEmitAcceleration);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(accelerationSpread, glmVec3, setAccelerationSpread);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(particleRadius, float, setParticleRadius);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(marketplaceID, QString, setMarketplaceID);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(name, QString, setName);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(collisionSoundURL, QString, setCollisionSoundURL);

    COPY_PROPERTY_FROM_QSCRIPTVALUE(keyLightColor, xColor, setKeyLightColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(keyLightIntensity, float, setKeyLightIntensity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(keyLightAmbientIntensity, float, setKeyLightAmbientIntensity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(keyLightDirection, glmVec3, setKeyLightDirection);
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

    _stage.copyFromScriptValue(object, _defaultSettings);
    _atmosphere.copyFromScriptValue(object, _defaultSettings);
    _skybox.copyFromScriptValue(object, _defaultSettings);


    COPY_PROPERTY_FROM_QSCRIPTVALUE(xTextureURL, QString, setXTextureURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(yTextureURL, QString, setYTextureURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE(zTextureURL, QString, setZTextureURL);

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

    // assuming we have rome to fit our octalCode, proceed...
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
            APPEND_ENTITY_PROPERTY(PROP_IGNORE_FOR_COLLISIONS, properties.getIgnoreForCollisions());
            APPEND_ENTITY_PROPERTY(PROP_COLLISIONS_WILL_MOVE, properties.getCollisionsWillMove());
            APPEND_ENTITY_PROPERTY(PROP_LOCKED, properties.getLocked());
            APPEND_ENTITY_PROPERTY(PROP_USER_DATA, properties.getUserData());
            APPEND_ENTITY_PROPERTY(PROP_HREF, properties.getHref());
            APPEND_ENTITY_PROPERTY(PROP_DESCRIPTION, properties.getDescription());


            if (properties.getType() == EntityTypes::Web) {
                APPEND_ENTITY_PROPERTY(PROP_SOURCE_URL, properties.getSourceUrl());
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
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_URL, properties.getAnimationURL());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, properties.getAnimationFPS());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, properties.getAnimationFrameIndex());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, properties.getAnimationIsPlaying());
                APPEND_ENTITY_PROPERTY(PROP_TEXTURES, properties.getTextures());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, properties.getAnimationSettings());
                APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)(properties.getShapeType()));
            }

            if (properties.getType() == EntityTypes::Light) {
                APPEND_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, properties.getIsSpotlight());
                APPEND_ENTITY_PROPERTY(PROP_COLOR, properties.getColor());
                APPEND_ENTITY_PROPERTY(PROP_INTENSITY, properties.getIntensity());
                APPEND_ENTITY_PROPERTY(PROP_EXPONENT, properties.getExponent());
                APPEND_ENTITY_PROPERTY(PROP_CUTOFF, properties.getCutoff());
            }

            if (properties.getType() == EntityTypes::ParticleEffect) {
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, properties.getAnimationFPS());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, properties.getAnimationFrameIndex());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, properties.getAnimationIsPlaying());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, properties.getAnimationSettings());
                APPEND_ENTITY_PROPERTY(PROP_TEXTURES, properties.getTextures());
                APPEND_ENTITY_PROPERTY(PROP_MAX_PARTICLES, properties.getMaxParticles());
                APPEND_ENTITY_PROPERTY(PROP_LIFESPAN, properties.getLifespan());
                APPEND_ENTITY_PROPERTY(PROP_EMIT_RATE, properties.getEmitRate());
                APPEND_ENTITY_PROPERTY(PROP_EMIT_VELOCITY, properties.getEmitVelocity());
                APPEND_ENTITY_PROPERTY(PROP_VELOCITY_SPREAD, properties.getVelocitySpread());
                APPEND_ENTITY_PROPERTY(PROP_EMIT_ACCELERATION, properties.getEmitAcceleration());
                APPEND_ENTITY_PROPERTY(PROP_ACCELERATION_SPREAD, properties.getAccelerationSpread());
                APPEND_ENTITY_PROPERTY(PROP_PARTICLE_RADIUS, properties.getParticleRadius());
            
            }

            if (properties.getType() == EntityTypes::Zone) {
                APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_COLOR, properties.getKeyLightColor());
                APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_INTENSITY,  properties.getKeyLightIntensity());
                APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, properties.getKeyLightAmbientIntensity());
                APPEND_ENTITY_PROPERTY(PROP_KEYLIGHT_DIRECTION, properties.getKeyLightDirection());

                _staticStage.setProperties(properties);
                _staticStage.appentToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit,  propertyCount, appendState );

                APPEND_ENTITY_PROPERTY(PROP_SHAPE_TYPE, (uint32_t)properties.getShapeType());
                APPEND_ENTITY_PROPERTY(PROP_COMPOUND_SHAPE_URL, properties.getCompoundShapeURL());

                APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_MODE, (uint32_t)properties.getBackgroundMode());

                _staticAtmosphere.setProperties(properties);
                _staticAtmosphere.appentToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit,  propertyCount, appendState );

                _staticSkybox.setProperties(properties);
                _staticSkybox.appentToEditPacket(packetData, requestedProperties, propertyFlags, propertiesDidntFit,  propertyCount, appendState );
            }

            if (properties.getType() == EntityTypes::PolyVox) {
                APPEND_ENTITY_PROPERTY(PROP_VOXEL_VOLUME_SIZE, properties.getVoxelVolumeSize());
                APPEND_ENTITY_PROPERTY(PROP_VOXEL_DATA, properties.getVoxelData());
                APPEND_ENTITY_PROPERTY(PROP_VOXEL_SURFACE_STYLE, properties.getVoxelSurfaceStyle());
                APPEND_ENTITY_PROPERTY(PROP_X_TEXTURE_URL, properties.getXTextureURL());
                APPEND_ENTITY_PROPERTY(PROP_Y_TEXTURE_URL, properties.getYTextureURL());
                APPEND_ENTITY_PROPERTY(PROP_Z_TEXTURE_URL, properties.getZTextureURL());
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
            }
            

            APPEND_ENTITY_PROPERTY(PROP_MARKETPLACE_ID, properties.getMarketplaceID());
            APPEND_ENTITY_PROPERTY(PROP_NAME, properties.getName());
            APPEND_ENTITY_PROPERTY(PROP_COLLISION_SOUND_URL, properties.getCollisionSoundURL());
            APPEND_ENTITY_PROPERTY(PROP_ACTION_DATA, properties.getActionData());
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
    int bytesToReadOfOctcode = bytesRequiredForCodeLength(octets);

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
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_IGNORE_FOR_COLLISIONS, bool, setIgnoreForCollisions);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLLISIONS_WILL_MOVE, bool, setCollisionsWillMove);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LOCKED, bool, setLocked);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_USER_DATA, QString, setUserData);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_HREF, QString, setHref);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DESCRIPTION, QString, setDescription);


    if (properties.getType() == EntityTypes::Web) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SOURCE_URL, QString, setSourceUrl);
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
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_URL, QString, setAnimationURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_FPS, float, setAnimationFPS);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_FRAME_INDEX, float, setAnimationFrameIndex);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_PLAYING, bool, setAnimationIsPlaying);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXTURES, QString, setTextures);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_SETTINGS, QString, setAnimationSettings);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SHAPE_TYPE, ShapeType, setShapeType);
    }

    if (properties.getType() == EntityTypes::Light) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_IS_SPOTLIGHT, bool, setIsSpotlight);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLOR, xColor, setColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_INTENSITY, float, setIntensity);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EXPONENT, float, setExponent);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CUTOFF, float, setCutoff);
    }

    if (properties.getType() == EntityTypes::ParticleEffect) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_FPS, float, setAnimationFPS);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_FRAME_INDEX, float, setAnimationFrameIndex);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_PLAYING, bool, setAnimationIsPlaying);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_SETTINGS, QString, setAnimationSettings);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_TEXTURES, QString, setTextures);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MAX_PARTICLES, float, setMaxParticles);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LIFESPAN, float, setLifespan);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_RATE, float, setEmitRate);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_VELOCITY, glm::vec3, setEmitVelocity);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VELOCITY_SPREAD, glm::vec3, setVelocitySpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EMIT_ACCELERATION, glm::vec3, setEmitAcceleration);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ACCELERATION_SPREAD, glm::vec3, setAccelerationSpread);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_PARTICLE_RADIUS, float, setParticleRadius);
    }

    if (properties.getType() == EntityTypes::Zone) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_KEYLIGHT_COLOR, xColor, setKeyLightColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_KEYLIGHT_INTENSITY,  float, setKeyLightIntensity);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_KEYLIGHT_AMBIENT_INTENSITY, float, setKeyLightAmbientIntensity);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_KEYLIGHT_DIRECTION, glm::vec3, setKeyLightDirection);

        properties.getStage().decodeFromEditPacket(propertyFlags, dataAt , processedBytes);

        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_SHAPE_TYPE, ShapeType, setShapeType);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COMPOUND_SHAPE_URL, QString, setCompoundShapeURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_BACKGROUND_MODE, BackgroundMode, setBackgroundMode);
        properties.getAtmosphere().decodeFromEditPacket(propertyFlags, dataAt , processedBytes);
        properties.getSkybox().decodeFromEditPacket(propertyFlags, dataAt , processedBytes);
    }

    if (properties.getType() == EntityTypes::PolyVox) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VOXEL_VOLUME_SIZE, glm::vec3, setVoxelVolumeSize);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VOXEL_DATA, QByteArray, setVoxelData);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VOXEL_SURFACE_STYLE, uint16_t, setVoxelSurfaceStyle);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_X_TEXTURE_URL, QString, setXTextureURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_Y_TEXTURE_URL, QString, setYTextureURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_Z_TEXTURE_URL, QString, setZTextureURL);
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
    }

    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MARKETPLACE_ID, QString, setMarketplaceID);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_NAME, QString, setName);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLLISION_SOUND_URL, QString, setCollisionSoundURL);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ACTION_DATA, QByteArray, setActionData);

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
    _modelURLChanged = true;
    _compoundShapeURLChanged = true;
    _animationURLChanged = true;
    _animationIsPlayingChanged = true;
    _animationFrameIndexChanged = true;
    _animationFPSChanged = true;
    _animationSettingsChanged = true;
    _glowLevelChanged = true;
    _localRenderAlphaChanged = true;
    _isSpotlightChanged = true;
    _ignoreForCollisionsChanged = true;
    _collisionsWillMoveChanged = true;

    _intensityChanged = true;
    _exponentChanged = true;
    _cutoffChanged = true;
    _lockedChanged = true;
    _texturesChanged = true;

    _textChanged = true;
    _lineHeightChanged = true;
    _textColorChanged = true;
    _backgroundColorChanged = true;
    _shapeTypeChanged = true;

    _maxParticlesChanged = true;
    _lifespanChanged = true;
    _emitRateChanged = true;
    _emitVelocityChanged = true;
    _velocitySpreadChanged = true;
    _emitAccelerationChanged = true;
    _accelerationSpreadChanged = true;
    _particleRadiusChanged = true;

    _marketplaceIDChanged = true;

    _keyLightColorChanged = true;
    _keyLightIntensityChanged = true;
    _keyLightAmbientIntensityChanged = true;
    _keyLightDirectionChanged = true;

    _backgroundModeChanged = true;
    _stage.markAllChanged();
    _atmosphere.markAllChanged();
    _skybox.markAllChanged();

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
}

/// The maximum bounding cube for the entity, independent of it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
///
AACube EntityItemProperties::getMaximumAACube() const {
    // * we know that the position is the center of rotation
    glm::vec3 centerOfRotation = _position; // also where _registration point is

    // * we know that the registration point is the center of rotation
    // * we can calculate the length of the furthest extent from the registration point
    //   as the dimensions * max (registrationPoint, (1.0,1.0,1.0) - registrationPoint)
    glm::vec3 registrationPoint = (_dimensions * _registrationPoint);
    glm::vec3 registrationRemainder = (_dimensions * (glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint));
    glm::vec3 furthestExtentFromRegistration = glm::max(registrationPoint, registrationRemainder);

    // * we know that if you rotate in any direction you would create a sphere
    //   that has a radius of the length of furthest extent from registration point
    float radius = glm::length(furthestExtentFromRegistration);

    // * we know that the minimum bounding cube of this maximum possible sphere is
    //   (center - radius) to (center + radius)
    glm::vec3 minimumCorner = centerOfRotation - glm::vec3(radius, radius, radius);
    float diameter = radius * 2.0f;

    return AACube(minimumCorner, diameter);
}

// The minimum bounding box for the entity.
AABox EntityItemProperties::getAABox() const {

    // _position represents the position of the registration point.
    glm::vec3 registrationRemainder = glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint;

    glm::vec3 unrotatedMinRelativeToEntity = - (_dimensions * _registrationPoint);
    glm::vec3 unrotatedMaxRelativeToEntity = _dimensions * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(getRotation());

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
        _compoundShapeURLChanged || _collisionsWillMoveChanged || _ignoreForCollisionsChanged;
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
