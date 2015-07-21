//
//  PanelAttachable.h
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/1/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PanelAttachable_h
#define hifi_PanelAttachable_h

#include "FloatingUIPanel.h"

#include <glm/glm.hpp>
#include <Transform.h>

class PanelAttachable {
public:
    PanelAttachable();
    PanelAttachable(const PanelAttachable* panelAttachable);

    FloatingUIPanel* getAttachedPanel() const { return _attachedPanel; }
    glm::vec3 getOffsetPosition() const { return _offsetPosition; }
    glm::quat getFacingRotation() const { return _facingRotation; }

    void setAttachedPanel(FloatingUIPanel* panel) { _attachedPanel = panel; }
    void setOffsetPosition(glm::vec3 position) { _offsetPosition = position; }
    void setFacingRotation(glm::quat rotation) { _facingRotation = rotation; }

protected:
    bool setTransforms(Transform* transform);

private:
    FloatingUIPanel* _attachedPanel;
    glm::vec3 _offsetPosition;
    glm::quat _facingRotation;
};

#endif // hifi_PanelAttachable_h
