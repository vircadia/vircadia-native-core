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

#if OVERLAY_PANELS

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

QVariant propertyBindingToVariant(const PropertyBinding& value) {
    QVariantMap obj;

    if (value.avatar == "MyAvatar") {
        obj["avatar"] = "MyAvatar";
    } else if (!value.entity.isNull()) {
        obj["entity"] = value.entity;
    }

    return obj;
}

void propertyBindingFromVariant(const QVariant& objectVar, PropertyBinding& value) {
    auto object = objectVar.toMap();
    auto avatar = object["avatar"];
    auto entity = object["entity"];

    if (avatar.isValid() && !avatar.isNull()) {
        value.avatar = avatar.toString();
    } else if (entity.isValid() && !entity.isNull()) {
        value.entity = entity.toUuid();
    }
}


void OverlayPanel::addChild(OverlayID childId) {
    if (!_children.contains(childId)) {
        _children.append(childId);
    }
}

void OverlayPanel::removeChild(OverlayID childId) {
    if (_children.contains(childId)) {
        _children.removeOne(childId);
    }
}

QVariant OverlayPanel::getProperty(const QString &property) {
    if (property == "anchorPosition") {
        return vec3toVariant(getAnchorPosition());
    }
    if (property == "anchorPositionBinding") {
        return propertyBindingToVariant(PropertyBinding(_anchorPositionBindMyAvatar ?
                                                            "MyAvatar" : "",
                                                            _anchorPositionBindEntity));
    }
    if (property == "anchorRotation") {
        return quatToVariant(getAnchorRotation());
    }
    if (property == "anchorRotationBinding") {
        return propertyBindingToVariant(PropertyBinding(_anchorRotationBindMyAvatar ?
                                                            "MyAvatar" : "",
                                                            _anchorRotationBindEntity));
    }
    if (property == "anchorScale") {
        return vec3toVariant(getAnchorScale());
    }
    if (property == "visible") {
        return getVisible();
    }
    if (property == "children") {
        QVariantList array;
        for (int i = 0; i < _children.length(); i++) {
            array.append(OverlayIDtoScriptValue(nullptr, _children[i]).toVariant());
        }
        return array;
    }

    auto value = Billboardable::getProperty(property);
    if (value.isValid()) {
        return value;
    }
    return PanelAttachable::getProperty(property);
}

void OverlayPanel::setProperties(const QVariantMap& properties) {
    PanelAttachable::setProperties(properties);
    Billboardable::setProperties(properties);

    auto anchorPosition = properties["anchorPosition"];
    if (anchorPosition.isValid()) {
        setAnchorPosition(vec3FromVariant(anchorPosition));
    }

    auto anchorPositionBinding = properties["anchorPositionBinding"];
    if (anchorPositionBinding.isValid()) {
        PropertyBinding binding = {};
        propertyBindingFromVariant(anchorPositionBinding, binding);
        _anchorPositionBindMyAvatar = binding.avatar == "MyAvatar";
        _anchorPositionBindEntity = binding.entity;
    }

    auto anchorRotation = properties["anchorRotation"];
    if (anchorRotation.isValid()) {
        setAnchorRotation(quatFromVariant(anchorRotation));
    }

    auto anchorRotationBinding = properties["anchorRotationBinding"];
    if (anchorRotationBinding.isValid()) {
        PropertyBinding binding = {};
        propertyBindingFromVariant(anchorPositionBinding, binding);
        _anchorRotationBindMyAvatar = binding.avatar == "MyAvatar";
        _anchorRotationBindEntity = binding.entity;
    }

    auto anchorScale = properties["anchorScale"];
    if (anchorScale.isValid()) {
        setAnchorScale(vec3FromVariant(anchorScale));
    }

    auto visible = properties["visible"];
    if (visible.isValid()) {
        setVisible(visible.toBool());
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
#endif