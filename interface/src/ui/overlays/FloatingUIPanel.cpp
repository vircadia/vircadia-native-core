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

std::function<glm::vec3()> const FloatingUIPanel::AVATAR_POSITION = []() -> glm::vec3 {
    return DependencyManager::get<AvatarManager>()->getMyAvatar()->getPosition();
};

std::function<glm::quat()> const FloatingUIPanel::AVATAR_ORIENTATION = []() -> glm::quat {
    return DependencyManager::get<AvatarManager>()->getMyAvatar()->getOrientation() *
    glm::angleAxis(glm::pi<float>(), IDENTITY_UP);
};

glm::vec3 FloatingUIPanel::getPosition() const {
    return getOffsetRotation() * getOffsetPosition() + getAnchorPosition();
}

glm::quat FloatingUIPanel::getRotation() const {
    return getOffsetRotation() * getFacingRotation();
}

void FloatingUIPanel::setAnchorPosition(const glm::vec3& position) {
    setAnchorPosition([position]() -> glm::vec3 {
        return position;
    });
}

void FloatingUIPanel::setOffsetRotation(const glm::quat& rotation) {
    setOffsetRotation([rotation]() -> glm::quat {
        return rotation;
    });
}

void FloatingUIPanel::setAttachedPanel(unsigned int panelID) {
    if (panelID) {
        attachAnchorToPanel(panelID);
        attachRotationToPanel(panelID);
    }
    _attachedPanel = panelID;
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
    if (property == "offsetRotation") {
        return quatToScriptValue(_scriptEngine, getOffsetRotation());
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
    QScriptValue anchor = properties.property("anchorPosition");
    if (anchor.isValid()) {
        QScriptValue bindType = anchor.property("bind");
        QScriptValue value = anchor.property("value");

        if (bindType.isValid()) {
            QString bindTypeString = bindType.toVariant().toString();
            if (bindTypeString == "myAvatar") {
                setAnchorPosition(AVATAR_POSITION);
            } else if (value.isValid()) {
                if (bindTypeString == "overlay") {
                    Overlay::Pointer overlay = Application::getInstance()->getOverlays()
                        .getOverlay(value.toVariant().toUInt());
                    if (overlay->is3D()) {
                        auto overlay3D = std::static_pointer_cast<Base3DOverlay>(overlay);
                        setAnchorPosition([&overlay3D]() -> glm::vec3 {
                            return overlay3D->getPosition();
                        });
                    }
                } else if (bindTypeString == "panel") {
                    attachAnchorToPanel(value.toVariant().toUInt());
                } else if (bindTypeString == "vec3") {
                    QScriptValue x = value.property("x");
                    QScriptValue y = value.property("y");
                    QScriptValue z = value.property("z");
                    if (x.isValid() && y.isValid() && z.isValid()) {
                        glm::vec3 newPosition;
                        newPosition.x = x.toVariant().toFloat();
                        newPosition.y = y.toVariant().toFloat();
                        newPosition.z = z.toVariant().toFloat();
                        setAnchorPosition(newPosition);
                    }
                }
            }
        }
    }

    QScriptValue offsetRotation = properties.property("offsetRotation");
    if (offsetRotation.isValid()) {
        QScriptValue bindType = offsetRotation.property("bind");
        QScriptValue value = offsetRotation.property("value");

        if (bindType.isValid()) {
            QString bindTypeString = bindType.toVariant().toString();
            if (bindTypeString == "myAvatar") {
                setOffsetRotation(AVATAR_ORIENTATION);
            } else if (value.isValid()) {
                if (bindTypeString == "overlay") {
                    Overlay::Pointer overlay = Application::getInstance()->getOverlays()
                        .getOverlay(value.toVariant().toUInt());
                    if (overlay->is3D()) {
                        auto overlay3D = std::static_pointer_cast<Base3DOverlay>(overlay);
                        setOffsetRotation([&overlay3D]() -> glm::quat {
                            return overlay3D->getRotation();
                        });
                    }
                } else if (bindTypeString == "panel") {
                    attachRotationToPanel(value.toVariant().toUInt());
                } else if (bindTypeString == "quat") {
                    QScriptValue x = value.property("x");
                    QScriptValue y = value.property("y");
                    QScriptValue z = value.property("z");
                    QScriptValue w = value.property("w");

                    if (x.isValid() && y.isValid() && z.isValid() && w.isValid()) {
                        glm::quat newRotation;
                        newRotation.x = x.toVariant().toFloat();
                        newRotation.y = y.toVariant().toFloat();
                        newRotation.z = z.toVariant().toFloat();
                        newRotation.w = w.toVariant().toFloat();
                        setOffsetRotation(newRotation);
                    }
                }
            }
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

void FloatingUIPanel::attachAnchorToPanel(unsigned int panelID) {
    FloatingUIPanel::Pointer panel = Application::getInstance()->getOverlays().getPanel(panelID);
    setAnchorPosition([panel]() -> glm::vec3 {
        return panel->getPosition();
    });
}

void FloatingUIPanel::attachRotationToPanel(unsigned int panelID) {
    FloatingUIPanel::Pointer panel = Application::getInstance()->getOverlays().getPanel(panelID);
    setOffsetRotation([panel]() -> glm::quat {
        return panel->getRotation();
    });
}
