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

#include <functional>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QScriptValue>

class FloatingUIPanel : public QObject {
    Q_OBJECT
public:
    typedef std::shared_ptr<FloatingUIPanel> Pointer;

    void init(QScriptEngine* scriptEngine) { _scriptEngine = scriptEngine; }

    glm::vec3 getAnchorPosition() const { return _anchorPosition(); }
    glm::quat getOffsetRotation() const { return _offsetRotation(); }
    glm::vec3 getOffsetPosition() const { return _offsetPosition; }
    glm::quat getFacingRotation() const { return _facingRotation; }
    glm::vec3 getPosition() const;
    glm::quat getRotation() const;

    void setAnchorPosition(const std::function<glm::vec3()>& func) { _anchorPosition = func; }
    void setAnchorPosition(const glm::vec3& position);
    void setOffsetRotation(const std::function<glm::quat()>& func) { _offsetRotation = func; }
    void setOffsetRotation(const glm::quat& rotation);
    void setOffsetPosition(const glm::vec3& position) { _offsetPosition = position; }
    void setFacingRotation(const glm::quat& rotation) { _facingRotation = rotation; }

    const QList<unsigned int>& getChildren() { return _children; }
    void addChild(unsigned int childId);
    void removeChild(unsigned int childId);
    unsigned int popLastChild() { return _children.takeLast(); }

    QScriptValue getProperty(const QString& property);
    void setProperties(const QScriptValue& properties);

private:
    static std::function<glm::vec3()> const AVATAR_POSITION;
    static std::function<glm::quat()> const AVATAR_ORIENTATION;

    std::function<glm::vec3()> _anchorPosition{AVATAR_POSITION};
    std::function<glm::quat()> _offsetRotation{AVATAR_ORIENTATION};
    glm::vec3 _offsetPosition{0, 0, 0};
    glm::quat _facingRotation{1, 0, 0, 0};
    QScriptEngine* _scriptEngine;
    QList<unsigned int> _children;
};

#endif // hifi_FloatingUIPanel_h
