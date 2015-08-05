//
//  FloatingUIPanel.cpp
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/2/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FloatingUIPanel.h"

#include <QVariant>
#include <RegisteredMetaTypes.h>
#include <DependencyManager.h>

#include "avatar/AvatarManager.h"
#include "avatar/MyAvatar.h"
#include "Application.h"
#include "Base3DOverlay.h"

glm::vec3 FloatingUIPanel::getComputedAnchorPosition() const {
    glm::vec3 pos = {};
    if (getAttachedPanel()) {
        pos = getAttachedPanel()->getPosition();
    } else if (_anchorPositionBindMyAvatar) {
        pos = DependencyManager::get<AvatarManager>()->getMyAvatar()->getPosition();
    }
    return pos + getAnchorPosition();
}

glm::quat FloatingUIPanel::getComputedOffsetRotation() const {
    glm::quat rot = {1, 0, 0, 0};
    if (getAttachedPanel()) {
        rot = getAttachedPanel()->getRotation();
    } else if (_offsetRotationBindMyAvatar) {
        rot = DependencyManager::get<AvatarManager>()->getMyAvatar()->getOrientation() *
              glm::angleAxis(glm::pi<float>(), IDENTITY_UP);
    }
    return rot * getOffsetRotation();
}

glm::vec3 FloatingUIPanel::getPosition() const {
    return getComputedOffsetRotation() * getOffsetPosition() + getComputedAnchorPosition();
}

glm::quat FloatingUIPanel::getRotation() const {
    return getComputedOffsetRotation() * getFacingRotation();
}

void FloatingUIPanel::addChild(unsigned int childId) {
    if (!_children.contains(childId)) {
        _children.append(childId);
    }
}

void FloatingUIPanel::removeChild(unsigned int childId) {
    if (_children.contains(childId)) {
        _children.removeOne(childId);
    }
}

QScriptValue FloatingUIPanel::getProperty(const QString &property) {
    if (property == "anchorPosition") {
        return vec3toScriptValue(_scriptEngine, getAnchorPosition());
    }
    if (property == "anchorPositionBinding") {
        QScriptValue obj = _scriptEngine->newObject();
        if (_anchorPositionBindMyAvatar) {
            obj.setProperty("avatar", "MyAvatar");
        }
        obj.setProperty("computed", vec3toScriptValue(_scriptEngine, getComputedAnchorPosition()));
        return obj;
    }
    if (property == "offsetRotation") {
        return quatToScriptValue(_scriptEngine, getOffsetRotation());
    }
    if (property == "offsetRotationBinding") {
        QScriptValue obj = _scriptEngine->newObject();
        if (_offsetRotationBindMyAvatar) {
            obj.setProperty("avatar", "MyAvatar");
        }
        obj.setProperty("computed", quatToScriptValue(_scriptEngine, getComputedOffsetRotation()));
        return obj;
    }
    if (property == "offsetPosition") {
        return vec3toScriptValue(_scriptEngine, getOffsetPosition());
    }
    if (property == "facingRotation") {
        return quatToScriptValue(_scriptEngine, getFacingRotation());
    }

    return QScriptValue();
}

void FloatingUIPanel::setProperties(const QScriptValue &properties) {
    QScriptValue anchorPosition = properties.property("anchorPosition");
    if (anchorPosition.isValid()) {
        QScriptValue x = anchorPosition.property("x");
        QScriptValue y = anchorPosition.property("y");
        QScriptValue z = anchorPosition.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            setAnchorPosition(newPosition);
        }
    }

    QScriptValue anchorPositionBinding = properties.property("anchorPositionBinding");
    if (anchorPositionBinding.isValid()) {
        _anchorPositionBindMyAvatar = false;

        QScriptValue avatar = anchorPositionBinding.property("avatar");

        if (avatar.isValid()) {
            _anchorPositionBindMyAvatar = (avatar.toVariant().toString() == "MyAvatar");
        }
    }

    QScriptValue offsetRotation = properties.property("offsetRotation");
    if (offsetRotation.isValid()) {
        QScriptValue x = offsetRotation.property("x");
        QScriptValue y = offsetRotation.property("y");
        QScriptValue z = offsetRotation.property("z");
        QScriptValue w = offsetRotation.property("w");

        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            glm::quat newRotation;
            newRotation.x = x.toVariant().toFloat();
            newRotation.y = y.toVariant().toFloat();
            newRotation.z = z.toVariant().toFloat();
            newRotation.w = w.toVariant().toFloat();
            setOffsetRotation(newRotation);
        }
    }

    QScriptValue offsetRotationBinding = properties.property("offsetRotationBinding");
    if (offsetRotationBinding.isValid()) {
        _offsetRotationBindMyAvatar = false;

        QScriptValue avatar = offsetRotationBinding.property("avatar");

        if (avatar.isValid()) {
            _offsetRotationBindMyAvatar = (avatar.toVariant().toString() == "MyAvatar");
        }
    }

    QScriptValue offsetPosition = properties.property("offsetPosition");
    if (offsetPosition.isValid()) {
        QScriptValue x = offsetPosition.property("x");
        QScriptValue y = offsetPosition.property("y");
        QScriptValue z = offsetPosition.property("z");
        if (x.isValid() && y.isValid() && z.isValid()) {
            glm::vec3 newPosition;
            newPosition.x = x.toVariant().toFloat();
            newPosition.y = y.toVariant().toFloat();
            newPosition.z = z.toVariant().toFloat();
            setOffsetPosition(newPosition);
        }
    }

    QScriptValue facingRotation = properties.property("facingRotation");
    if (facingRotation.isValid()) {
        QScriptValue x = facingRotation.property("x");
        QScriptValue y = facingRotation.property("y");
        QScriptValue z = facingRotation.property("z");
        QScriptValue w = facingRotation.property("w");

        if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
            glm::quat newRotation;
            newRotation.x = x.toVariant().toFloat();
            newRotation.y = y.toVariant().toFloat();
            newRotation.z = z.toVariant().toFloat();
            newRotation.w = w.toVariant().toFloat();
            setFacingRotation(newRotation);
        }
    }
}
