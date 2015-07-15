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
