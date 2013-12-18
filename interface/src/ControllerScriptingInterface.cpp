//
//  ControllerScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/17/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <HandData.h>
#include "Application.h"
#include "ControllerScriptingInterface.h"

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
    return getNumberOfActivePalms() * NUMBER_OF_BUTTONS_PER_PALM;
}

bool ControllerScriptingInterface::isButtonPressed(int buttonIndex) const {
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
    return getNumberOfActivePalms() * NUMBER_OF_JOYSTICKS_PER_PALM;
}

glm::vec2 ControllerScriptingInterface::getJoystickPosition(int joystickIndex) const {
    // we know there's one joystick per palm, so the joystickIndex is the palm Index
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
    return glm::vec3(0); // not yet implemented
}



