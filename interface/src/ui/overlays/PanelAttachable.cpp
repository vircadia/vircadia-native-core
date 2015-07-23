//
//  PanelAttachable.cpp
//  hifi
//
//  Created by Zander Otavka on 7/15/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PanelAttachable.h"

PanelAttachable::PanelAttachable() :
    _attachedPanel(nullptr),
    _facingRotation(1, 0, 0, 0)
{
}

PanelAttachable::PanelAttachable(const PanelAttachable* panelAttachable) :
    _attachedPanel(panelAttachable->_attachedPanel),
    _offsetPosition(panelAttachable->_offsetPosition),
    _facingRotation(panelAttachable->_facingRotation)
{
}

void PanelAttachable::setTransforms(Transform* transform) {
    Q_ASSERT(transform != nullptr);
    if (getAttachedPanel()) {
        transform->setTranslation(getAttachedPanel()->getAnchorPosition());
        transform->setRotation(getAttachedPanel()->getOffsetRotation());
        transform->postTranslate(getOffsetPosition() + getAttachedPanel()->getOffsetPosition());
        transform->postRotate(getFacingRotation() * getAttachedPanel()->getFacingRotation());
    }
}
