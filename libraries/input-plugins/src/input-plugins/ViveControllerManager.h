//
//  ViveControllerManager.h
//  interface/src/devices
//
//  Created by Sam Gondelman on 6/29/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi__ViveControllerManager
#define hifi__ViveControllerManager

#include <QObject>
#include <unordered_set>

#include <GLMHelpers.h>

#include "UserInputMapper.h"

class ViveControllerManager : public QObject {
    Q_OBJECT
    
public:
    enum JoystickAxisChannel {
        AXIS_Y_POS = 1U << 1,
        AXIS_Y_NEG = 1U << 2,
        AXIS_X_POS = 1U << 3,
        AXIS_X_NEG = 1U << 4,
        BACK_TRIGGER = 1U << 5,
    };

    enum Axis {
        TRACKPAD_AXIS = 0,
        TRIGGER_AXIS,
        AXIS_2,
        AXIS_3,
        AXIS_4,
    };
    
    enum JointChannel {
        LEFT_HAND = 0,
        RIGHT_HAND,
    };
    
    void focusOutEvent();
    
    void update();
    
    static ViveControllerManager& getInstance();
    
    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, float> AxisStateMap;
    typedef std::map<int, UserInputMapper::PoseValue> PoseStateMap;
    
    float getButton(int channel) const;
    float getAxis(int channel) const;
    UserInputMapper::PoseValue getPose(int channel) const;
    
    UserInputMapper::Input makeInput(unsigned int button, int index);
    UserInputMapper::Input makeInput(JoystickAxisChannel axis, int index);
    UserInputMapper::Input makeInput(JointChannel joint);
    
    void registerToUserInputMapper(UserInputMapper& mapper);
    void assignDefaultInputMapping(UserInputMapper& mapper);
    
    void activate();
    
private:
    ViveControllerManager();
    ~ViveControllerManager();
    
    void handleButtonEvent(uint64_t buttons, int index);
    void handleAxisEvent(Axis axis, float x, float y, int index);
    void handlePoseEvent(const mat4& mat, int index);
    
    bool _isInitialized;
    bool _isEnabled;
    int _trackedControllers;
    
protected:
    int _deviceID = 0;
    
    ButtonPressedMap _buttonPressedMap;
    AxisStateMap _axisStateMap;
    PoseStateMap _poseStateMap;
};


#endif // hifi__ViveControllerManager
