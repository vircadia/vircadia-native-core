//
//  PanelAttachable.h
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/1/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PanelAttachable_h
#define hifi_PanelAttachable_h

#include "OverlayPanel.h"

#include <glm/glm.hpp>
#include <Transform.h>

class PanelAttachable {
public:
    OverlayPanel::Pointer getParentPanel() const { return _parentPanel; }
    virtual glm::vec3 getOffsetPosition() const { return _offsetPosition; }
    virtual glm::quat getFacingRotation() const { return _offsetRotation; }
    bool getParentVisible() const;

    void setParentPanel(OverlayPanel::Pointer panel) { _parentPanel = panel; }
    virtual void setOffsetPosition(const glm::vec3& position) { _offsetPosition = position; }
    virtual void setFacingRotation(const glm::quat& rotation) { _offsetRotation = rotation; }

    QScriptValue getProperty(QScriptEngine* scriptEngine, const QString& property);
    void setProperties(const QScriptValue& properties);

protected:
    virtual void applyTransformTo(Transform& transform);

private:
    OverlayPanel::Pointer _parentPanel = nullptr;
    glm::vec3 _offsetPosition = {0, 0, 0};
    glm::quat _offsetRotation = {1, 0, 0, 0};
};

#endif // hifi_PanelAttachable_h
