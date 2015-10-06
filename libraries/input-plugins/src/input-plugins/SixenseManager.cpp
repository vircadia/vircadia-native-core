//
//  SixenseManager.cpp
//  input-plugins/src/input-plugins
//
//  Created by Andrzej Kapolka on 11/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <vector>

#include <QCoreApplication>

#include <GLMHelpers.h>
#include <NumericalConstants.h>
#include <PerfStat.h>
#include <plugins/PluginContainer.h>

#include "NumericalConstants.h"
#include "SixenseManager.h"
#include "UserActivityLogger.h"

#ifdef HAVE_SIXENSE
    #include "sixense.h"
#endif

// TODO: This should not be here
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

// These bits aren't used for buttons, so they can be used as masks:
const unsigned int LEFT_MASK = 0;
const unsigned int RIGHT_MASK = 1U << 1;

#ifdef HAVE_SIXENSE

const int CALIBRATION_STATE_IDLE = 0;
const int CALIBRATION_STATE_X = 1;
const int CALIBRATION_STATE_COMPLETE = 2;

const glm::vec3 DEFAULT_AVATAR_POSITION(-0.25f, -0.35f, -0.3f); // in hydra frame

const float CONTROLLER_THRESHOLD = 0.35f;

#endif

#ifdef __APPLE__
typedef int (*SixenseBaseFunction)();
typedef int (*SixenseTakeIntFunction)(int);
#ifdef HAVE_SIXENSE
typedef int (*SixenseTakeIntAndSixenseControllerData)(int, sixenseControllerData*);
#endif
#endif

const QString SixenseManager::NAME = "Sixense";

const QString MENU_PARENT = "Avatar";
const QString MENU_NAME = "Sixense";
const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;
const QString TOGGLE_SMOOTH = "Smooth Sixense Movement";

SixenseManager& SixenseManager::getInstance() {
    static SixenseManager sharedInstance;
    return sharedInstance;
}

SixenseManager::SixenseManager() :
    InputDevice("Hydra"),
#ifdef __APPLE__
    _sixenseLibrary(NULL),
#endif
    _hydrasConnected(false)
{

}

bool SixenseManager::isSupported() const {
#ifdef HAVE_SIXENSE
    return true;
#else
    return false;
#endif
}

void SixenseManager::activate() {
#ifdef HAVE_SIXENSE
    _calibrationState = CALIBRATION_STATE_IDLE;
    _avatarPosition = DEFAULT_AVATAR_POSITION;

    CONTAINER->addMenu(MENU_PATH);
    CONTAINER->addMenuItem(MENU_PATH, TOGGLE_SMOOTH,
                           [this] (bool clicked) { this->setFilter(clicked); },
                           true, true);

#ifdef __APPLE__

    if (!_sixenseLibrary) {

#ifdef SIXENSE_LIB_FILENAME
        _sixenseLibrary = new QLibrary(SIXENSE_LIB_FILENAME);
#else
        const QString SIXENSE_LIBRARY_NAME = "libsixense_x64";
        QString frameworkSixenseLibrary = QCoreApplication::applicationDirPath() + "/../Frameworks/"
            + SIXENSE_LIBRARY_NAME;

        _sixenseLibrary = new QLibrary(frameworkSixenseLibrary);
#endif
    }

    if (_sixenseLibrary->load()){
        qCDebug(inputplugins) << "Loaded sixense library for hydra support -" << _sixenseLibrary->fileName();
    } else {
        qCDebug(inputplugins) << "Sixense library at" << _sixenseLibrary->fileName() << "failed to load."
            << "Continuing without hydra support.";
        return;
    }

    SixenseBaseFunction sixenseInit = (SixenseBaseFunction) _sixenseLibrary->resolve("sixenseInit");
#endif
    sixenseInit();
#endif
}

void SixenseManager::deactivate() {
#ifdef HAVE_SIXENSE
    CONTAINER->removeMenuItem(MENU_NAME, TOGGLE_SMOOTH);
    CONTAINER->removeMenu(MENU_PATH);

    _poseStateMap.clear();

#ifdef __APPLE__
    SixenseBaseFunction sixenseExit = (SixenseBaseFunction)_sixenseLibrary->resolve("sixenseExit");
#endif

    sixenseExit();

#ifdef __APPLE__
    delete _sixenseLibrary;
#endif

#endif
}

void SixenseManager::setFilter(bool filter) {
#ifdef HAVE_SIXENSE
#ifdef __APPLE__
    SixenseTakeIntFunction sixenseSetFilterEnabled = (SixenseTakeIntFunction) _sixenseLibrary->resolve("sixenseSetFilterEnabled");
#endif
    int newFilter = filter ? 1 : 0;
    sixenseSetFilterEnabled(newFilter);
#endif
}

void SixenseManager::update(float deltaTime, bool jointsCaptured) {
#ifdef HAVE_SIXENSE
    _buttonPressedMap.clear();

#ifdef __APPLE__
    SixenseBaseFunction sixenseGetNumActiveControllers =
    (SixenseBaseFunction) _sixenseLibrary->resolve("sixenseGetNumActiveControllers");
#endif

    auto userInputMapper = DependencyManager::get<UserInputMapper>();

    if (sixenseGetNumActiveControllers() == 0) {
        _hydrasConnected = false;
        if (_deviceID != 0) {
            userInputMapper->removeDevice(_deviceID);
            _deviceID = 0;
            _poseStateMap.clear();
        }
        return;
    }

    PerformanceTimer perfTimer("sixense");
    if (!_hydrasConnected) {
        _hydrasConnected = true;
        registerToUserInputMapper(*userInputMapper);
        assignDefaultInputMapping(*userInputMapper);
        UserActivityLogger::getInstance().connectedDevice("spatial_controller", "hydra");
    }

#ifdef __APPLE__
    SixenseBaseFunction sixenseGetMaxControllers =
    (SixenseBaseFunction) _sixenseLibrary->resolve("sixenseGetMaxControllers");
#endif

    int maxControllers = sixenseGetMaxControllers();

    // we only support two controllers
    sixenseControllerData controllers[2];

#ifdef __APPLE__
    SixenseTakeIntFunction sixenseIsControllerEnabled =
    (SixenseTakeIntFunction) _sixenseLibrary->resolve("sixenseIsControllerEnabled");

    SixenseTakeIntAndSixenseControllerData sixenseGetNewestData =
    (SixenseTakeIntAndSixenseControllerData) _sixenseLibrary->resolve("sixenseGetNewestData");
#endif

    int numActiveControllers = 0;
    for (int i = 0; i < maxControllers && numActiveControllers < 2; i++) {
        if (!sixenseIsControllerEnabled(i)) {
            continue;
        }
        sixenseControllerData* data = controllers + numActiveControllers;
        ++numActiveControllers;
        sixenseGetNewestData(i, data);

        // NOTE: Sixense API returns pos data in millimeters but we IMMEDIATELY convert to meters.
        glm::vec3 position(data->pos[0], data->pos[1], data->pos[2]);
        position *= METERS_PER_MILLIMETER;

        // Check to see if this hand/controller is on the base
        const float CONTROLLER_AT_BASE_DISTANCE = 0.075f;
        if (glm::length(position) >= CONTROLLER_AT_BASE_DISTANCE) {
            handleButtonEvent(data->buttons, numActiveControllers - 1);
            handleAxisEvent(data->joystick_x, data->joystick_y, data->trigger, numActiveControllers - 1);

            if (!jointsCaptured) {
                //  Rotation of Palm
                glm::quat rotation(data->rot_quat[3], data->rot_quat[0], data->rot_quat[1], data->rot_quat[2]);
                handlePoseEvent(position, rotation, numActiveControllers - 1);
            } else {
                _poseStateMap.clear();
            }
        } else {
            _poseStateMap[(numActiveControllers - 1) == 0 ? LEFT_HAND : RIGHT_HAND] = UserInputMapper::PoseValue();
        }

//            //  Read controller buttons and joystick into the hand
//            palm->setControllerButtons(data->buttons);
//            palm->setTrigger(data->trigger);
//            palm->setJoystick(data->joystick_x, data->joystick_y);
    }

    if (numActiveControllers == 2) {
        updateCalibration(controllers);
    }

    for (auto axisState : _axisStateMap) {
        if (fabsf(axisState.second) < CONTROLLER_THRESHOLD) {
            _axisStateMap[axisState.first] = 0.0f;
        }
    }
#endif  // HAVE_SIXENSE
}

#ifdef HAVE_SIXENSE

// the calibration sequence is:
// (1) reach arm straight out to the sides (xAxis is to the left)
// (2) press BUTTON_FWD on both hands and hold for one second
// (3) release both BUTTON_FWDs
//
// The code will:
// (4) assume that the orb is on a flat surface (yAxis is UP)
// (5) compute the forward direction (zAxis = xAxis cross yAxis)

const float MINIMUM_ARM_REACH = 0.3f; // meters
const float MAXIMUM_NOISE_LEVEL = 0.05f; // meters
const quint64 LOCK_DURATION = USECS_PER_SECOND / 4;     // time for lock to be acquired

void SixenseManager::updateCalibration(void* controllersX) {
    auto controllers = reinterpret_cast<sixenseControllerData*>(controllersX);
    const sixenseControllerData* dataLeft = controllers;
    const sixenseControllerData* dataRight = controllers + 1;

    // calibration only happpens while both hands are holding BUTTON_FORWARD
    if (dataLeft->buttons != BUTTON_FWD || dataRight->buttons != BUTTON_FWD) {
        if (_calibrationState == CALIBRATION_STATE_IDLE) {
            return;
        }
        switch (_calibrationState) {
            case CALIBRATION_STATE_COMPLETE:
            {
                // compute calibration results
                _avatarPosition = - 0.5f * (_reachLeft + _reachRight); // neck is midway between right and left hands
                glm::vec3 xAxis = glm::normalize(_reachRight - _reachLeft);
                glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, Vectors::UNIT_Y));
                xAxis = glm::normalize(glm::cross(Vectors::UNIT_Y, zAxis));
                _avatarRotation = glm::inverse(glm::quat_cast(glm::mat3(xAxis, Vectors::UNIT_Y, zAxis)));
                const float Y_OFFSET_CALIBRATED_HANDS_TO_AVATAR = -0.3f;
                _avatarPosition.y += Y_OFFSET_CALIBRATED_HANDS_TO_AVATAR;
                CONTAINER->requestReset();
                qCDebug(inputplugins, "succeess: sixense calibration");
            }
            break;
            default:
                qCDebug(inputplugins, "failed: sixense calibration");
                break;
        }

        _calibrationState = CALIBRATION_STATE_IDLE;
        return;
    }

    // NOTE: Sixense API returns pos data in millimeters but we IMMEDIATELY convert to meters.
    const float* pos = dataLeft->pos;
    glm::vec3 positionLeft(pos[0], pos[1], pos[2]);
    positionLeft *= METERS_PER_MILLIMETER;
    pos = dataRight->pos;
    glm::vec3 positionRight(pos[0], pos[1], pos[2]);
    positionRight *= METERS_PER_MILLIMETER;

    if (_calibrationState == CALIBRATION_STATE_IDLE) {
        float reach = glm::distance(positionLeft, positionRight);
        if (reach > 2.0f * MINIMUM_ARM_REACH) {
            qCDebug(inputplugins, "started: sixense calibration");
            _averageLeft = positionLeft;
            _averageRight = positionRight;
            _reachLeft = _averageLeft;
            _reachRight = _averageRight;
            _lastDistance = reach;
            _lockExpiry = usecTimestampNow() + LOCK_DURATION;
            // move to next state
            _calibrationState = CALIBRATION_STATE_X;
        }
        return;
    }

    quint64 now = usecTimestampNow() + LOCK_DURATION;
    // these are weighted running averages
    _averageLeft = 0.9f * _averageLeft + 0.1f * positionLeft;
    _averageRight = 0.9f * _averageRight + 0.1f * positionRight;

    if (_calibrationState == CALIBRATION_STATE_X) {
        // compute new sliding average
        float distance = glm::distance(_averageLeft, _averageRight);
        if (fabsf(distance - _lastDistance) > MAXIMUM_NOISE_LEVEL) {
            // distance is increasing so acquire the data and push the expiry out
            _reachLeft = _averageLeft;
            _reachRight = _averageRight;
            _lastDistance = distance;
            _lockExpiry = now + LOCK_DURATION;
        } else if (now > _lockExpiry) {
            // lock has expired so clamp the data and move on
            _lockExpiry = now + LOCK_DURATION;
            _lastDistance = 0.0f;
            _reachUp = 0.5f * (_reachLeft + _reachRight);
            _calibrationState = CALIBRATION_STATE_COMPLETE;
            qCDebug(inputplugins, "success: sixense calibration: left");
        }
    }
}

#endif  // HAVE_SIXENSE

void SixenseManager::focusOutEvent() {
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void SixenseManager::handleAxisEvent(float stickX, float stickY, float trigger, int index) {
    _axisStateMap[makeInput(AXIS_Y_POS, index).getChannel()] = (stickY > 0.0f) ? stickY : 0.0f;
    _axisStateMap[makeInput(AXIS_Y_NEG, index).getChannel()] = (stickY < 0.0f) ? -stickY : 0.0f;
    _axisStateMap[makeInput(AXIS_X_POS, index).getChannel()] = (stickX > 0.0f) ? stickX : 0.0f;
    _axisStateMap[makeInput(AXIS_X_NEG, index).getChannel()] = (stickX < 0.0f) ? -stickX : 0.0f;
    _axisStateMap[makeInput(BACK_TRIGGER, index).getChannel()] = trigger;
}

void SixenseManager::handleButtonEvent(unsigned int buttons, int index) {
    if (buttons & BUTTON_0) {
        _buttonPressedMap.insert(makeInput(BUTTON_0, index).getChannel());
    }
    if (buttons & BUTTON_1) {
        _buttonPressedMap.insert(makeInput(BUTTON_1, index).getChannel());
    }
    if (buttons & BUTTON_2) {
        _buttonPressedMap.insert(makeInput(BUTTON_2, index).getChannel());
    }
    if (buttons & BUTTON_3) {
        _buttonPressedMap.insert(makeInput(BUTTON_3, index).getChannel());
    }
    if (buttons & BUTTON_4) {
        _buttonPressedMap.insert(makeInput(BUTTON_4, index).getChannel());
    }
    if (buttons & BUTTON_FWD) {
        _buttonPressedMap.insert(makeInput(BUTTON_FWD, index).getChannel());
    }
    if (buttons & BUTTON_TRIGGER) {
        _buttonPressedMap.insert(makeInput(BUTTON_TRIGGER, index).getChannel());
    }
}

void SixenseManager::handlePoseEvent(glm::vec3 position, glm::quat rotation, int index) {
#ifdef HAVE_SIXENSE
    // From ABOVE the sixense coordinate frame looks like this:
    //
    //       |
    //   USB cables
    //       |
    //      .-.                             user
    //     (Orb) --neckX----               forward
    //      '-'                               |
    //       |                                |     user
    //     neckZ          y                   +---- right
    //       |             (o)-----x
    //                      |
    //                      |
    //                      z

    // Transform the measured position into body frame.
    position = _avatarRotation * (position + _avatarPosition);

    // From ABOVE the hand canonical axes look like this:
    //
    //      | | | |          y        | | | |
    //      | | | |          |        | | | |
    //      |     |          |        |     |
    //      |left | /  x----(+)     \ |right|
    //      |     _/           z     \_     |
    //       |   |                     |   |
    //       |   |                     |   |
    //

    // To convert sixense's delta-rotation into the hand's frame we will have to transform it like so:
    //
    //     deltaHand = Qsh^ * deltaSixense * Qsh
    //
    // where  Qsh = transform from sixense axes to hand axes.  By inspection we can determine Qsh:
    //
    //     Qsh = angleAxis(PI, zAxis) * angleAxis(-PI/2, xAxis)
    //
    const glm::quat sixenseToHand = glm::angleAxis(PI, Vectors::UNIT_Z) * glm::angleAxis(-PI/2.0f, Vectors::UNIT_X);

    // In addition to Qsh each hand has pre-offset introduced by the shape of the sixense controllers
    // and how they fit into the hand in their relaxed state.  This offset is a quarter turn about
    // the sixense's z-axis, with its direction different for the two hands:
    float sign = (index == 0) ? 1.0f : -1.0f;
    const glm::quat preOffset = glm::angleAxis(sign * PI / 2.0f, Vectors::UNIT_Z);

    // Finally, there is a post-offset (same for both hands) to get the hand's rest orientation
    // (fingers forward, palm down) aligned properly in the avatar's model-frame,
    // and then a flip about the yAxis to get into model-frame.
    const glm::quat postOffset = glm::angleAxis(PI, Vectors::UNIT_Y) * glm::angleAxis(PI / 2.0f, Vectors::UNIT_X);

    // The total rotation of the hand uses the formula:
    //
    //     rotation = postOffset * Qsh^ * (measuredRotation * preOffset) * Qsh
    //
    // TODO: find a shortcut with fewer rotations.
    rotation = _avatarRotation * postOffset * glm::inverse(sixenseToHand) * rotation * preOffset * sixenseToHand;

    _poseStateMap[makeInput(JointChannel(index)).getChannel()] = UserInputMapper::PoseValue(position, rotation);
#endif // HAVE_SIXENSE
}

void SixenseManager::registerToUserInputMapper(UserInputMapper& mapper) {
    // Grab the current free device ID
    _deviceID = mapper.getFreeDeviceID();

    auto proxy = std::make_shared<UserInputMapper::DeviceProxy>(_name);
    proxy->getButton = [this] (const UserInputMapper::Input& input, int timestamp) -> bool { return this->getButton(input.getChannel()); };
    proxy->getAxis = [this] (const UserInputMapper::Input& input, int timestamp) -> float { return this->getAxis(input.getChannel()); };
    proxy->getPose = [this](const UserInputMapper::Input& input, int timestamp) -> UserInputMapper::PoseValue { return this->getPose(input.getChannel()); };
    proxy->getAvailabeInputs = [this] () -> QVector<UserInputMapper::InputPair> {
        QVector<UserInputMapper::InputPair> availableInputs;
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_0, 0), "Left Start"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_1, 0), "Left Button 1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_2, 0), "Left Button 2"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_3, 0), "Left Button 3"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_4, 0), "Left Button 4"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_FWD, 0), "L1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BACK_TRIGGER, 0), "L2"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_POS, 0), "Left Stick Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_NEG, 0), "Left Stick Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_POS, 0), "Left Stick Right"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_NEG, 0), "Left Stick Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_TRIGGER, 0), "Left Trigger Press"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_0, 1), "Right Start"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_1, 1), "Right Button 1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_2, 1), "Right Button 2"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_3, 1), "Right Button 3"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_4, 1), "Right Button 4"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_FWD, 1), "R1"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BACK_TRIGGER, 1), "R2"));

        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_POS, 1), "Right Stick Up"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_Y_NEG, 1), "Right Stick Down"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_POS, 1), "Right Stick Right"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(AXIS_X_NEG, 1), "Right Stick Left"));
        availableInputs.append(UserInputMapper::InputPair(makeInput(BUTTON_TRIGGER, 1), "Right Trigger Press"));

        return availableInputs;
    };
    proxy->resetDeviceBindings = [this, &mapper] () -> bool {
        mapper.removeAllInputChannelsForDevice(_deviceID);
        this->assignDefaultInputMapping(mapper);
        return true;
    };
    mapper.registerDevice(_deviceID, proxy);
}

void SixenseManager::assignDefaultInputMapping(UserInputMapper& mapper) {
    const float JOYSTICK_MOVE_SPEED = 1.0f;
    const float JOYSTICK_YAW_SPEED = 0.5f;
    const float JOYSTICK_PITCH_SPEED = 0.25f;
    const float BUTTON_MOVE_SPEED = 1.0f;
    const float BOOM_SPEED = 0.1f;

    // Left Joystick: Movement, strafing
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_FORWARD, makeInput(AXIS_Y_POS, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LONGITUDINAL_BACKWARD, makeInput(AXIS_Y_NEG, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_RIGHT, makeInput(AXIS_X_POS, 0), JOYSTICK_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::LATERAL_LEFT, makeInput(AXIS_X_NEG, 0), JOYSTICK_MOVE_SPEED);

    // Right Joystick: Camera orientation
    mapper.addInputChannel(UserInputMapper::YAW_RIGHT, makeInput(AXIS_X_POS, 1), JOYSTICK_YAW_SPEED);
    mapper.addInputChannel(UserInputMapper::YAW_LEFT, makeInput(AXIS_X_NEG, 1), JOYSTICK_YAW_SPEED);
    mapper.addInputChannel(UserInputMapper::PITCH_UP, makeInput(AXIS_Y_POS, 1), JOYSTICK_PITCH_SPEED);
    mapper.addInputChannel(UserInputMapper::PITCH_DOWN, makeInput(AXIS_Y_NEG, 1), JOYSTICK_PITCH_SPEED);

    // Buttons
    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(BUTTON_3, 0), BOOM_SPEED);
    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(BUTTON_1, 0), BOOM_SPEED);

    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(BUTTON_3, 1), BUTTON_MOVE_SPEED);
    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(BUTTON_1, 1), BUTTON_MOVE_SPEED);

    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(BUTTON_2, 0));
    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(BUTTON_2, 1));

    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(BUTTON_4, 0));
    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(BUTTON_4, 1));

    mapper.addInputChannel(UserInputMapper::LEFT_HAND, makeInput(LEFT_HAND));
    mapper.addInputChannel(UserInputMapper::RIGHT_HAND, makeInput(RIGHT_HAND));

    mapper.addInputChannel(UserInputMapper::LEFT_HAND_CLICK, makeInput(BACK_TRIGGER, 0));
    mapper.addInputChannel(UserInputMapper::RIGHT_HAND_CLICK, makeInput(BACK_TRIGGER, 1));

    // TODO find a mechanism to allow users to navigate the context menu via
    mapper.addInputChannel(UserInputMapper::CONTEXT_MENU, makeInput(BUTTON_0, 0));
    mapper.addInputChannel(UserInputMapper::TOGGLE_MUTE, makeInput(BUTTON_0, 1));

}

UserInputMapper::Input SixenseManager::makeInput(unsigned int button, int index) {
    return UserInputMapper::Input(_deviceID, button | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input SixenseManager::makeInput(SixenseManager::JoystickAxisChannel axis, int index) {
    return UserInputMapper::Input(_deviceID, axis | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::AXIS);
}

UserInputMapper::Input SixenseManager::makeInput(JointChannel joint) {
    return UserInputMapper::Input(_deviceID, joint, UserInputMapper::ChannelType::POSE);
}
