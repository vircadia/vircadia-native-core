//
//  FloatingUIPanel.h
//  interface/src/ui/overlays
//
//  Created by Zander Otavka on 7/2/15.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FloatingUIPanel_h
#define hifi_FloatingUIPanel_h

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QScriptValue>
#include <QUuid>

class FloatingUIPanel : public QObject {
    Q_OBJECT

public:
    typedef std::shared_ptr<FloatingUIPanel> Pointer;

    void init(QScriptEngine* scriptEngine) { _scriptEngine = scriptEngine; }

    // getters
    glm::vec3 getAnchorPosition() const { return _anchorPosition; }
    glm::vec3 getComputedAnchorPosition() const;
    glm::quat getOffsetRotation() const { return _offsetRotation; }
    glm::quat getComputedOffsetRotation() const;
    glm::vec3 getOffsetPosition() const { return _offsetPosition; }
    glm::quat getFacingRotation() const { return _facingRotation; }
    glm::vec3 getPosition() const;
    glm::quat getRotation() const;
    Pointer getAttachedPanel() const { return _attachedPanel; }

    // setters
    void setAnchorPosition(const glm::vec3& position) { _anchorPosition = position; }
    void setOffsetRotation(const glm::quat& rotation) { _offsetRotation = rotation; }
    void setOffsetPosition(const glm::vec3& position) { _offsetPosition = position; }
    void setFacingRotation(const glm::quat& rotation) { _facingRotation = rotation; }
    void setAttachedPanel(Pointer panel) { _attachedPanel = panel; }

    const QList<unsigned int>& getChildren() { return _children; }
    void addChild(unsigned int childId);
    void removeChild(unsigned int childId);
    unsigned int popLastChild() { return _children.takeLast(); }

    QScriptValue getProperty(const QString& property);
    void setProperties(const QScriptValue& properties);

private:
    glm::vec3 _anchorPosition = {0, 0, 0};
    glm::quat _offsetRotation = {1, 0, 0, 0};
    glm::vec3 _offsetPosition = {0, 0, 0};
    glm::quat _facingRotation = {1, 0, 0, 0};

    bool _anchorPositionBindMyAvatar = false;
    QUuid _anchorPositionBindEntity;

    bool _offsetRotationBindMyAvatar = false;
    QUuid _offsetRotationBindEntity;

    Pointer _attachedPanel = nullptr;
    QScriptEngine* _scriptEngine;
    QList<unsigned int> _children;
};

#endif // hifi_FloatingUIPanel_h
