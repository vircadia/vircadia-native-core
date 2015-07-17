//
//  PanelAttachable.cpp
//  hifi
//
//  Created by Zander Otavka on 7/15/15.
//
//

#include "PanelAttachable.h"

PanelAttachable::PanelAttachable() :
    _attachedPanel(nullptr),
    _offsetPosition(glm::vec3())
{
}

PanelAttachable::PanelAttachable(const PanelAttachable* panelAttachable) :
    _attachedPanel(panelAttachable->_attachedPanel),
    _offsetPosition(panelAttachable->_offsetPosition)
{
}

glm::vec3 PanelAttachable::getTranslatedPosition(glm::vec3 avatarPosition) const {
    if (getAttachedPanel()) {
        glm::vec3 totalOffsetPosition =
        getAttachedPanel()->getFacingRotation() * getOffsetPosition() +
        getAttachedPanel()->getOffsetPosition();

        return getAttachedPanel()->getOffsetRotation() * totalOffsetPosition +
        avatarPosition;
    }
    return glm::vec3();
}

glm::quat PanelAttachable::getTranslatedRotation(glm::quat offsetRotation) const {
    glm::quat rotation = offsetRotation;
    if (getAttachedPanel()) {
        rotation *= getAttachedPanel()->getOffsetRotation() *
        getAttachedPanel()->getFacingRotation();
        //            if (getAttachedPanel()->getFacingRotation() != glm::quat(0, 0, 0, 0)) {
        //                rotation *= getAttachedPanel()->getFacingRotation();
        //            } else if (getAttachedPanel()->getOffsetRotation() != glm::quat(0, 0, 0, 0)) {
        //                rotation *= getAttachedPanel()->getOffsetRotation();
        //            } else {
        //                rotation *= Application::getInstance()->getCamera()->getOrientation() *
        //                            glm::quat(0, 0, 1, 0);
        //            }
    }
    return rotation;
}
