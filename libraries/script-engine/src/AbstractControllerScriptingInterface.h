//
//  AbstractControllerScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/17/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AbstractControllerScriptingInterface__
#define __hifi__AbstractControllerScriptingInterface__

#include <QtCore/QObject>
#include <glm/glm.hpp>

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
};

#endif /* defined(__hifi__AbstractControllerScriptingInterface__) */
