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

#include <RegisteredMetaTypes.h>

#include "EntityItemProperties.h"

EntityItemProperties::EntityItemProperties() :

    _id(UNKNOWN_ENTITY_ID),
    _idSet(false),
    _lastEdited(0),
    _type(EntityTypes::Base),

    _position(0),
    _radius(ENTITY_DEFAULT_RADIUS),
    _rotation(ENTITY_DEFAULT_ROTATION),
    _shouldBeDeleted(false),

    _positionChanged(false),
    _radiusChanged(false),
    _rotationChanged(false),
    _shouldBeDeletedChanged(false),


    _color(),
    _modelURL(""),
    _animationURL(""),
    _animationIsPlaying(false),
    _animationFrameIndex(0.0),
    _animationFPS(ENTITY_DEFAULT_ANIMATION_FPS),
    _glowLevel(0.0f),

    _colorChanged(false),
    _modelURLChanged(false),
    _animationURLChanged(false),
    _animationIsPlayingChanged(false),
    _animationFrameIndexChanged(false),
    _animationFPSChanged(false),
    _glowLevelChanged(false),

    _defaultSettings(true)
{
}

void EntityItemProperties::debugDump() const {
    qDebug() << "EntityItemProperties...";
    qDebug() << "   _id=" << _id;
    qDebug() << "   _idSet=" << _idSet;
    qDebug() << "   _position=" << _position.x << "," << _position.y << "," << _position.z;
    qDebug() << "   _radius=" << _radius;
}

EntityPropertyFlags EntityItemProperties::getChangedProperties() const {
    EntityPropertyFlags changedProperties;
    if (_radiusChanged) {
        changedProperties += PROP_RADIUS;
    }

    if (_positionChanged) {
        changedProperties += PROP_POSITION;
    }

    if (_rotationChanged) {
        changedProperties += PROP_ROTATION;
    }

    if (_shouldBeDeletedChanged) {
        changedProperties += PROP_SHOULD_BE_DELETED;
    }

    if (_colorChanged) {
        changedProperties += PROP_COLOR;
    }

    if (_modelURLChanged) {
        changedProperties += PROP_MODEL_URL;
    }

    if (_animationURLChanged) {
        changedProperties += PROP_ANIMATION_URL;
    }

    if (_animationIsPlayingChanged) {
        changedProperties += PROP_ANIMATION_PLAYING;
    }

    if (_animationFrameIndexChanged) {
        changedProperties += PROP_ANIMATION_FRAME_INDEX;
    }

    if (_animationFPSChanged) {
        changedProperties += PROP_ANIMATION_FPS;
    }

    return changedProperties;
}

QScriptValue EntityItemProperties::copyToScriptValue(QScriptEngine* engine) const {
    QScriptValue properties = engine->newObject();

    if (_idSet) {
        properties.setProperty("id", _id);
        bool isKnownID = (_id != UNKNOWN_ENTITY_ID);
        properties.setProperty("isKnownID", isKnownID);
qDebug() << "EntityItemProperties::copyToScriptValue()... isKnownID=" << isKnownID << "id=" << _id;
    }

    QScriptValue position = vec3toScriptValue(engine, _position);
    properties.setProperty("position", position);
    properties.setProperty("radius", _radius);
    QScriptValue rotation = quatToScriptValue(engine, _rotation);
    properties.setProperty("rotation", rotation);
    properties.setProperty("shouldBeDeleted", _shouldBeDeleted);

    QScriptValue color = xColorToScriptValue(engine, _color);
    properties.setProperty("color", color);
    properties.setProperty("modelURL", _modelURL);
    
//qDebug() << "EntityItemProperties::copyToScriptValue().... _modelURL=" << _modelURL;    
    
    properties.setProperty("animationURL", _animationURL);
    properties.setProperty("animationIsPlaying", _animationIsPlaying);
    properties.setProperty("animationFrameIndex", _animationFrameIndex);
    properties.setProperty("animationFPS", _animationFPS);
    properties.setProperty("glowLevel", _glowLevel);

    // Sitting properties support
    QScriptValue sittingPoints = engine->newObject();
    for (int i = 0; i < _sittingPoints.size(); ++i) {
        QScriptValue sittingPoint = engine->newObject();
        sittingPoint.setProperty("name", _sittingPoints[i].name);
        sittingPoint.setProperty("position", vec3toScriptValue(engine, _sittingPoints[i].position));
        sittingPoint.setProperty("rotation", quatToScriptValue(engine, _sittingPoints[i].rotation));
        sittingPoints.setProperty(i, sittingPoint);
    }
    sittingPoints.setProperty("length", _sittingPoints.size());
    properties.setProperty("sittingPoints", sittingPoints);

    return properties;
}

void EntityItemProperties::copyFromScriptValue(const QScriptValue& object) {

    QScriptValue typeScriptValue = object.property("type");
    if (typeScriptValue.isValid()) {
        QString typeName;
        typeName = typeScriptValue.toVariant().toString();
        _type = EntityTypes::getEntityTypeFromName(typeName);
    }

    QScriptValue position = object.property("position");
    if (position.isValid()) {
        QScriptValue x = position.property("x");
        QScriptValue y = position.property("y");
        QScriptValue z = position.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            if (_defaultSettings || newPosition != _position) {
                _position = newPosition;
                _positionChanged = true;
            }
        }
    }

    QScriptValue radius = object.property("radius");
    if (radius.isValid()) {
        float newRadius;
        newRadius = radius.toVariant().toFloat();
        if (_defaultSettings || newRadius != _radius) {
            _radius = newRadius;
            _radiusChanged = true;
        }
    }

    QScriptValue rotation = object.property("rotation");
    if (rotation.isValid()) {
        QScriptValue x = rotation.property("x");
        QScriptValue y = rotation.property("y");
        QScriptValue z = rotation.property("z");
        QScriptValue w = rotation.property("w");
        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            glm::quat newRotation;
            newRotation.x = x.toVariant().toFloat();
            newRotation.y = y.toVariant().toFloat();
            newRotation.z = z.toVariant().toFloat();
            newRotation.w = w.toVariant().toFloat();
            if (_defaultSettings || newRotation != _rotation) {
                _rotation = newRotation;
                _rotationChanged = true;
            }
        }
    }

    QScriptValue shouldBeDeleted = object.property("shouldBeDeleted");
    if (shouldBeDeleted.isValid()) {
        bool newShouldBeDeleted;
        newShouldBeDeleted = shouldBeDeleted.toVariant().toBool();
        if (_defaultSettings || newShouldBeDeleted != _shouldBeDeleted) {
            _shouldBeDeleted = newShouldBeDeleted;
            _shouldBeDeletedChanged = true;
        }
    }

    QScriptValue color = object.property("color");
    if (color.isValid()) {
        QScriptValue red = color.property("red");
        QScriptValue green = color.property("green");
        QScriptValue blue = color.property("blue");
        if (red.isValid() && green.isValid() && blue.isValid()) {
            xColor newColor;
            newColor.red = red.toVariant().toInt();
            newColor.green = green.toVariant().toInt();
            newColor.blue = blue.toVariant().toInt();
            if (_defaultSettings || (newColor.red != _color.red ||
                newColor.green != _color.green ||
                newColor.blue != _color.blue)) {
                _color = newColor;
                _colorChanged = true;
            }
        }
    }

    QScriptValue modelURL = object.property("modelURL");
    if (modelURL.isValid()) {
        QString newModelURL;
        newModelURL = modelURL.toVariant().toString();
        if (_defaultSettings || newModelURL != _modelURL) {
            _modelURL = newModelURL;
            _modelURLChanged = true;
        }
    }
    
qDebug() << "EntityItemProperties::copyFromScriptValue().... _modelURL=" << _modelURL;    

    QScriptValue animationURL = object.property("animationURL");
    if (animationURL.isValid()) {
        QString newAnimationURL;
        newAnimationURL = animationURL.toVariant().toString();
        if (_defaultSettings || newAnimationURL != _animationURL) {
            _animationURL = newAnimationURL;
            _animationURLChanged = true;
        }
    }

    QScriptValue animationIsPlaying = object.property("animationIsPlaying");
    if (animationIsPlaying.isValid()) {
        bool newIsAnimationPlaying;
        newIsAnimationPlaying = animationIsPlaying.toVariant().toBool();
        if (_defaultSettings || newIsAnimationPlaying != _animationIsPlaying) {
            _animationIsPlaying = newIsAnimationPlaying;
            _animationIsPlayingChanged = true;
        }
    }
    
    QScriptValue animationFrameIndex = object.property("animationFrameIndex");
    if (animationFrameIndex.isValid()) {
        float newFrameIndex;
        newFrameIndex = animationFrameIndex.toVariant().toFloat();
        if (_defaultSettings || newFrameIndex != _animationFrameIndex) {
            _animationFrameIndex = newFrameIndex;
            _animationFrameIndexChanged = true;
        }
    }
    
    QScriptValue animationFPS = object.property("animationFPS");
    if (animationFPS.isValid()) {
        float newFPS;
        newFPS = animationFPS.toVariant().toFloat();
        if (_defaultSettings || newFPS != _animationFPS) {
            _animationFPS = newFPS;
            _animationFPSChanged = true;
        }
    }
    
    QScriptValue glowLevel = object.property("glowLevel");
    if (glowLevel.isValid()) {
        float newGlowLevel;
        newGlowLevel = glowLevel.toVariant().toFloat();
        if (_defaultSettings || newGlowLevel != _glowLevel) {
            _glowLevel = newGlowLevel;
            _glowLevelChanged = true;
        }
    }

    _lastEdited = usecTimestampNow();
}

QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties) {
    return properties.copyToScriptValue(engine);
}

void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties) {
    properties.copyFromScriptValue(object);
}
