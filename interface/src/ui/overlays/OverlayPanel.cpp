//
//  OverlayPanel.cpp
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/2/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OverlayPanel.h"

#include <QVariant>
#include <RegisteredMetaTypes.h>
#include <DependencyManager.h>
#include <EntityScriptingInterface.h>

#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"
#include "Application.h"
#include "Base3DOverlay.h"

PropertyBinding::PropertyBinding(QString avatar, QUuid entity) :
    avatar(avatar),
    entity(entity)
{
}

QScriptValue propertyBindingToScriptValue(QScriptEngine* engine, const PropertyBinding& value) {
    QScriptValue obj = engine->newObject();

    if (value.avatar == "MyAvatar") {
        obj.setProperty("avatar", "MyAvatar");
    } else if (!value.entity.isNull()) {
        obj.setProperty("entity", engine->newVariant(value.entity));
    }

    return obj;
}

void propertyBindingFromScriptValue(const QScriptValue& object, PropertyBinding& value) {
    QScriptValue avatar = object.property("avatar");
    QScriptValue entity = object.property("entity");

    if (avatar.isValid() && !avatar.isNull()) {
        value.avatar = avatar.toVariant().toString();
    } else if (entity.isValid() && !entity.isNull()) {
        value.entity = entity.toVariant().toUuid();
    }
}


void OverlayPanel::addChild(unsigned int childId) {
    if (!_children.contains(childId)) {
        _children.append(childId);
    }
}

void OverlayPanel::removeChild(unsigned int childId) {
    if (_children.contains(childId)) {
        _children.removeOne(childId);
    }
}

QScriptValue OverlayPanel::getProperty(const QString &property) {
    if (property == "position") {
        return vec3toScriptValue(_scriptEngine, getPosition());
    }
    if (property == "positionBinding") {
        return propertyBindingToScriptValue(_scriptEngine, PropertyBinding(_positionBindMyAvatar ?
                                                                           "MyAvatar" : "",
                                                                           _positionBindEntity));
    }
    if (property == "rotation") {
        return quatToScriptValue(_scriptEngine, getRotation());
    }
    if (property == "rotationBinding") {
        return propertyBindingToScriptValue(_scriptEngine, PropertyBinding(_rotationBindMyAvatar ?
                                                                           "MyAvatar" : "",
                                                                           _rotationBindEntity));
    }
    if (property == "scale") {
        return vec3toScriptValue(_scriptEngine, getScale());
    }
    if (property == "visible") {
        return getVisible();
    }
    if (property == "isFacingAvatar") {
        return getIsFacingAvatar();
    }
    if (property == "children") {
        QScriptValue array = _scriptEngine->newArray(_children.length());
        for (int i = 0; i < _children.length(); i++) {
            array.setProperty(i, _children[i]);
        }
        return array;
    }

    return PanelAttachable::getProperty(_scriptEngine, property);
}

void OverlayPanel::setProperties(const QScriptValue &properties) {
    PanelAttachable::setProperties(properties);

    QScriptValue position = properties.property("position");
    if (position.isValid() &&
        position.property("x").isValid() &&
        position.property("y").isValid() &&
        position.property("z").isValid()) {
        glm::vec3 newPosition;
        vec3FromScriptValue(position, newPosition);
        setPosition(newPosition);
    }

    QScriptValue positionBinding = properties.property("positionBinding");
    if (positionBinding.isValid()) {
        PropertyBinding binding = {};
        propertyBindingFromScriptValue(positionBinding, binding);
        _positionBindMyAvatar = binding.avatar == "MyAvatar";
        _positionBindEntity = binding.entity;
    }

    QScriptValue rotation = properties.property("rotation");
    if (rotation.isValid() &&
        rotation.property("x").isValid() &&
        rotation.property("y").isValid() &&
        rotation.property("z").isValid() &&
        rotation.property("w").isValid()) {
        glm::quat newRotation;
        quatFromScriptValue(rotation, newRotation);
        setRotation(newRotation);
    }

    QScriptValue rotationBinding = properties.property("rotationBinding");
    if (rotationBinding.isValid()) {
        PropertyBinding binding = {};
        propertyBindingFromScriptValue(positionBinding, binding);
        _rotationBindMyAvatar = binding.avatar == "MyAvatar";
        _rotationBindEntity = binding.entity;
    }

    QScriptValue scale = properties.property("scale");
    if (scale.isValid()) {
        if (scale.property("x").isValid() &&
            scale.property("y").isValid() &&
            scale.property("z").isValid()) {
            glm::vec3 newScale;
            vec3FromScriptValue(scale, newScale);
            setScale(newScale);
        } else {
            setScale(scale.toVariant().toFloat());
        }
    }

    QScriptValue visible = properties.property("visible");
    if (visible.isValid()) {
        setVisible(visible.toVariant().toBool());
    }

    QScriptValue isFacingAvatar = properties.property("isFacingAvatar");
    if (isFacingAvatar.isValid()) {
        setIsFacingAvatar(isFacingAvatar.toVariant().toBool());
    }
}

void OverlayPanel::applyTransformTo(Transform& transform, bool force) {
    if (force || usecTimestampNow() > _transformExpiry) {
        PanelAttachable::applyTransformTo(transform, true);
        if (!getParentPanel()) {
            updateTransform();
            transform.setTranslation(getPosition());
            transform.setRotation(getRotation());
            transform.setScale(getScale());
            transform.postTranslate(getOffsetPosition());
            transform.postRotate(getOffsetRotation());
            transform.postScale(getOffsetScale());
        }
        if (_isFacingAvatar) {
            glm::vec3 billboardPos = transform.getTranslation();
            glm::vec3 cameraPos = Application::getInstance()->getCamera()->getPosition();
            glm::vec3 look = cameraPos - billboardPos;
            float elevation = -asinf(look.y / glm::length(look));
            float azimuth = atan2f(look.x, look.z);
            glm::quat rotation(glm::vec3(elevation, azimuth, 0.0f));
            transform.setRotation(rotation);
            transform.postRotate(getOffsetRotation());
        }
    }
}

void OverlayPanel::updateTransform() {
    if (_positionBindMyAvatar) {
        setPosition(DependencyManager::get<AvatarManager>()->getMyAvatar()->getPosition());
    } else if (!_positionBindEntity.isNull()) {
        setPosition(DependencyManager::get<EntityScriptingInterface>()->getEntityTree()->
                    findEntityByID(_positionBindEntity)->getPosition());
    }

    if (_rotationBindMyAvatar) {
        setRotation(DependencyManager::get<AvatarManager>()->getMyAvatar()->getOrientation() *
                    glm::angleAxis(glm::pi<float>(), IDENTITY_UP));
    } else if (!_rotationBindEntity.isNull()) {
        setRotation(DependencyManager::get<EntityScriptingInterface>()->getEntityTree()->
                    findEntityByID(_rotationBindEntity)->getRotation());
    }
}
