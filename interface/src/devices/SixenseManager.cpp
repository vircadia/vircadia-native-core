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
    
void SixenseManager::update() {
#ifdef HAVE_SIXENSE
    if (sixenseGetNumActiveControllers() == 0) {
        return;
    }
    Hand& hand = Application::getInstance()->getAvatar()->getHand();
    hand.getPalms().clear();
    
    int maxControllers = sixenseGetMaxControllers();
    for (int i = 0; i < maxControllers; i++) {
        if (!sixenseIsControllerEnabled(i)) {
            continue;
        }
        sixenseControllerData data;
        sixenseGetNewestData(i, &data);
        
        // set palm position and normal based on Hydra position/orientation
        PalmData palm(&hand);
        palm.setActive(true);
        glm::vec3 position(-data.pos[0], data.pos[1], -data.pos[2]);
        palm.setRawPosition(position);
        glm::quat rotation(data.rot_quat[3], -data.rot_quat[0], data.rot_quat[1], -data.rot_quat[2]);
        const glm::vec3 PALM_VECTOR(0.0f, -1.0f, 0.0f);
        palm.setRawNormal(rotation * PALM_VECTOR);
        
        // initialize the "finger" based on the direction
        FingerData finger(&palm, &hand);
        finger.setActive(true);
        finger.setRawRootPosition(position);
        const glm::vec3 FINGER_VECTOR(0.0f, 0.0f, 100.0f);
        finger.setRawTipPosition(position + rotation * FINGER_VECTOR);
        
        // three fingers indicates to the skeleton that we have enough data to determine direction
        palm.getFingers().clear();
        palm.getFingers().push_back(finger);
        palm.getFingers().push_back(finger);
        palm.getFingers().push_back(finger);
        
        hand.getPalms().push_back(palm);
    }
#endif
}

