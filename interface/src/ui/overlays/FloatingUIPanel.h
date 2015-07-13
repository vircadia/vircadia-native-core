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

    glm::vec3 getOffsetPosition() const { return _offsetPosition; }
    glm::quat getOffsetRotation() const;
    glm::quat getActualOffsetRotation() const { return _offsetRotation; }
    glm::quat getFacingRotation() const { return _facingRotation; }

    void setOffsetPosition(glm::vec3 position) { _offsetPosition = position; };
    void setOffsetRotation(glm::quat rotation) { _offsetRotation = rotation; };
    void setFacingRotation(glm::quat rotation) { _facingRotation = rotation; };

    QScriptValue getProperty(const QString& property);
    void setProperties(const QScriptValue& properties);

private:
    glm::vec3 _offsetPosition = glm::vec3(0, 0, 0);
    glm::quat _offsetRotation = glm::quat(0, 0, 0, 0);
    glm::quat _facingRotation = glm::quat(1, 0, 0, 0);
    QScriptEngine* _scriptEngine;
};

#endif // hifi_FloatingUIPanel_h
