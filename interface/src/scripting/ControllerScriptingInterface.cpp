//
//  ControllerScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 12/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <HandData.h>
#include "Application.h"
#include "devices/SixenseManager.h"
#include "ControllerScriptingInterface.h"

ControllerScriptingInterface::ControllerScriptingInterface() :
    _mouseCaptured(false),
    _touchCaptured(false),
    _wheelCaptured(false)
{
}


const PalmData* ControllerScriptingInterface::getPrimaryPalm() const {
    int leftPalmIndex, rightPalmIndex;

    const HandData* handData = Application::getInstance()->getAvatar()->getHandData();
    handData->getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);
    
    if (rightPalmIndex != -1) {
        return &handData->getPalms()[rightPalmIndex];
    }

    return NULL;
}

int ControllerScriptingInterface::getNumberOfActivePalms() const {
    const HandData* handData = Application::getInstance()->getAvatar()->getHandData();
    int numberOfPalms = handData->getNumPalms();
    int numberOfActivePalms = 0;
    for (int i = 0; i < numberOfPalms; i++) {
        if (getPalm(i)->isActive()) {
            numberOfActivePalms++;
        }
    }
    return numberOfActivePalms;
}

const PalmData* ControllerScriptingInterface::getPalm(int palmIndex) const {
    const HandData* handData = Application::getInstance()->getAvatar()->getHandData();
    return &handData->getPalms()[palmIndex];
}

const PalmData* ControllerScriptingInterface::getActivePalm(int palmIndex) const {
    const HandData* handData = Application::getInstance()->getAvatar()->getHandData();
    int numberOfPalms = handData->getNumPalms();
    int numberOfActivePalms = 0;
    for (int i = 0; i < numberOfPalms; i++) {
        if (getPalm(i)->isActive()) {
            if (numberOfActivePalms == palmIndex) {
                return &handData->getPalms()[i];
            }
            numberOfActivePalms++;
        }
    }
    return NULL;
}

bool ControllerScriptingInterface::isPrimaryButtonPressed() const {
    const PalmData* primaryPalm = getPrimaryPalm();
    if (primaryPalm) {
        if (primaryPalm->getControllerButtons() & BUTTON_FWD) {
            return true;
        }
    }

    return false;
}

glm::vec2 ControllerScriptingInterface::getPrimaryJoystickPosition() const {
    const PalmData* primaryPalm = getPrimaryPalm();
    if (primaryPalm) {
        return glm::vec2(primaryPalm->getJoystickX(), primaryPalm->getJoystickY());
    }

    return glm::vec2(0);
}

int ControllerScriptingInterface::getNumberOfButtons() const {
    int buttonCount = 0;
    foreach (const JoystickState& state, Application::getInstance()->getJoystickManager()->getJoystickStates()) {
        buttonCount += state.buttons.size();
    }
    return buttonCount + getNumberOfActivePalms() * NUMBER_OF_BUTTONS_PER_PALM;
}

bool ControllerScriptingInterface::isButtonPressed(int buttonIndex) const {
    int managedButtonIndex = buttonIndex - getNumberOfActivePalms() * NUMBER_OF_BUTTONS_PER_PALM;
    if (managedButtonIndex >= 0) {
        foreach (const JoystickState& state, Application::getInstance()->getJoystickManager()->getJoystickStates()) {
            if (managedButtonIndex < state.buttons.size()) {
                return state.buttons.at(managedButtonIndex);
            }
            managedButtonIndex -= state.buttons.size();
        }
        return false;
    }
    int palmIndex = buttonIndex / NUMBER_OF_BUTTONS_PER_PALM;
    int buttonOnPalm = buttonIndex % NUMBER_OF_BUTTONS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (buttonOnPalm) {
            case 0:
                return palmData->getControllerButtons() & BUTTON_0;
            case 1:
                return palmData->getControllerButtons() & BUTTON_1;
            case 2:
                return palmData->getControllerButtons() & BUTTON_2;
            case 3:
                return palmData->getControllerButtons() & BUTTON_3;
            case 4:
                return palmData->getControllerButtons() & BUTTON_4;
            case 5:
                return palmData->getControllerButtons() & BUTTON_FWD;
        }
    }
    return false;
}

int ControllerScriptingInterface::getNumberOfTriggers() const {
    return getNumberOfActivePalms() * NUMBER_OF_TRIGGERS_PER_PALM;
}

float ControllerScriptingInterface::getTriggerValue(int triggerIndex) const {
    // we know there's one trigger per palm, so the triggerIndex is the palm Index
    int palmIndex = triggerIndex;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        return palmData->getTrigger();
    }
    return 0.0f;
}

int ControllerScriptingInterface::getNumberOfJoysticks() const {
    return getNumberOfActivePalms() * NUMBER_OF_JOYSTICKS_PER_PALM +
        Application::getInstance()->getJoystickManager()->getJoystickStates().size();
}

glm::vec2 ControllerScriptingInterface::getJoystickPosition(int joystickIndex) const {
    // we know there's one joystick per palm, so the joystickIndex is the palm Index
    int managedJoystickIndex = joystickIndex - getNumberOfActivePalms();
    if (managedJoystickIndex >= 0) {
        const JoystickState& state = Application::getInstance()->getJoystickManager()->getJoystickStates().at(
            managedJoystickIndex);
        return glm::vec2(state.axes.size() > 0 ? state.axes.at(0) : 0.0f, state.axes.size() > 1 ? state.axes.at(1) : 0.0f);
    }
    int palmIndex = joystickIndex;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        return glm::vec2(palmData->getJoystickX(), palmData->getJoystickY());
    }
    return glm::vec2(0);
}

int ControllerScriptingInterface::getNumberOfSpatialControls() const {
    return getNumberOfActivePalms() * NUMBER_OF_SPATIALCONTROLS_PER_PALM;
}

glm::vec3 ControllerScriptingInterface::getSpatialControlPosition(int controlIndex) const {
    int palmIndex = controlIndex / NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    int controlOfPalm = controlIndex % NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (controlOfPalm) {
            case PALM_SPATIALCONTROL:
                return palmData->getPosition();
            case TIP_SPATIALCONTROL:
                return palmData->getTipPosition();
        }
    }
    return glm::vec3(0); // bad index
}

glm::vec3 ControllerScriptingInterface::getSpatialControlVelocity(int controlIndex) const {
    int palmIndex = controlIndex / NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    int controlOfPalm = controlIndex % NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (controlOfPalm) {
            case PALM_SPATIALCONTROL:
                return palmData->getVelocity();
            case TIP_SPATIALCONTROL:
                return palmData->getTipVelocity();
        }
    }
    return glm::vec3(0); // bad index
}

glm::quat ControllerScriptingInterface::getSpatialControlRawRotation(int controlIndex) const {
    int palmIndex = controlIndex / NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    int controlOfPalm = controlIndex % NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (controlOfPalm) {
            case PALM_SPATIALCONTROL:
                return palmData->getRawRotation();
            case TIP_SPATIALCONTROL:
                return palmData->getRawRotation(); // currently the tip doesn't have a unique rotation, use the palm rotation
        }
    }
    return glm::quat(); // bad index
}

glm::vec3 ControllerScriptingInterface::getSpatialControlNormal(int controlIndex) const {
    int palmIndex = controlIndex / NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    int controlOfPalm = controlIndex % NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (controlOfPalm) {
            case PALM_SPATIALCONTROL:
                return palmData->getNormal();
            case TIP_SPATIALCONTROL:
                return palmData->getFingerDirection();
        }
    }
    return glm::vec3(0); // bad index
}

bool ControllerScriptingInterface::isKeyCaptured(QKeyEvent* event) const {
    return isKeyCaptured(KeyEvent(*event));
}

bool ControllerScriptingInterface::isKeyCaptured(const KeyEvent& event) const {
    // if we've captured some combination of this key it will be in the map
    if (_capturedKeys.contains(event.key, event)) {
        return true;
    }
    return false;
}

void ControllerScriptingInterface::captureKeyEvents(const KeyEvent& event) {
    // if it's valid
    if (event.isValid) {
        // and not already captured
        if (!isKeyCaptured(event)) {
            // then add this KeyEvent record to the captured combos for this key
            _capturedKeys.insert(event.key, event);
        }
    }
}

void ControllerScriptingInterface::releaseKeyEvents(const KeyEvent& event) {
    if (event.isValid) {
        // and not already captured
        if (isKeyCaptured(event)) {
            _capturedKeys.remove(event.key, event);
        }
    }
}

bool ControllerScriptingInterface::isJoystickCaptured(int joystickIndex) const {
    return _capturedJoysticks.contains(joystickIndex);
}

void ControllerScriptingInterface::captureJoystick(int joystickIndex) {
    if (!isJoystickCaptured(joystickIndex)) {
        _capturedJoysticks.insert(joystickIndex);
    }
}

void ControllerScriptingInterface::releaseJoystick(int joystickIndex) {
    if (isJoystickCaptured(joystickIndex)) {
        _capturedJoysticks.remove(joystickIndex);
    }
}

glm::vec2 ControllerScriptingInterface::getViewportDimensions() const { 
    QGLWidget* widget = Application::getInstance()->getGLWidget();
    return glm::vec2(widget->width(), widget->height()); 
}
