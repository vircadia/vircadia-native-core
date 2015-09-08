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

#include <PerfStat.h>
#include <NumericalConstants.h>

#include "NumericalConstants.h"
#include <plugins/PluginContainer.h>
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
const int CALIBRATION_STATE_Y = 2;
const int CALIBRATION_STATE_Z = 3;
const int CALIBRATION_STATE_COMPLETE = 4;

// default (expected) location of neck in sixense space
const float NECK_X = 0.25f; // meters
const float NECK_Y = 0.3f;  // meters
const float NECK_Z = 0.3f;  // meters

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
    // By default we assume the _neckBase (in orb frame) is as high above the orb
    // as the "torso" is below it.
    _neckBase = glm::vec3(NECK_X, -NECK_Y, NECK_Z);

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
// (1) press BUTTON_FWD on both hands
// (2) reach arm straight out to the side (X)
// (3) lift arms staight up above head (Y)
// (4) move arms a bit forward (Z)
// (5) release BUTTON_FWD on both hands

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
            case CALIBRATION_STATE_Y:
            case CALIBRATION_STATE_Z:
            case CALIBRATION_STATE_COMPLETE:
            {
                // compute calibration results
                // ATM we only handle the case where the XAxis has been measured, and we assume the rest
                // (i.e. that the orb is on a level surface)
                // TODO: handle COMPLETE state where all three axes have been defined.  This would allow us
                // to also handle the case where left and right controllers have been reversed.
                _neckBase = 0.5f * (_reachLeft + _reachRight); // neck is midway between right and left reaches
                glm::vec3 xAxis = glm::normalize(_reachRight - _reachLeft);
                glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
                glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, yAxis));
                xAxis = glm::normalize(glm::cross(yAxis, zAxis));
                _orbRotation = glm::inverse(glm::quat_cast(glm::mat3(xAxis, yAxis, zAxis)));
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
            _calibrationState = CALIBRATION_STATE_Y;
            qCDebug(inputplugins, "success: sixense calibration: left");
        }
    }
    else if (_calibrationState == CALIBRATION_STATE_Y) {
        glm::vec3 torso = 0.5f * (_reachLeft + _reachRight);
        glm::vec3 averagePosition = 0.5f * (_averageLeft + _averageRight);
        float distance = (averagePosition - torso).y;
        if (fabsf(distance) > fabsf(_lastDistance) + MAXIMUM_NOISE_LEVEL) {
            // distance is increasing so acquire the data and push the expiry out
            _reachUp = averagePosition;
            _lastDistance = distance;
            _lockExpiry = now + LOCK_DURATION;
        } else if (now > _lockExpiry) {
            if (_lastDistance > MINIMUM_ARM_REACH) {
                // lock has expired so clamp the data and move on
                _reachForward = _reachUp;
                _lastDistance = 0.0f;
                _lockExpiry = now + LOCK_DURATION;
                _calibrationState = CALIBRATION_STATE_Z;
                qCDebug(inputplugins, "success: sixense calibration: up");
            }
        }
    }
    else if (_calibrationState == CALIBRATION_STATE_Z) {
        glm::vec3 xAxis = glm::normalize(_reachRight - _reachLeft);
        glm::vec3 torso = 0.5f * (_reachLeft + _reachRight);
        //glm::vec3 yAxis = glm::normalize(_reachUp - torso);
        glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
        glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, yAxis));

        glm::vec3 averagePosition = 0.5f * (_averageLeft + _averageRight);
        float distance = glm::dot((averagePosition - torso), zAxis);
        if (fabs(distance) > fabs(_lastDistance)) {
            // distance is increasing so acquire the data and push the expiry out
            _reachForward = averagePosition;
            _lastDistance = distance;
            _lockExpiry = now + LOCK_DURATION;
        } else if (now > _lockExpiry) {
            if (fabsf(_lastDistance) > 0.05f * MINIMUM_ARM_REACH) {
                // lock has expired so clamp the data and move on
                _calibrationState = CALIBRATION_STATE_COMPLETE;
                qCDebug(inputplugins, "success: sixense calibration: forward");
                // TODO: it is theoretically possible to detect that the controllers have been
                // accidentally switched (left hand is holding right controller) and to swap the order.
            }
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
    glm::vec3 neck = _neckBase;
    // Set y component of the "neck" to raise the measured position a little bit.
    neck.y = 0.5f;
    position = _orbRotation * (position - neck);

    // From ABOVE the hand canonical axes looks like this:
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
    const glm::vec3 xAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 yAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 zAxis = glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::quat sixenseToHand = glm::angleAxis(PI, zAxis) * glm::angleAxis(-PI/2.0f, xAxis);

    // In addition to Qsh each hand has pre-offset introduced by the shape of the sixense controllers
    // and how they fit into the hand in their relaxed state.  This offset is a quarter turn about
    // the sixense's z-axis, with its direction different for the two hands:
    float sign = (index == 0) ? 1.0f : -1.0f;
    const glm::quat preOffset = glm::angleAxis(sign * PI / 2.0f, zAxis);

    // Finally, there is a post-offset (same for both hands) to get the hand's rest orientation
    // (fingers forward, palm down) aligned properly in the avatar's model-frame,
    // and then a flip about the yAxis to get into model-frame.
    const glm::quat postOffset = glm::angleAxis(PI, yAxis) * glm::angleAxis(PI / 2.0f, xAxis);

    // The total rotation of the hand uses the formula:
    //
    //     rotation = postOffset * Qsh^ * (measuredRotation * preOffset) * Qsh
    //
    // TODO: find a shortcut with fewer rotations.
    rotation = postOffset * glm::inverse(sixenseToHand) * rotation * preOffset * sixenseToHand;

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
