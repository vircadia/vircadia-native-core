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

class OverlayPanel : public QObject {
    Q_OBJECT

public:
    typedef std::shared_ptr<OverlayPanel> Pointer;

    void init(QScriptEngine* scriptEngine) { _scriptEngine = scriptEngine; }

    // getters
    glm::vec3 getPosition() const { return _position; }
    glm::vec3 getComputedPosition() const;
    glm::quat getRotation() const { return _rotation; }
    glm::quat getComputedRotation() const;
    glm::vec3 getOffsetPosition() const { return _offsetPosition; }
    glm::quat getFacingRotation() const { return _offsetRotation; }
    Pointer getParentPanel() const { return _parentPanel; }
    bool getVisible() const { return _visible; }
    bool getParentVisible() const;

    // setters
    void setPosition(const glm::vec3& position) { _position = position; }
    void setRotation(const glm::quat& rotation) { _rotation = rotation; }
    void setOffsetPosition(const glm::vec3& position) { _offsetPosition = position; }
    void setFacingRotation(const glm::quat& rotation) { _offsetRotation = rotation; }
    void setParentPanel(Pointer panel) { _parentPanel = panel; }
    void setVisible(bool visible) { _visible = visible; }

    const QList<unsigned int>& getChildren() { return _children; }
    void addChild(unsigned int childId);
    void removeChild(unsigned int childId);
    unsigned int popLastChild() { return _children.takeLast(); }

    QScriptValue getProperty(const QString& property);
    void setProperties(const QScriptValue& properties);

private:
    glm::vec3 _position = {0, 0, 0};
    glm::quat _rotation = {1, 0, 0, 0};
    glm::vec3 _offsetPosition = {0, 0, 0};
    glm::quat _offsetRotation = {1, 0, 0, 0};

    bool _positionBindMyAvatar = false;
    QUuid _positionBindEntity;

    bool _rotationBindMyAvatar = false;
    QUuid _rotationBindEntity;

    Pointer _parentPanel = nullptr;
    QList<unsigned int> _children;
    bool _visible = true;

    QScriptEngine* _scriptEngine;
};

#endif // hifi_OverlayPanel_h
