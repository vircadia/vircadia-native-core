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

#include <memory>

#include <glm/glm.hpp>
#include <Transform.h>
#include <QScriptValue>
#include <QScriptEngine>
#include <QUuid>

class OverlayPanel;

class PanelAttachable {
public:
    // getters
    std::shared_ptr<OverlayPanel> getParentPanel() const { return _parentPanel; }
    glm::vec3 getOffsetPosition() const { return _offset.getTranslation(); }
    glm::quat getOffsetRotation() const { return _offset.getRotation(); }
    glm::vec3 getOffsetScale() const { return _offset.getScale(); }
    bool getParentVisible() const;

    // setters
    void setParentPanel(std::shared_ptr<OverlayPanel> panel) { _parentPanel = panel; }
    void setOffsetPosition(const glm::vec3& position) { _offset.setTranslation(position); }
    void setOffsetRotation(const glm::quat& rotation) { _offset.setRotation(rotation); }
    void setOffsetScale(float scale) { _offset.setScale(scale); }
    void setOffsetScale(const glm::vec3& scale) { _offset.setScale(scale); }

protected:
    QScriptValue getProperty(QScriptEngine* scriptEngine, const QString& property);
    void setProperties(const QScriptValue& properties);

    virtual void applyTransformTo(Transform& transform, bool force = false);
    quint64 _transformExpiry = 0;

private:
    std::shared_ptr<OverlayPanel> _parentPanel = nullptr;
    Transform _offset;
};

#endif // hifi_PanelAttachable_h
