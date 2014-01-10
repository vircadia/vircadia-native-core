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

using namespace std;

SixenseManager::SixenseManager() : _lastMovement(usecTimestampNow()) {
#ifdef HAVE_SIXENSE
    sixenseInit();
#endif
}

SixenseManager::~SixenseManager() {
#ifdef HAVE_SIXENSE
    sixenseExit();
#endif
}

void SixenseManager::setFilter(bool filter) {
#ifdef HAVE_SIXENSE
    if (filter) {
        qDebug("Sixense Filter ON\n");
        sixenseSetFilterEnabled(1);
    } else {
        qDebug("Sixense Filter OFF\n");
        sixenseSetFilterEnabled(0);
    }
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
        
        //  Rotation of Palm
        glm::quat rotation(data.rot_quat[3], -data.rot_quat[0], data.rot_quat[1], -data.rot_quat[2]);
        rotation = glm::angleAxis(180.0f, 0.f, 1.f, 0.f) * rotation;
        const glm::vec3 PALM_VECTOR(0.0f, -1.0f, 0.0f);
        glm::vec3 newNormal = rotation * PALM_VECTOR;
        palm->setRawNormal(newNormal);
        palm->setRawRotation(rotation);
        
        //  Compute current velocity from position change
        glm::vec3 rawVelocity = (position - palm->getRawPosition()) / deltaTime / 1000.f;
        palm->setRawVelocity(rawVelocity);   //  meters/sec
        palm->setRawPosition(position);
        
        // use the velocity to determine whether there's any movement
        const float MOVEMENT_SPEED_THRESHOLD = 0.001f;
        if (glm::length(rawVelocity) > MOVEMENT_SPEED_THRESHOLD) {
            _lastMovement = usecTimestampNow();
        }
        
        // initialize the "finger" based on the direction
        FingerData finger(palm, &hand);
        finger.setActive(true);
        finger.setRawRootPosition(position);
        const float FINGER_LENGTH = 150.0f;   //  Millimeters
        const glm::vec3 FINGER_VECTOR(0.0f, 0.0f, FINGER_LENGTH);
        const glm::vec3 newTipPosition = position + rotation * FINGER_VECTOR;
        finger.setRawTipPosition(position + rotation * FINGER_VECTOR);
        
        // Store the one fingertip in the palm structure so we can track velocity
        glm::vec3 oldTipPosition = palm->getTipRawPosition();
        palm->setTipVelocity((newTipPosition - oldTipPosition) / deltaTime / 1000.f);
        palm->setTipPosition(newTipPosition);
        
        // three fingers indicates to the skeleton that we have enough data to determine direction
        palm->getFingers().clear();
        palm->getFingers().push_back(finger);
        palm->getFingers().push_back(finger);
        palm->getFingers().push_back(finger);
    }
    
    // if the controllers haven't been moved in a while, disable
    const int MOVEMENT_DISABLE_DURATION = 5 * 1000 * 1000;
    if (usecTimestampNow() - _lastMovement > MOVEMENT_DISABLE_DURATION) {
        for (vector<PalmData>::iterator it = hand.getPalms().begin(); it != hand.getPalms().end(); it++) {
            it->setActive(false);
        }
    }
#endif
}

