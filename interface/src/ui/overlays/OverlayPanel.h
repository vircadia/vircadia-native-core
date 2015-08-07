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
#include <QScriptValue>
#include <QUuid>

#include "PanelAttachable.h"
#include "Billboardable.h"

class PropertyBinding {
public:
    PropertyBinding() {}
    PropertyBinding(QString avatar, QUuid entity);
    QString avatar;
    QUuid entity;
};

QScriptValue propertyBindingToScriptValue(QScriptEngine* engine, const PropertyBinding& value);
void propertyBindingFromScriptValue(const QScriptValue& object, PropertyBinding& value);


class OverlayPanel : public QObject, public PanelAttachable, public Billboardable {
    Q_OBJECT

public:
    typedef std::shared_ptr<OverlayPanel> Pointer;

    void init(QScriptEngine* scriptEngine) { _scriptEngine = scriptEngine; }

    // getters
    glm::vec3 getPosition() const { return _transform.getTranslation(); }
    glm::quat getRotation() const { return _transform.getRotation(); }
    glm::vec3 getScale() const { return _transform.getScale(); }
    bool getVisible() const { return _visible; }

    // setters
    void setPosition(const glm::vec3& position) { _transform.setTranslation(position); }
    void setRotation(const glm::quat& rotation) { _transform.setRotation(rotation); }
    void setScale(float scale) { _transform.setScale(scale); }
    void setScale(const glm::vec3& scale) { _transform.setScale(scale); }
    void setVisible(bool visible) { _visible = visible; }

    const QList<unsigned int>& getChildren() { return _children; }
    void addChild(unsigned int childId);
    void removeChild(unsigned int childId);
    unsigned int popLastChild() { return _children.takeLast(); }

    QScriptValue getProperty(const QString& property);
    void setProperties(const QScriptValue& properties);

    virtual void applyTransformTo(Transform& transform, bool force = false);

private:
    void updateTransform();

    Transform _transform;

    bool _positionBindMyAvatar = false;
    QUuid _positionBindEntity;

    bool _rotationBindMyAvatar = false;
    QUuid _rotationBindEntity;

    bool _visible = true;
    QList<unsigned int> _children;

    QScriptEngine* _scriptEngine;
};

#endif // hifi_OverlayPanel_h
