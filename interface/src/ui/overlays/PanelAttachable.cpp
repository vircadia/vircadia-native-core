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

bool PanelAttachable::setTransforms(Transform* transform) {
    Q_ASSERT(transform != nullptr);
    if (getAttachedPanel()) {
        transform->setTranslation(getAttachedPanel()->getAnchorPosition());
        transform->setRotation(getAttachedPanel()->getOffsetRotation());
        transform->postTranslate(getOffsetPosition() + getAttachedPanel()->getOffsetPosition());
        transform->postRotate(getFacingRotation() * getAttachedPanel()->getFacingRotation());
        return true;
    }
    return false;
}
