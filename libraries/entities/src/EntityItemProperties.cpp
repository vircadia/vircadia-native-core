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
#include <QObject>
#include <QtCore/QJsonDocument>

#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <RegisteredMetaTypes.h>

#include "EntityItem.h"
#include "EntityItemProperties.h"
#include "ModelEntityItem.h"
#include "TextEntityItem.h"

EntityItemProperties::EntityItemProperties() :

    _id(UNKNOWN_ENTITY_ID),
    _idSet(false),
    _lastEdited(0), // ????
    _created(UNKNOWN_CREATED_TIME),
    _type(EntityTypes::Unknown),

    _position(0),
    _dimensions(EntityItem::DEFAULT_DIMENSIONS),
    _rotation(EntityItem::DEFAULT_ROTATION),
    _mass(EntityItem::DEFAULT_MASS),
    _velocity(EntityItem::DEFAULT_VELOCITY),
    _gravity(EntityItem::DEFAULT_GRAVITY),
    _damping(EntityItem::DEFAULT_DAMPING),
    _lifetime(EntityItem::DEFAULT_LIFETIME),
    _userData(EntityItem::DEFAULT_USER_DATA),
    _script(EntityItem::DEFAULT_SCRIPT),
    _registrationPoint(EntityItem::DEFAULT_REGISTRATION_POINT),
    _angularVelocity(EntityItem::DEFAULT_ANGULAR_VELOCITY),
    _angularDamping(EntityItem::DEFAULT_ANGULAR_DAMPING),
    _visible(EntityItem::DEFAULT_VISIBLE),
    _ignoreForCollisions(EntityItem::DEFAULT_IGNORE_FOR_COLLISIONS),
    _collisionsWillMove(EntityItem::DEFAULT_COLLISIONS_WILL_MOVE),

    _positionChanged(false),
    _dimensionsChanged(false),
    _rotationChanged(false),
    _massChanged(false),
    _velocityChanged(false),
    _gravityChanged(false),
    _dampingChanged(false),
    _lifetimeChanged(false),
    _userDataChanged(false),
    _scriptChanged(false),
    _registrationPointChanged(false),
    _angularVelocityChanged(false),
    _angularDampingChanged(false),
    _visibleChanged(false),
    _ignoreForCollisionsChanged(false),
    _collisionsWillMoveChanged(false),

    _color(),
    _modelURL(""),
    _animationURL(""),
    _animationIsPlaying(ModelEntityItem::DEFAULT_ANIMATION_IS_PLAYING),
    _animationFrameIndex(ModelEntityItem::DEFAULT_ANIMATION_FRAME_INDEX),
    _animationFPS(ModelEntityItem::DEFAULT_ANIMATION_FPS),
    _animationSettings(""),
    _glowLevel(0.0f),
    _localRenderAlpha(1.0f),
    _isSpotlight(false),

    _colorChanged(false),
    _modelURLChanged(false),
    _animationURLChanged(false),
    _animationIsPlayingChanged(false),
    _animationFrameIndexChanged(false),
    _animationFPSChanged(false),
    _animationSettingsChanged(false),

    _glowLevelChanged(false),
    _localRenderAlphaChanged(false),
    _isSpotlightChanged(false),

    _diffuseColor(),
    _ambientColor(),
    _specularColor(),
    _constantAttenuation(1.0f),
    _linearAttenuation(0.0f), 
    _quadraticAttenuation(0.0f),
    _exponent(0.0f),
    _cutoff(PI),
    _locked(false),
    _textures(""),

    _diffuseColorChanged(false),
    _ambientColorChanged(false),
    _specularColorChanged(false),
    _constantAttenuationChanged(false),
    _linearAttenuationChanged(false),
    _quadraticAttenuationChanged(false),
    _exponentChanged(false),
    _cutoffChanged(false),
    _lockedChanged(false),
    _texturesChanged(false),

    CONSTRUCT_PROPERTY(text, TextEntityItem::DEFAULT_TEXT),
    CONSTRUCT_PROPERTY(lineHeight, TextEntityItem::DEFAULT_LINE_HEIGHT),
    CONSTRUCT_PROPERTY(textColor, TextEntityItem::DEFAULT_TEXT_COLOR),
    CONSTRUCT_PROPERTY(backgroundColor, TextEntityItem::DEFAULT_BACKGROUND_COLOR),

    _defaultSettings(true),
    _naturalDimensions(1.0f, 1.0f, 1.0f)
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

void EntityItemProperties::debugDump() const {
    qDebug() << "EntityItemProperties...";
    qDebug() << "    _type=" << EntityTypes::getEntityTypeName(_type);
    qDebug() << "   _id=" << _id;
    qDebug() << "   _idSet=" << _idSet;
    qDebug() << "   _position=" << _position.x << "," << _position.y << "," << _position.z;
    qDebug() << "   _dimensions=" << getDimensions();
    qDebug() << "   _modelURL=" << _modelURL;
    qDebug() << "   changed properties...";
    EntityPropertyFlags props = getChangedProperties();
    props.debugDumpBits();
}

EntityPropertyFlags EntityItemProperties::getChangedProperties() const {
    EntityPropertyFlags changedProperties;
    
    CHECK_PROPERTY_CHANGE(PROP_DIMENSIONS, dimensions);
    CHECK_PROPERTY_CHANGE(PROP_POSITION, position);
    CHECK_PROPERTY_CHANGE(PROP_ROTATION, rotation);
    CHECK_PROPERTY_CHANGE(PROP_MASS, mass);
    CHECK_PROPERTY_CHANGE(PROP_VELOCITY, velocity);
    CHECK_PROPERTY_CHANGE(PROP_GRAVITY, gravity);
    CHECK_PROPERTY_CHANGE(PROP_DAMPING, damping);
    CHECK_PROPERTY_CHANGE(PROP_LIFETIME, lifetime);
    CHECK_PROPERTY_CHANGE(PROP_SCRIPT, script);
    CHECK_PROPERTY_CHANGE(PROP_COLOR, color);
    CHECK_PROPERTY_CHANGE(PROP_MODEL_URL, modelURL);
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
    CHECK_PROPERTY_CHANGE(PROP_DIFFUSE_COLOR, diffuseColor);
    CHECK_PROPERTY_CHANGE(PROP_AMBIENT_COLOR, ambientColor);
    CHECK_PROPERTY_CHANGE(PROP_SPECULAR_COLOR, specularColor);
    CHECK_PROPERTY_CHANGE(PROP_CONSTANT_ATTENUATION, constantAttenuation);
    CHECK_PROPERTY_CHANGE(PROP_LINEAR_ATTENUATION, linearAttenuation);
    CHECK_PROPERTY_CHANGE(PROP_QUADRATIC_ATTENUATION, quadraticAttenuation);
    CHECK_PROPERTY_CHANGE(PROP_EXPONENT, exponent);
    CHECK_PROPERTY_CHANGE(PROP_CUTOFF, cutoff);
    CHECK_PROPERTY_CHANGE(PROP_LOCKED, locked);
    CHECK_PROPERTY_CHANGE(PROP_TEXTURES, textures);
    CHECK_PROPERTY_CHANGE(PROP_USER_DATA, userData);
    CHECK_PROPERTY_CHANGE(PROP_TEXT, text);
    CHECK_PROPERTY_CHANGE(PROP_LINE_HEIGHT, lineHeight);
    CHECK_PROPERTY_CHANGE(PROP_TEXT_COLOR, textColor);
    CHECK_PROPERTY_CHANGE(PROP_BACKGROUND_COLOR, backgroundColor);

    return changedProperties;
}

QScriptValue EntityItemProperties::copyToScriptValue(QScriptEngine* engine) const {
    QScriptValue properties = engine->newObject();

    if (_idSet) {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(id, _id.toString());
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(isKnownID, (_id != UNKNOWN_ENTITY_ID));
    } else {
        COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(isKnownID, false);
    }

    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(type, EntityTypes::getEntityTypeName(_type));
    COPY_PROPERTY_TO_QSCRIPTVALUE_VEC3(position);
    COPY_PROPERTY_TO_QSCRIPTVALUE_VEC3(dimensions);
    COPY_PROPERTY_TO_QSCRIPTVALUE_VEC3(naturalDimensions); // gettable, but not settable
    COPY_PROPERTY_TO_QSCRIPTVALUE_QUAT(rotation);
    COPY_PROPERTY_TO_QSCRIPTVALUE_VEC3(velocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE_VEC3(gravity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(damping);
    COPY_PROPERTY_TO_QSCRIPTVALUE(mass);
    COPY_PROPERTY_TO_QSCRIPTVALUE(lifetime);
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(age, getAge()); // gettable, but not settable
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(ageAsText, formatSecondsElapsed(getAge())); // gettable, but not settable
    COPY_PROPERTY_TO_QSCRIPTVALUE(script);
    COPY_PROPERTY_TO_QSCRIPTVALUE_VEC3(registrationPoint);
    COPY_PROPERTY_TO_QSCRIPTVALUE_VEC3(angularVelocity);
    COPY_PROPERTY_TO_QSCRIPTVALUE(angularDamping);
    COPY_PROPERTY_TO_QSCRIPTVALUE(visible);
    COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR(color);
    COPY_PROPERTY_TO_QSCRIPTVALUE(modelURL);
    COPY_PROPERTY_TO_QSCRIPTVALUE(animationURL);
    COPY_PROPERTY_TO_QSCRIPTVALUE(animationIsPlaying);
    COPY_PROPERTY_TO_QSCRIPTVALUE(animationFPS);
    COPY_PROPERTY_TO_QSCRIPTVALUE(animationFrameIndex);
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(animationSettings,getAnimationSettings());
    COPY_PROPERTY_TO_QSCRIPTVALUE(glowLevel);
    COPY_PROPERTY_TO_QSCRIPTVALUE(localRenderAlpha);
    COPY_PROPERTY_TO_QSCRIPTVALUE(ignoreForCollisions);
    COPY_PROPERTY_TO_QSCRIPTVALUE(collisionsWillMove);
    COPY_PROPERTY_TO_QSCRIPTVALUE(isSpotlight);
    COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR_GETTER(diffuseColor, getDiffuseColor()); 
    COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR_GETTER(ambientColor, getAmbientColor());
    COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR_GETTER(specularColor, getSpecularColor());
    COPY_PROPERTY_TO_QSCRIPTVALUE(constantAttenuation);
    COPY_PROPERTY_TO_QSCRIPTVALUE(linearAttenuation);
    COPY_PROPERTY_TO_QSCRIPTVALUE(quadraticAttenuation);
    COPY_PROPERTY_TO_QSCRIPTVALUE(exponent);
    COPY_PROPERTY_TO_QSCRIPTVALUE(cutoff);
    COPY_PROPERTY_TO_QSCRIPTVALUE(locked);
    COPY_PROPERTY_TO_QSCRIPTVALUE(textures);
    COPY_PROPERTY_TO_QSCRIPTVALUE(userData);
    COPY_PROPERTY_TO_QSCRIPTVALUE(text);
    COPY_PROPERTY_TO_QSCRIPTVALUE(lineHeight);
    COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR_GETTER(textColor, getTextColor());
    COPY_PROPERTY_TO_QSCRIPTVALUE_COLOR_GETTER(backgroundColor, getBackgroundColor());

    // Sitting properties support
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

    AABox aaBox = getAABoxInMeters();
    QScriptValue boundingBox = engine->newObject();
    QScriptValue bottomRightNear = vec3toScriptValue(engine, aaBox.getCorner());
    QScriptValue topFarLeft = vec3toScriptValue(engine, aaBox.calcTopFarLeft());
    QScriptValue center = vec3toScriptValue(engine, aaBox.calcCenter());
    QScriptValue boundingBoxDimensions = vec3toScriptValue(engine, aaBox.getDimensions());
    boundingBox.setProperty("brn", bottomRightNear);
    boundingBox.setProperty("tfl", topFarLeft);
    boundingBox.setProperty("center", center);
    boundingBox.setProperty("dimensions", boundingBoxDimensions);
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(boundingBox, boundingBox); // gettable, but not settable

    QString textureNamesList = _textureNames.join(",\n");
    COPY_PROPERTY_TO_QSCRIPTVALUE_GETTER(originalTextures, textureNamesList); // gettable, but not settable

    return properties;
}

void EntityItemProperties::copyFromScriptValue(const QScriptValue& object) {


    QScriptValue typeScriptValue = object.property("type");
    if (typeScriptValue.isValid()) {
        setType(typeScriptValue.toVariant().toString());
    }

    COPY_PROPERTY_FROM_QSCRIPTVALUE_VEC3(position, setPosition);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_VEC3(dimensions, setDimensions);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_QUAT(rotation, setRotation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(mass, setMass);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_VEC3(velocity, setVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_VEC3(gravity, setGravity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(damping, setDamping);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(lifetime, setLifetime);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_STRING(script, setScript);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_VEC3(registrationPoint, setRegistrationPoint);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_VEC3(angularVelocity, setAngularVelocity);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(angularDamping, setAngularDamping);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_BOOL(visible, setVisible);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_COLOR(color, setColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_STRING(modelURL, setModelURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_STRING(animationURL, setAnimationURL);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_BOOL(animationIsPlaying, setAnimationIsPlaying);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(animationFPS, setAnimationFPS);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(animationFrameIndex, setAnimationFrameIndex);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_STRING(animationSettings, setAnimationSettings);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(glowLevel, setGlowLevel);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(localRenderAlpha, setLocalRenderAlpha);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_BOOL(ignoreForCollisions, setIgnoreForCollisions);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_BOOL(collisionsWillMove, setCollisionsWillMove);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_BOOL(isSpotlight, setIsSpotlight);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_COLOR(diffuseColor, setDiffuseColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_COLOR(ambientColor, setAmbientColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_COLOR(specularColor, setSpecularColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(constantAttenuation, setConstantAttenuation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(linearAttenuation, setLinearAttenuation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(quadraticAttenuation, setQuadraticAttenuation);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(exponent, setExponent);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(cutoff, setCutoff);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_BOOL(locked, setLocked);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_STRING(textures, setTextures);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_STRING(userData, setUserData);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_STRING(text, setText);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_FLOAT(lineHeight, setLineHeight);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_COLOR(textColor, setTextColor);
    COPY_PROPERTY_FROM_QSCRIPTVALUE_COLOR(backgroundColor, setBackgroundColor);

    _lastEdited = usecTimestampNow();
}

QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties) {
    return properties.copyToScriptValue(engine);
}

void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties) {
    properties.copyFromScriptValue(object);
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
        unsigned char* bufferOut, int sizeIn, int& sizeOut) {
    OctreePacketData ourDataPacket(false, sizeIn); // create a packetData object to add out packet details too.
    OctreePacketData* packetData = &ourDataPacket; // we want a pointer to this so we can use our APPEND_ENTITY_PROPERTY macro

    bool success = true; // assume the best
    OctreeElement::AppendState appendState = OctreeElement::COMPLETED; // assume the best
    sizeOut = 0;

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
        bool isNewEntityItem = (id.id == NEW_ENTITY);

        // id
        // encode our ID as a byte count coded byte stream
        QByteArray encodedID = id.id.toRfc4122(); // NUM_BYTES_RFC4122_UUID

        // encode our ID as a byte count coded byte stream
        ByteCountCoded<quint32> tokenCoder;
        QByteArray encodedToken;

        // special case for handling "new" modelItems
        if (isNewEntityItem) {
            // encode our creator token as a byte count coded byte stream
            tokenCoder = id.creatorTokenID;
            encodedToken = tokenCoder;
        }

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

        bool successIDFits = packetData->appendValue(encodedID);
        if (isNewEntityItem && successIDFits) {
            successIDFits = packetData->appendValue(encodedToken);
        }
        bool successTypeFits = packetData->appendValue(encodedType);

        // NOTE: We intentionally do not send "created" times in edit messages. This is because:
        //   1) if the edit is to an existing entity, the created time can not be changed
        //   2) if the edit is to a new entity, the created time is the last edited time

        // TODO: Should we get rid of this in this in edit packets, since this has to always be 0?
        bool successLastUpdatedFits = packetData->appendValue(encodedUpdateDelta);
    
        int propertyFlagsOffset = packetData->getUncompressedByteOffset();
        QByteArray encodedPropertyFlags = propertyFlags;
        int oldPropertyFlagsLength = encodedPropertyFlags.length();
        bool successPropertyFlagsFits = packetData->appendValue(encodedPropertyFlags);
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

            APPEND_ENTITY_PROPERTY(PROP_POSITION, appendPosition, properties.getPosition());
            APPEND_ENTITY_PROPERTY(PROP_DIMENSIONS, appendValue, properties.getDimensions()); // NOTE: PROP_RADIUS obsolete
            APPEND_ENTITY_PROPERTY(PROP_ROTATION, appendValue, properties.getRotation());
            APPEND_ENTITY_PROPERTY(PROP_MASS, appendValue, properties.getMass());
            APPEND_ENTITY_PROPERTY(PROP_VELOCITY, appendValue, properties.getVelocity());
            APPEND_ENTITY_PROPERTY(PROP_GRAVITY, appendValue, properties.getGravity());
            APPEND_ENTITY_PROPERTY(PROP_DAMPING, appendValue, properties.getDamping());
            APPEND_ENTITY_PROPERTY(PROP_LIFETIME, appendValue, properties.getLifetime());
            APPEND_ENTITY_PROPERTY(PROP_SCRIPT, appendValue, properties.getScript());
            APPEND_ENTITY_PROPERTY(PROP_COLOR, appendColor, properties.getColor());
            APPEND_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, appendValue, properties.getRegistrationPoint());
            APPEND_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, appendValue, properties.getAngularVelocity());
            APPEND_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, appendValue, properties.getAngularDamping());
            APPEND_ENTITY_PROPERTY(PROP_VISIBLE, appendValue, properties.getVisible());
            APPEND_ENTITY_PROPERTY(PROP_IGNORE_FOR_COLLISIONS, appendValue, properties.getIgnoreForCollisions());
            APPEND_ENTITY_PROPERTY(PROP_COLLISIONS_WILL_MOVE, appendValue, properties.getCollisionsWillMove());
            APPEND_ENTITY_PROPERTY(PROP_LOCKED, appendValue, properties.getLocked());
            APPEND_ENTITY_PROPERTY(PROP_USER_DATA, appendValue, properties.getUserData());
            
            if (properties.getType() == EntityTypes::Text) {
                APPEND_ENTITY_PROPERTY(PROP_TEXT, appendValue, properties.getText());
                APPEND_ENTITY_PROPERTY(PROP_LINE_HEIGHT, appendValue, properties.getLineHeight());
                APPEND_ENTITY_PROPERTY(PROP_TEXT_COLOR, appendColor, properties.getTextColor());
                APPEND_ENTITY_PROPERTY(PROP_BACKGROUND_COLOR, appendColor, properties.getBackgroundColor());
            }
            
            if (properties.getType() == EntityTypes::Model) {
                APPEND_ENTITY_PROPERTY(PROP_MODEL_URL, appendValue, properties.getModelURL());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_URL, appendValue, properties.getAnimationURL());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FPS, appendValue, properties.getAnimationFPS());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_FRAME_INDEX, appendValue, properties.getAnimationFrameIndex());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_PLAYING, appendValue, properties.getAnimationIsPlaying());
                APPEND_ENTITY_PROPERTY(PROP_TEXTURES, appendValue, properties.getTextures());
                APPEND_ENTITY_PROPERTY(PROP_ANIMATION_SETTINGS, appendValue, properties.getAnimationSettings());
            }

            if (properties.getType() == EntityTypes::Light) {
                APPEND_ENTITY_PROPERTY(PROP_IS_SPOTLIGHT, appendValue, properties.getIsSpotlight());
                APPEND_ENTITY_PROPERTY(PROP_DIFFUSE_COLOR, appendColor, properties.getDiffuseColor());
                APPEND_ENTITY_PROPERTY(PROP_AMBIENT_COLOR, appendColor, properties.getAmbientColor());
                APPEND_ENTITY_PROPERTY(PROP_SPECULAR_COLOR, appendColor, properties.getSpecularColor());
                APPEND_ENTITY_PROPERTY(PROP_CONSTANT_ATTENUATION, appendValue, properties.getConstantAttenuation());
                APPEND_ENTITY_PROPERTY(PROP_LINEAR_ATTENUATION, appendValue, properties.getLinearAttenuation());
                APPEND_ENTITY_PROPERTY(PROP_QUADRATIC_ATTENUATION, appendValue, properties.getQuadraticAttenuation());
                APPEND_ENTITY_PROPERTY(PROP_EXPONENT, appendValue, properties.getExponent());
                APPEND_ENTITY_PROPERTY(PROP_CUTOFF, appendValue, properties.getCutoff());
            }
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
        const unsigned char* finalizedData = packetData->getFinalizedData();
        int  finalizedSize = packetData->getFinalizedSize();
        if (finalizedSize <= sizeIn) {
            memcpy(bufferOut, finalizedData, finalizedSize);
            sizeOut = finalizedSize;
        } else {
            qDebug() << "ERROR - encoded edit message doesn't fit in output buffer.";
            sizeOut = 0;
            success = false;
        }
    } else {
        packetData->discardSubTree();
        sizeOut = 0;
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
    QByteArray encodedID((const char*)dataAt, NUM_BYTES_RFC4122_UUID); // maximum possible size
    QUuid editID = QUuid::fromRfc4122(encodedID);
    dataAt += encodedID.size();
    processedBytes += encodedID.size();

    bool isNewEntityItem = (editID == NEW_ENTITY);

    if (isNewEntityItem) {
        // If this is a NEW_ENTITY, then we assume that there's an additional uint32_t creatorToken, that
        // we want to send back to the creator as an map to the actual id

        QByteArray encodedToken((const char*)dataAt, (bytesToRead - processedBytes));
        ByteCountCoded<quint32> tokenCoder = encodedToken;
        quint32 creatorTokenID = tokenCoder;
        encodedToken = tokenCoder; // determine true bytesToRead
        dataAt += encodedToken.size();
        processedBytes += encodedToken.size();

        //newEntityItem.setCreatorTokenID(creatorTokenID);
        //newEntityItem._newlyCreated = true;
        
        entityID.id = NEW_ENTITY;
        entityID.creatorTokenID = creatorTokenID;
        entityID.isKnownID = false;

        valid = true;

        // created time is lastEdited time
        properties.setCreated(lastEdited);
    } else {
        entityID.id = editID;
        entityID.creatorTokenID = UNKNOWN_ENTITY_TOKEN;
        entityID.isKnownID = true;
        valid = true;

        // created time is lastEdited time
        properties.setCreated(USE_EXISTING_CREATED_TIME);
    }

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

    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_POSITION, glm::vec3, setPosition);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DIMENSIONS, glm::vec3, setDimensions);  // NOTE: PROP_RADIUS obsolete
    READ_ENTITY_PROPERTY_QUAT_TO_PROPERTIES(PROP_ROTATION, setRotation);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_MASS, float, setMass);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VELOCITY, glm::vec3, setVelocity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_GRAVITY, glm::vec3, setGravity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_DAMPING, float, setDamping);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LIFETIME, float, setLifetime);
    READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(PROP_SCRIPT,setScript);
    READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(PROP_COLOR, setColor);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_REGISTRATION_POINT, glm::vec3, setRegistrationPoint);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANGULAR_VELOCITY, glm::vec3, setAngularVelocity);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANGULAR_DAMPING, float, setAngularDamping);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_VISIBLE, bool, setVisible);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_IGNORE_FOR_COLLISIONS, bool, setIgnoreForCollisions);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_COLLISIONS_WILL_MOVE, bool, setCollisionsWillMove);
    READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LOCKED, bool, setLocked);
    READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(PROP_USER_DATA, setUserData);

    if (properties.getType() == EntityTypes::Text) {
        READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(PROP_TEXT, setText);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINE_HEIGHT, float, setLineHeight);
        READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(PROP_TEXT_COLOR, setTextColor);
        READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(PROP_BACKGROUND_COLOR, setBackgroundColor);
    }
    
    if (properties.getType() == EntityTypes::Model) {
        READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(PROP_MODEL_URL, setModelURL);
        READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(PROP_ANIMATION_URL, setAnimationURL);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_FPS, float, setAnimationFPS);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_FRAME_INDEX, float, setAnimationFrameIndex);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_ANIMATION_PLAYING, bool, setAnimationIsPlaying);
        READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(PROP_TEXTURES, setTextures);
        READ_ENTITY_PROPERTY_STRING_TO_PROPERTIES(PROP_ANIMATION_SETTINGS, setAnimationSettings);
    }
    
    if (properties.getType() == EntityTypes::Light) {
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_IS_SPOTLIGHT, bool, setIsSpotlight);
        READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(PROP_DIFFUSE_COLOR, setDiffuseColor);
        READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(PROP_AMBIENT_COLOR, setAmbientColor);
        READ_ENTITY_PROPERTY_COLOR_TO_PROPERTIES(PROP_SPECULAR_COLOR, setSpecularColor);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CONSTANT_ATTENUATION, float, setConstantAttenuation);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_LINEAR_ATTENUATION, float, setLinearAttenuation);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_QUADRATIC_ATTENUATION, float, setQuadraticAttenuation);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_EXPONENT, float, setExponent);
        READ_ENTITY_PROPERTY_TO_PROPERTIES(PROP_CUTOFF, float, setCutoff);
    }
    
    return valid;
}


// NOTE: This version will only encode the portion of the edit message immediately following the
// header it does not include the send times and sequence number because that is handled by the 
// edit packet sender...
bool EntityItemProperties::encodeEraseEntityMessage(const EntityItemID& entityItemID, 
                                            unsigned char* outputBuffer, size_t maxLength, size_t& outputLength) {

    unsigned char* copyAt = outputBuffer;
    uint16_t numberOfIds = 1; // only one entity ID in this message

    if (maxLength < sizeof(numberOfIds) + NUM_BYTES_RFC4122_UUID) {
        qDebug() << "ERROR - encodeEraseEntityMessage() called with buffer that is too small!";
        outputLength = 0;
        return false;
    }
    memcpy(copyAt, &numberOfIds, sizeof(numberOfIds));
    copyAt += sizeof(numberOfIds);
    outputLength = sizeof(numberOfIds);

    QUuid entityID = entityItemID.id;                
    QByteArray encodedEntityID = entityID.toRfc4122();

    memcpy(copyAt, encodedEntityID.constData(), NUM_BYTES_RFC4122_UUID);
    copyAt += NUM_BYTES_RFC4122_UUID;
    outputLength += NUM_BYTES_RFC4122_UUID;

    return true;
}

void EntityItemProperties::markAllChanged() {
    _positionChanged = true;
    _dimensionsChanged = true;
    _rotationChanged = true;
    _massChanged = true;
    _velocityChanged = true;
    _gravityChanged = true;
    _dampingChanged = true;
    _lifetimeChanged = true;
    _userDataChanged = true;
    _scriptChanged = true;
    _registrationPointChanged = true;
    _angularVelocityChanged = true;
    _angularDampingChanged = true;
    _visibleChanged = true;
    _colorChanged = true;
    _modelURLChanged = true;
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

    _diffuseColorChanged = true;
    _ambientColorChanged = true;
    _specularColorChanged = true;
    _constantAttenuationChanged = true;
    _linearAttenuationChanged = true; 
    _quadraticAttenuationChanged = true;
    _exponentChanged = true;
    _cutoffChanged = true;
    _lockedChanged = true;
    _texturesChanged = true;
    
    _textChanged = true;
    _lineHeightChanged = true;
    _textColorChanged = true;
    _backgroundColorChanged = true;
}

AACube EntityItemProperties::getMaximumAACubeInTreeUnits() const {
    AACube maxCube = getMaximumAACubeInMeters();
    maxCube.scale(1.0f / (float)TREE_SCALE);
    return maxCube;
}

/// The maximum bounding cube for the entity, independent of it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
/// 
AACube EntityItemProperties::getMaximumAACubeInMeters() const { 
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
AABox EntityItemProperties::getAABoxInMeters() const { 

    // _position represents the position of the registration point.
    glm::vec3 registrationRemainder = glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint;
    
    glm::vec3 unrotatedMinRelativeToEntity = glm::vec3(0.0f, 0.0f, 0.0f) - (_dimensions * _registrationPoint);
    glm::vec3 unrotatedMaxRelativeToEntity = _dimensions * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(getRotation());
    
    // shift the extents to be relative to the position/registration point
    rotatedExtentsRelativeToRegistrationPoint.shiftBy(_position);
    
    return AABox(rotatedExtentsRelativeToRegistrationPoint);
}
