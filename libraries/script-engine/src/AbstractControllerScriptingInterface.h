//
//  AbstractControllerScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 12/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AbstractControllerScriptingInterface_h
#define hifi_AbstractControllerScriptingInterface_h

#include <QtCore/QObject>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "EventTypes.h"

class AbstractInputController : public QObject {
    Q_OBJECT

public:
    typedef unsigned int Key;

    virtual void update() = 0;

    virtual Key getKey() const = 0;

public slots:

    virtual bool isActive() const = 0;
    virtual glm::vec3 getAbsTranslation() const = 0;
    virtual glm::quat getAbsRotation() const = 0;
    virtual glm::vec3 getLocTranslation() const = 0;
    virtual glm::quat getLocRotation() const = 0;

signals:
    void spatialEvent(const SpatialEvent& event);

};

/// handles scripting of input controller commands from JS
class AbstractControllerScriptingInterface : public QObject {
    Q_OBJECT

public slots:
    virtual bool isPrimaryButtonPressed() const = 0;
    virtual glm::vec2 getPrimaryJoystickPosition() const = 0;

    virtual int getNumberOfButtons() const = 0;
    virtual bool isButtonPressed(int buttonIndex) const = 0;

    virtual int getNumberOfTriggers() const = 0;
    virtual float getTriggerValue(int triggerIndex) const = 0;

    virtual int getNumberOfJoysticks() const = 0;
    virtual glm::vec2 getJoystickPosition(int joystickIndex) const = 0;

    virtual int getNumberOfSpatialControls() const = 0;
    virtual glm::vec3 getSpatialControlPosition(int controlIndex) const = 0;
    virtual glm::vec3 getSpatialControlVelocity(int controlIndex) const = 0;
    virtual glm::vec3 getSpatialControlNormal(int controlIndex) const = 0;
    virtual glm::quat getSpatialControlRawRotation(int controlIndex) const = 0;

    virtual void captureKeyEvents(const KeyEvent& event) = 0;
    virtual void releaseKeyEvents(const KeyEvent& event) = 0;

    virtual void captureMouseEvents() = 0;
    virtual void releaseMouseEvents() = 0;

    virtual void captureTouchEvents() = 0;
    virtual void releaseTouchEvents() = 0;

    virtual void captureWheelEvents() = 0;
    virtual void releaseWheelEvents() = 0;

    virtual void captureJoystick(int joystickIndex) = 0;
    virtual void releaseJoystick(int joystickIndex) = 0;

    virtual glm::vec2 getViewportDimensions() const = 0;


    virtual AbstractInputController* createInputController( const QString& category, const QString& tracker ) = 0;

signals:
    void keyPressEvent(const KeyEvent& event);
    void keyReleaseEvent(const KeyEvent& event);

    void mouseMoveEvent(const MouseEvent& event, unsigned int deviceID = 0);
    void mousePressEvent(const MouseEvent& event, unsigned int deviceID = 0);
    void mouseReleaseEvent(const MouseEvent& event, unsigned int deviceID = 0);

    void touchBeginEvent(const TouchEvent& event);
    void touchEndEvent(const TouchEvent& event);
    void touchUpdateEvent(const TouchEvent& event);
    
    void wheelEvent(const WheelEvent& event);

};

#endif // hifi_AbstractControllerScriptingInterface_h
