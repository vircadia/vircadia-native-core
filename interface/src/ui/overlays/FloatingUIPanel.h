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

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QScriptValue>

class FloatingUIPanel : public QObject {
    Q_OBJECT
public:
    typedef std::shared_ptr<FloatingUIPanel> Pointer;

    QList<unsigned int> children;

    void init(QScriptEngine* scriptEngine) { _scriptEngine = scriptEngine; }

    glm::vec3 getAnchorPosition() const { return _anchorPosition(); }
    glm::quat getOffsetRotation() const { return _offsetRotation(); }
    glm::vec3 getOffsetPosition() const { return _offsetPosition; }
    glm::quat getFacingRotation() const { return _facingRotation; }
    glm::vec3 getPosition() const;
    glm::quat getRotation() const;

    void setAnchorPosition(const std::function<glm::vec3()>& func) { _anchorPosition = func; }
    void setAnchorPosition(glm::vec3 position);
    void setOffsetRotation(const std::function<glm::quat()>& func) { _offsetRotation = func; }
    void setOffsetRotation(glm::quat rotation);
    void setOffsetPosition(glm::vec3 position) { _offsetPosition = position; }
    void setFacingRotation(glm::quat rotation) { _facingRotation = rotation; }

    QScriptValue getProperty(const QString& property);
    void setProperties(const QScriptValue& properties);

private:
    static std::function<glm::vec3()> const AVATAR_POSITION;
    static std::function<glm::quat()> const AVATAR_ORIENTATION;

    std::function<glm::vec3()> _anchorPosition;
    std::function<glm::quat()> _offsetRotation;
    glm::vec3 _offsetPosition{0, 0, 0};
    glm::quat _facingRotation{1, 0, 0, 0};
    QScriptEngine* _scriptEngine;
};

#endif // hifi_FloatingUIPanel_h
