//
//  ControllerScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/17/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ControllerScriptingInterface__
#define __hifi__ControllerScriptingInterface__

#include <QtCore/QObject>

#include <AbstractControllerScriptingInterface.h>

/// handles scripting of input controller commands from JS
class ControllerScriptingInterface : public AbstractControllerScriptingInterface {
    Q_OBJECT

public slots:
    virtual bool isPrimaryButtonPressed() const;
    virtual glm::vec2 getPrimaryJoystickPosition() const;

    virtual int getNumberOfButtons() const;
    virtual bool isButtonPressed(int buttonIndex) const;

    virtual int getNumberOfTriggers() const;
    virtual float getTriggerValue(int triggerIndex) const;

    virtual int getNumberOfJoysticks() const;
    virtual glm::vec2 getJoystickPosition(int joystickIndex) const;

    virtual int getNumberOfSpatialControls() const;
    virtual glm::vec3 getSpatialControlPosition(int controlIndex) const;

private:
    const PalmData* getPrimaryPalm() const;
    const PalmData* getPalm(int palmIndex) const;
    int getNumberOfActivePalms() const;
    const PalmData* getActivePalm(int palmIndex) const;

    const int NUMBER_OF_SPATIALCONTROLS_PER_PALM = 0;
    const int NUMBER_OF_JOYSTICKS_PER_PALM = 1;
    const int NUMBER_OF_TRIGGERS_PER_PALM = 1;
    const int NUMBER_OF_BUTTONS_PER_PALM = 6;
};

#endif /* defined(__hifi__ControllerScriptingInterface__) */
