//
//  OverlayPanel.h
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/2/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OverlayPanel_h
#define hifi_OverlayPanel_h

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QUuid>

#include "PanelAttachable.h"
#include "Billboardable.h"
#include "Overlay.h"

#if OVERLAY_PANELS
class PropertyBinding {
public:
    PropertyBinding() {}
    PropertyBinding(QString avatar, QUuid entity);
    QString avatar;
    QUuid entity;
};

QVariant propertyBindingToVariant(const PropertyBinding& value);
void propertyBindingFromVariant(const QVariant& object, PropertyBinding& value);


class OverlayPanel : public QObject, public PanelAttachable, public Billboardable {
    Q_OBJECT

public:
    typedef std::shared_ptr<OverlayPanel> Pointer;

    void init(QScriptEngine* scriptEngine) { _scriptEngine = scriptEngine; }

    // getters
    glm::vec3 getAnchorPosition() const { return _anchorTransform.getTranslation(); }
    glm::quat getAnchorRotation() const { return _anchorTransform.getRotation(); }
    glm::vec3 getAnchorScale() const { return _anchorTransform.getScale(); }
    bool getVisible() const { return _visible; }

    // setters
    void setAnchorPosition(const glm::vec3& position) { _anchorTransform.setTranslation(position); }
    void setAnchorRotation(const glm::quat& rotation) { _anchorTransform.setRotation(rotation); }
    void setAnchorScale(float scale) { _anchorTransform.setScale(scale); }
    void setAnchorScale(const glm::vec3& scale) { _anchorTransform.setScale(scale); }
    void setVisible(bool visible) { _visible = visible; }

    const QList<OverlayID>& getChildren() { return _children; }
    void addChild(OverlayID childId);
    void removeChild(OverlayID childId);
    OverlayID popLastChild() { return _children.takeLast(); }

    void setProperties(const QVariantMap& properties);
    QVariant getProperty(const QString& property);

    virtual void applyTransformTo(Transform& transform, bool force = false) override;

private:
    Transform _anchorTransform;

    bool _anchorPositionBindMyAvatar = false;
    QUuid _anchorPositionBindEntity;

    bool _anchorRotationBindMyAvatar = false;
    QUuid _anchorRotationBindEntity;

    bool _visible = true;
    QList<OverlayID> _children;

    QScriptEngine* _scriptEngine;
};

#endif

#endif // hifi_OverlayPanel_h
