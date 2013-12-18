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

