//
//  SixenseManager.h
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 11/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __interface__SixenseManager__
#define __interface__SixenseManager__

#include <QObject>

#ifdef HAVE_SIXENSE
    #include <glm/glm.hpp>
    #include <glm/gtc/quaternion.hpp>
    #include "sixense.h"
#endif

const unsigned int BUTTON_0 = 1U << 0; // the skinny button between 1 and 2
const unsigned int BUTTON_1 = 1U << 5;
const unsigned int BUTTON_2 = 1U << 6;
const unsigned int BUTTON_3 = 1U << 3;
const unsigned int BUTTON_4 = 1U << 4;
const unsigned int BUTTON_FWD = 1U << 7;

/// Handles interaction with the Sixense SDK (e.g., Razer Hydra).
class SixenseManager : public QObject {
    Q_OBJECT
public:
    
    SixenseManager();
    ~SixenseManager();
    
    void update(float deltaTime);
    
public slots:
    
    void setFilter(bool filter);

private:
#ifdef HAVE_SIXENSE
    void updateCalibration(const sixenseControllerData* controllers);

    int _calibrationState;

    // these are calibration results
    glm::vec3 _neckBase;    // midpoint between controllers during X-axis calibration
    glm::quat _orbRotation; // rotates from orb frame into body frame
    float _armLength;

    // these are measured values used to compute the calibration results
    quint64 _lockExpiry;
    glm::vec3 _averageLeft;
    glm::vec3 _averageRight;
    glm::vec3 _reachLeft;
    glm::vec3 _reachRight;
    glm::vec3 _reachUp;
    glm::vec3 _reachForward;
    float _lastDistance;

#endif
    quint64 _lastMovement;
};

#endif /* defined(__interface__SixenseManager__) */

