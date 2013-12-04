//
//  Sixense.cpp
//  interface
//
//  Created by Andrzej Kapolka on 11/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifdef HAVE_SIXENSE
    #include "sixense.h"
#endif

#include "Application.h"
#include "SixenseManager.h"

SixenseManager::SixenseManager() {
#ifdef HAVE_SIXENSE
    sixenseInit();
#endif
}

SixenseManager::~SixenseManager() {
#ifdef HAVE_SIXENSE
    sixenseExit();
#endif
}
    
void SixenseManager::update(float deltaTime) {
#ifdef HAVE_SIXENSE
    if (sixenseGetNumActiveControllers() == 0) {
        return;
    }
    MyAvatar* avatar = Application::getInstance()->getAvatar();
    Hand& hand = avatar->getHand();
    
    int maxControllers = sixenseGetMaxControllers();
    for (int i = 0; i < maxControllers; i++) {
        if (!sixenseIsControllerEnabled(i)) {
            continue;
        }
        sixenseControllerData data;
        sixenseGetNewestData(i, &data);
        
        //printf("si: %i\n", data.controller_index);
        
        //  Set palm position and normal based on Hydra position/orientation
        
        // Either find a palm matching the sixense controller, or make a new one
        PalmData* palm;
        bool foundHand = false;
        for (int j = 0; j < hand.getNumPalms(); j++) {
            if (hand.getPalms()[j].getSixenseID() == data.controller_index) {
                palm = &hand.getPalms()[j];
                foundHand = true;
            }
        }
        if (!foundHand) {
            PalmData newPalm(&hand);
            hand.getPalms().push_back(newPalm);
            palm = &hand.getPalms()[hand.getNumPalms() - 1];
            palm->setSixenseID(data.controller_index);
            printf("Found new Sixense controller, ID %i\n", data.controller_index);
        }
        
        palm->setActive(true);
        
        //  Read controller buttons and joystick into the hand
        palm->setControllerButtons(data.buttons);
        palm->setTrigger(data.trigger);
        palm->setJoystick(data.joystick_x, data.joystick_y);

        glm::vec3 position(data.pos[0], data.pos[1], data.pos[2]);
        //  Adjust for distance between acquisition 'orb' and the user's torso
        //  (distance to the right of body center, distance below torso, distance behind torso)
        const glm::vec3 SPHERE_TO_TORSO(-250.f, -300.f, -300.f);
        position = SPHERE_TO_TORSO + position;
        
        //  Compute current velocity from position change
        palm->setVelocity((position - palm->getRawPosition()) / deltaTime / 1000.f);   //  meters/sec
        palm->setRawPosition(position);
        
        //  Rotation of Palm
        glm::quat rotation(data.rot_quat[3], -data.rot_quat[0], data.rot_quat[1], -data.rot_quat[2]);
        rotation = glm::angleAxis(180.0f, 0.f, 1.f, 0.f) * rotation;
        const glm::vec3 PALM_VECTOR(0.0f, -1.0f, 0.0f);
        palm->setRawNormal(rotation * PALM_VECTOR);
        
        // initialize the "finger" based on the direction
        FingerData finger(palm, &hand);
        finger.setActive(true);
        finger.setRawRootPosition(position);
        const float FINGER_LENGTH = 100.0f;   //  Millimeters
        const glm::vec3 FINGER_VECTOR(0.0f, 0.0f, FINGER_LENGTH);
        finger.setRawTipPosition(position + rotation * FINGER_VECTOR);
        
        // three fingers indicates to the skeleton that we have enough data to determine direction
        palm->getFingers().clear();
        palm->getFingers().push_back(finger);
        palm->getFingers().push_back(finger);
        palm->getFingers().push_back(finger);
    }
#endif
}

