//
//  Sixense.cpp
//  interface
//
//  Created by Andrzej Kapolka on 11/15/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "sixense.h"

#include "Application.h"
#include "SixenseManager.h"

SixenseManager::SixenseManager() {
    sixenseInit();
}

SixenseManager::~SixenseManager() {
    sixenseExit();
}
    
void SixenseManager::update() {
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
        PalmData palm(&hand);
        palm.setActive(true);
        glm::vec3 position(-data.pos[0], data.pos[1], -data.pos[2]);
        palm.setRawPosition(position);
        glm::quat rotation(data.rot_quat[3], -data.rot_quat[0], data.rot_quat[1], -data.rot_quat[2]);
        palm.setRawNormal(rotation * glm::vec3(0.0f, -1.0f, 0.0f));
        FingerData finger(&palm, &hand);
        finger.setActive(true);
        finger.setRawRootPosition(position);
        finger.setRawTipPosition(position + rotation * glm::vec3(0.0f, 0.0f, 100.0f));
        palm.getFingers().clear();
        palm.getFingers().push_back(finger);
        palm.getFingers().push_back(finger);
        palm.getFingers().push_back(finger);
        hand.getPalms().push_back(palm);
    }   
}

