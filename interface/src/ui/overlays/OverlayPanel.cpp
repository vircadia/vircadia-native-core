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
    if (property == "anchorPosition") {
        return vec3toScriptValue(_scriptEngine, getAnchorPosition());
    }
    if (property == "anchorPositionBinding") {
        return propertyBindingToScriptValue(_scriptEngine,
                                            PropertyBinding(_anchorPositionBindMyAvatar ?
                                                            "MyAvatar" : "",
                                                            _anchorPositionBindEntity));
    }
    if (property == "anchorRotation") {
        return quatToScriptValue(_scriptEngine, getAnchorRotation());
    }
    if (property == "anchorRotationBinding") {
        return propertyBindingToScriptValue(_scriptEngine,
                                            PropertyBinding(_anchorRotationBindMyAvatar ?
                                                            "MyAvatar" : "",
                                                            _anchorRotationBindEntity));
    }
    if (property == "anchorScale") {
        return vec3toScriptValue(_scriptEngine, getAnchorScale());
    }
    if (property == "visible") {
        return getVisible();
    }
    if (property == "children") {
        QScriptValue array = _scriptEngine->newArray(_children.length());
        for (int i = 0; i < _children.length(); i++) {
            array.setProperty(i, _children[i]);
        }
        return array;
    }

    QScriptValue value = Billboardable::getProperty(_scriptEngine, property);
    if (value.isValid()) {
        return value;
    }
    return PanelAttachable::getProperty(_scriptEngine, property);
}

void OverlayPanel::setProperties(const QScriptValue &properties) {
    PanelAttachable::setProperties(properties);
    Billboardable::setProperties(properties);

    QScriptValue anchorPosition = properties.property("anchorPosition");
    if (anchorPosition.isValid() &&
        anchorPosition.property("x").isValid() &&
        anchorPosition.property("y").isValid() &&
        anchorPosition.property("z").isValid()) {
        glm::vec3 newPosition;
        vec3FromScriptValue(anchorPosition, newPosition);
        setAnchorPosition(newPosition);
    }

    QScriptValue anchorPositionBinding = properties.property("anchorPositionBinding");
    if (anchorPositionBinding.isValid()) {
        PropertyBinding binding = {};
        propertyBindingFromScriptValue(anchorPositionBinding, binding);
        _anchorPositionBindMyAvatar = binding.avatar == "MyAvatar";
        _anchorPositionBindEntity = binding.entity;
    }

    QScriptValue anchorRotation = properties.property("anchorRotation");
    if (anchorRotation.isValid() &&
        anchorRotation.property("x").isValid() &&
        anchorRotation.property("y").isValid() &&
        anchorRotation.property("z").isValid() &&
        anchorRotation.property("w").isValid()) {
        glm::quat newRotation;
        quatFromScriptValue(anchorRotation, newRotation);
        setAnchorRotation(newRotation);
    }

    QScriptValue anchorRotationBinding = properties.property("anchorRotationBinding");
    if (anchorRotationBinding.isValid()) {
        PropertyBinding binding = {};
        propertyBindingFromScriptValue(anchorPositionBinding, binding);
        _anchorRotationBindMyAvatar = binding.avatar == "MyAvatar";
        _anchorRotationBindEntity = binding.entity;
    }

    QScriptValue anchorScale = properties.property("anchorScale");
    if (anchorScale.isValid()) {
        if (anchorScale.property("x").isValid() &&
            anchorScale.property("y").isValid() &&
            anchorScale.property("z").isValid()) {
            glm::vec3 newScale;
            vec3FromScriptValue(anchorScale, newScale);
            setAnchorScale(newScale);
        } else {
            setAnchorScale(anchorScale.toVariant().toFloat());
        }
    }

    QScriptValue visible = properties.property("visible");
    if (visible.isValid()) {
        setVisible(visible.toVariant().toBool());
    }
}

void OverlayPanel::applyTransformTo(Transform& transform, bool force) {
    if (force || usecTimestampNow() > _transformExpiry) {
        PanelAttachable::applyTransformTo(transform, true);
        if (!getParentPanel()) {
            if (_anchorPositionBindMyAvatar) {
                transform.setTranslation(DependencyManager::get<AvatarManager>()->getMyAvatar()
                                         ->getPosition());
            } else if (!_anchorPositionBindEntity.isNull()) {
                EntityTreePointer entityTree = DependencyManager::get<EntityScriptingInterface>()->getEntityTree();
                entityTree->withReadLock([&] {
                    EntityItemPointer foundEntity = entityTree->findEntityByID(_anchorPositionBindEntity);
                    if (foundEntity) {
                        transform.setTranslation(foundEntity->getPosition());
                    }
                });
            } else {
                transform.setTranslation(getAnchorPosition());
            }

            if (_anchorRotationBindMyAvatar) {
                transform.setRotation(DependencyManager::get<AvatarManager>()->getMyAvatar()
                                      ->getOrientation());
            } else if (!_anchorRotationBindEntity.isNull()) {
                EntityTreePointer entityTree = DependencyManager::get<EntityScriptingInterface>()->getEntityTree();
                entityTree->withReadLock([&] {
                    EntityItemPointer foundEntity = entityTree->findEntityByID(_anchorRotationBindEntity);
                    if (foundEntity) {
                        transform.setRotation(foundEntity->getRotation());
                    }
                });
            } else {
                transform.setRotation(getAnchorRotation());
            }

            transform.setScale(getAnchorScale());

            transform.postTranslate(getOffsetPosition());
            transform.postRotate(getOffsetRotation());
            transform.postScale(getOffsetScale());
        }
        pointTransformAtCamera(transform, getOffsetRotation());
    }
}
