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
#include <unordered_set>

#ifdef HAVE_SIXENSE
    #include <glm/glm.hpp>
    #include <glm/gtc/quaternion.hpp>
    #include "sixense.h"

#ifdef __APPLE__
    #include <qlibrary.h>
#endif

#endif

#include "ui/UserInputMapper.h"

class PalmData;

const unsigned int BUTTON_0 = 1U << 0; // the skinny button between 1 and 2
const unsigned int BUTTON_1 = 1U << 5;
const unsigned int BUTTON_2 = 1U << 6;
const unsigned int BUTTON_3 = 1U << 3;
const unsigned int BUTTON_4 = 1U << 4;
const unsigned int BUTTON_FWD = 1U << 7;
const unsigned int BUTTON_TRIGGER = 1U << 8;

// Event type that represents using the controller
const unsigned int CONTROLLER_0_EVENT = 1500U;
const unsigned int CONTROLLER_1_EVENT = 1501U;

const float DEFAULT_SIXENSE_RETICLE_MOVE_SPEED = 37.5f;
const bool DEFAULT_INVERT_SIXENSE_MOUSE_BUTTONS = false;

/// Handles interaction with the Sixense SDK (e.g., Razer Hydra).
class SixenseManager : public QObject {
    Q_OBJECT
public:
    enum JoystickAxisChannel {
        AXIS_Y_POS = 1U << 0,
        AXIS_Y_NEG = 1U << 3,
        AXIS_X_POS = 1U << 4,
        AXIS_X_NEG = 1U << 5,
        BACK_TRIGGER = 1U << 6,
    };
    
    static SixenseManager& getInstance();
    
    void initialize();
    bool isInitialized() const { return _isInitialized; }
    
    void setIsEnabled(bool isEnabled) { _isEnabled = isEnabled; }
    
    void update(float deltaTime);
    float getCursorPixelRangeMult() const;
    
    float getReticleMoveSpeed() const { return _reticleMoveSpeed; }
    void setReticleMoveSpeed(float sixenseReticleMoveSpeed) { _reticleMoveSpeed = sixenseReticleMoveSpeed; }
    bool getInvertButtons() const { return _invertButtons; }
    void setInvertButtons(bool invertSixenseButtons) { _invertButtons = invertSixenseButtons; }
    
    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, float> AxisStateMap;
    
    float getButton(int channel) const;
    float getAxis(int channel) const;
    
    UserInputMapper::Input makeInput(unsigned int button, int index);
    UserInputMapper::Input makeInput(JoystickAxisChannel axis, int index);
    
    void registerToUserInputMapper(UserInputMapper& mapper);
    void assignDefaultInputMapping(UserInputMapper& mapper);
    
    void update();
    void focusOutEvent();
    
public slots:
    void toggleSixense(bool shouldEnable);
    void setFilter(bool filter);
    void setLowVelocityFilter(bool lowVelocityFilter) { _lowVelocityFilter = lowVelocityFilter; };

private:
    SixenseManager();
    ~SixenseManager();
    
    void handleButtonEvent(unsigned int buttons, int index);
    void handleAxisEvent(float x, float y, float trigger, int index);
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

    // for mouse emulation with the two controllers
    bool _triggerPressed[2];
    bool _bumperPressed[2];
    int _oldX[2];
    int _oldY[2];
    PalmData* _prevPalms[2];
    
    bool _lowVelocityFilter;
    bool _controllersAtBase;
    
    float _reticleMoveSpeed = DEFAULT_SIXENSE_RETICLE_MOVE_SPEED;
    bool _invertButtons = DEFAULT_INVERT_SIXENSE_MOUSE_BUTTONS;

protected:
    int _deviceID = 0;
    
    ButtonPressedMap _buttonPressedMap;
    AxisStateMap _axisStateMap;
};

#endif // hifi_SixenseManager_h
