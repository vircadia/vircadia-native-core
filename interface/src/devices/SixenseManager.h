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

#ifndef hifi_SixenseManager_h
#define hifi_SixenseManager_h

#include <QObject>

#ifdef HAVE_SIXENSE
    #include <glm/glm.hpp>
    #include <glm/gtc/quaternion.hpp>
    #include "sixense.h"

#ifdef __APPLE__
    #include <qlibrary.h>
#endif

#endif

const unsigned int BUTTON_0 = 1U << 0; // the skinny button between 1 and 2
const unsigned int BUTTON_1 = 1U << 5;
const unsigned int BUTTON_2 = 1U << 6;
const unsigned int BUTTON_3 = 1U << 3;
const unsigned int BUTTON_4 = 1U << 4;
const unsigned int BUTTON_FWD = 1U << 7;

// Event type that represents using the controller
const unsigned int CONTROLLER_0_EVENT = 1500U;
const unsigned int CONTROLLER_1_EVENT = 1501U;

const float DEFAULT_SIXENSE_RETICLE_MOVE_SPEED = 37.5f;
const bool DEFAULT_INVERT_SIXENSE_MOUSE_BUTTONS = false;

/// Handles interaction with the Sixense SDK (e.g., Razer Hydra).
class SixenseManager : public QObject {
    Q_OBJECT
public:
    static SixenseManager& getInstance();
    
    void initialize();
    bool isInitialized() const { return _isInitialized; }
    
    void setIsEnabled(bool isEnabled) { _isEnabled = isEnabled; }
    
    void update(float deltaTime);
    float getCursorPixelRangeMult() const;
    
public slots:
    
    void setFilter(bool filter);
    void setLowVelocityFilter(bool lowVelocityFilter) { _lowVelocityFilter = lowVelocityFilter; };

private:
    SixenseManager();
    ~SixenseManager();
    
#ifdef HAVE_SIXENSE
    void updateCalibration(const sixenseControllerData* controllers);
    void emulateMouse(PalmData* palm, int index);

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
    
#ifdef __APPLE__
    QLibrary* _sixenseLibrary;
#endif
    
#endif
    bool _isInitialized;
    bool _isEnabled;
    bool _hydrasConnected;
    quint64 _lastMovement;
    glm::vec3 _amountMoved;

    // for mouse emulation with the two controllers
    bool _triggerPressed[2];
    bool _bumperPressed[2];
    int _oldX[2];
    int _oldY[2];
    
    bool _lowVelocityFilter;
};

#endif // hifi_SixenseManager_h
