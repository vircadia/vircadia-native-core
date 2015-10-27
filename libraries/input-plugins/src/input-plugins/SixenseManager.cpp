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
#include <SettingHandle.h>
#include <plugins/PluginContainer.h>
#include <PathUtils.h>
#include <NumericalConstants.h>
#include <UserActivityLogger.h>
#include <controllers/UserInputMapper.h>

#include "SixenseManager.h"


#ifdef HAVE_SIXENSE
    #include "sixense.h"
#endif

// TODO: This should not be here
#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

#ifdef HAVE_SIXENSE

const int CALIBRATION_STATE_IDLE = 0;
const int CALIBRATION_STATE_IN_PROGRESS = 1;
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
const QString SixenseManager::HYDRA_ID_STRING = "Razer Hydra";

const QString MENU_PARENT = "Avatar";
const QString MENU_NAME = "Sixense";
const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;
const QString TOGGLE_SMOOTH = "Smooth Sixense Movement";
const float DEFAULT_REACH_LENGTH = 1.5f;


SixenseManager::SixenseManager() :
    InputDevice("Hydra"),
    _reachLength(DEFAULT_REACH_LENGTH) 
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
    InputPlugin::activate();
#ifdef HAVE_SIXENSE
    _calibrationState = CALIBRATION_STATE_IDLE;
    _avatarPosition = DEFAULT_AVATAR_POSITION;

    CONTAINER->addMenu(MENU_PATH);
    CONTAINER->addMenuItem(MENU_PATH, TOGGLE_SMOOTH,
                           [this] (bool clicked) { this->setSixenseFilter(clicked); },
                           true, true);

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(this);

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
    loadSettings();
    sixenseInit();
#endif
}

void SixenseManager::deactivate() {
    InputPlugin::deactivate();
#ifdef HAVE_SIXENSE
    CONTAINER->removeMenuItem(MENU_NAME, TOGGLE_SMOOTH);
    CONTAINER->removeMenu(MENU_PATH);

    _poseStateMap.clear();
    _collectedSamples.clear();

    if (_deviceID != controller::Input::INVALID_DEVICE) {
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->removeDevice(_deviceID);
        _deviceID = controller::Input::INVALID_DEVICE;
    }

#ifdef __APPLE__
    SixenseBaseFunction sixenseExit = (SixenseBaseFunction)_sixenseLibrary->resolve("sixenseExit");
#endif

    sixenseExit();

#ifdef __APPLE__
    delete _sixenseLibrary;
#endif

#endif
}

void SixenseManager::setSixenseFilter(bool filter) {
#ifdef HAVE_SIXENSE
#ifdef __APPLE__
    SixenseTakeIntFunction sixenseSetFilterEnabled = (SixenseTakeIntFunction) _sixenseLibrary->resolve("sixenseSetFilterEnabled");
#endif
    int newFilter = filter ? 1 : 0;
    sixenseSetFilterEnabled(newFilter);
#endif
}

void SixenseManager::update(float deltaTime, bool jointsCaptured) {
    // FIXME - Some of the code in update() will crash if you haven't actually activated the
    // plugin. But we want register with the UserInputMapper if we don't call this.
    // We need to clean this up.
    //if (!_activated) {
    //    return;
    //}
#ifdef HAVE_SIXENSE
    _buttonPressedMap.clear();

#ifdef __APPLE__
    SixenseBaseFunction sixenseGetNumActiveControllers =
    (SixenseBaseFunction) _sixenseLibrary->resolve("sixenseGetNumActiveControllers");
#endif

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();

    static const float MAX_DISCONNECTED_TIME = 2.0f;
    static bool disconnected { false };
    static float disconnectedInterval { 0.0f };
    if (sixenseGetNumActiveControllers() == 0) {
        if (!disconnected) {
            disconnectedInterval += deltaTime;
        }
        if (disconnectedInterval > MAX_DISCONNECTED_TIME) {
            disconnected = true;
            _axisStateMap.clear();
            _buttonPressedMap.clear();
            _poseStateMap.clear();
            _collectedSamples.clear();
        }
        return;
    }

    if (disconnected) {
        disconnected = 0;
        disconnectedInterval = 0.0f;
    }

    PerformanceTimer perfTimer("sixense");
    // FIXME send this message once when we've positively identified hydra hardware
    //UserActivityLogger::getInstance().connectedDevice("spatial_controller", "hydra");

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
        bool left = i == 0;
        using namespace controller;
        // Check to see if this hand/controller is on the base
        const float CONTROLLER_AT_BASE_DISTANCE = 0.075f;
        if (glm::length(position) >= CONTROLLER_AT_BASE_DISTANCE) {
            handleButtonEvent(data->buttons, left);
            _axisStateMap[left ? LX : RX] = data->joystick_x;
            _axisStateMap[left ? LY : RY] = data->joystick_y;
            _axisStateMap[left ? LT : RT] = data->trigger;

            if (!jointsCaptured) {
                //  Rotation of Palm
                glm::quat rotation(data->rot_quat[3], data->rot_quat[0], data->rot_quat[1], data->rot_quat[2]);
                handlePoseEvent(deltaTime, position, rotation, left);
                
            } else {
                _poseStateMap.clear();
                _collectedSamples.clear();
            }
        } else {
            auto hand = left ? controller::StandardPoseChannel::LEFT_HAND : controller::StandardPoseChannel::RIGHT_HAND;
            _poseStateMap[hand] = controller::Pose();
            _collectedSamples[hand].first.clear();
            _collectedSamples[hand].second.clear();
        }
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
                _reachLength = glm::dot(xAxis, _reachRight - _reachLeft);
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
            _calibrationState = CALIBRATION_STATE_IN_PROGRESS;
        }
        return;
    }

    quint64 now = usecTimestampNow() + LOCK_DURATION;
    // these are weighted running averages
    _averageLeft = 0.9f * _averageLeft + 0.1f * positionLeft;
    _averageRight = 0.9f * _averageRight + 0.1f * positionRight;

    if (_calibrationState == CALIBRATION_STATE_IN_PROGRESS) {
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

void SixenseManager::handleAxisEvent(float stickX, float stickY, float trigger, bool left) {
}

void SixenseManager::handleButtonEvent(unsigned int buttons, bool left) {
    using namespace controller;
    if (buttons & BUTTON_0) {
        _buttonPressedMap.insert(left ? BACK : START);
    }
    if (buttons & BUTTON_1) {
        _buttonPressedMap.insert(left ? DL : X);
    }
    if (buttons & BUTTON_2) {
        _buttonPressedMap.insert(left ? DD : A);
    }
    if (buttons & BUTTON_3) {
        _buttonPressedMap.insert(left ? DR : B);
    }
    if (buttons & BUTTON_4) {
        _buttonPressedMap.insert(left ? DU : Y);
    }
    if (buttons & BUTTON_FWD) {
        _buttonPressedMap.insert(left ? LB : RB);
    }
    if (buttons & BUTTON_TRIGGER) {
        _buttonPressedMap.insert(left ? LS : RS);
    }
}

void SixenseManager::handlePoseEvent(float deltaTime, glm::vec3 position, glm::quat rotation, bool left) {
#ifdef HAVE_SIXENSE
    auto hand = left ? controller::StandardPoseChannel::LEFT_HAND : controller::StandardPoseChannel::RIGHT_HAND;

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
    auto prevPose = _poseStateMap[hand];

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
    float sign = left ? 1.0f : -1.0f;
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

    glm::vec3 velocity(0.0f);
    glm::quat angularVelocity;



    if (prevPose.isValid() && deltaTime > std::numeric_limits<float>::epsilon()) {

        velocity = (position - prevPose.getTranslation()) / deltaTime;

        auto deltaRot = rotation * glm::conjugate(prevPose.getRotation());
        auto axis = glm::axis(deltaRot);
        auto angle = glm::angle(deltaRot);
        angularVelocity = glm::angleAxis(angle / deltaTime, axis);

        // Average
        auto& samples = _collectedSamples[hand];
        samples.first.addSample(velocity);
        velocity = samples.first.average;
     
        // FIXME: // Not using quaternion average yet for angular velocity because it s probably wrong but keep the MovingAverage in place
        //samples.second.addSample(glm::vec4(angularVelocity.x, angularVelocity.y, angularVelocity.z, angularVelocity.w));
        //angularVelocity = glm::quat(samples.second.average.w, samples.second.average.x, samples.second.average.y, samples.second.average.z);
    } else if (!prevPose.isValid()) {
        _collectedSamples[hand].first.clear();
        _collectedSamples[hand].second.clear();
    }

    _poseStateMap[hand] = controller::Pose(position, rotation, velocity, angularVelocity);
#endif // HAVE_SIXENSE
}

static const auto L0 = controller::BACK;
static const auto L1 = controller::DL;
static const auto L2 = controller::DD;
static const auto L3 = controller::DR;
static const auto L4 = controller::DU;
static const auto R0 = controller::START;
static const auto R1 = controller::X;
static const auto R2 = controller::A;
static const auto R3 = controller::B;
static const auto R4 = controller::Y;

using namespace controller;

void SixenseManager::buildDeviceProxy(controller::DeviceProxy::Pointer proxy) {
    proxy->getButton = [this](const Input& input, int timestamp) -> bool { return this->getButton(input.getChannel()); };
    proxy->getAxis = [this](const Input& input, int timestamp) -> float { 
        return this->getAxis(input.getChannel()); 
    };
    proxy->getPose = [this](const Input& input, int timestamp) -> Pose { return this->getPose(input.getChannel()); };
    proxy->getAvailabeInputs = [this]() -> QVector<Input::NamedPair> {
        QVector<Input::NamedPair> availableInputs;
        availableInputs.append(Input::NamedPair(makeInput(L0), "L0"));
        availableInputs.append(Input::NamedPair(makeInput(L1), "L1"));
        availableInputs.append(Input::NamedPair(makeInput(L2), "L2"));
        availableInputs.append(Input::NamedPair(makeInput(L3), "L3"));
        availableInputs.append(Input::NamedPair(makeInput(L4), "L4"));
        availableInputs.append(Input::NamedPair(makeInput(LB), "LB"));
        availableInputs.append(Input::NamedPair(makeInput(LS), "LS"));
        availableInputs.append(Input::NamedPair(makeInput(LX), "LX"));
        availableInputs.append(Input::NamedPair(makeInput(LY), "LY"));
        availableInputs.append(Input::NamedPair(makeInput(LT), "LT"));
        availableInputs.append(Input::NamedPair(makeInput(R0), "R0"));
        availableInputs.append(Input::NamedPair(makeInput(R1), "R1"));
        availableInputs.append(Input::NamedPair(makeInput(R2), "R2"));
        availableInputs.append(Input::NamedPair(makeInput(R3), "R3"));
        availableInputs.append(Input::NamedPair(makeInput(R4), "R4"));
        availableInputs.append(Input::NamedPair(makeInput(RB), "RB"));
        availableInputs.append(Input::NamedPair(makeInput(RS), "RS"));
        availableInputs.append(Input::NamedPair(makeInput(RX), "RX"));
        availableInputs.append(Input::NamedPair(makeInput(RY), "RY"));
        availableInputs.append(Input::NamedPair(makeInput(RT), "RT"));
        availableInputs.append(Input::NamedPair(makeInput(LEFT_HAND), "LeftHand"));
        availableInputs.append(Input::NamedPair(makeInput(RIGHT_HAND), "RightHand"));
        return availableInputs;
    };
}


QString SixenseManager::getDefaultMappingConfig() {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/hydra.json";
    return MAPPING_JSON;
}

//
//void SixenseManager::assignDefaultInputMapping(UserInputMapper& mapper) {
//    const float JOYSTICK_MOVE_SPEED = 1.0f;
//    const float JOYSTICK_YAW_SPEED = 0.5f;
//    const float JOYSTICK_PITCH_SPEED = 0.25f;
//    const float BUTTON_MOVE_SPEED = 1.0f;
//    const float BOOM_SPEED = 0.1f;
//    using namespace controller;
//
//    // Left Joystick: Movement, strafing
//    mapper.addInputChannel(UserInputMapper::TRANSLATE_Z, makeInput(LY), JOYSTICK_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::TRANSLATE_X, makeInput(LX), JOYSTICK_MOVE_SPEED);
//
//    // Right Joystick: Camera orientation
//    mapper.addInputChannel(UserInputMapper::YAW, makeInput(RX), JOYSTICK_YAW_SPEED);
//    mapper.addInputChannel(UserInputMapper::PITCH, makeInput(RY), JOYSTICK_PITCH_SPEED);
//
//    // Buttons
//    mapper.addInputChannel(UserInputMapper::BOOM_IN, makeInput(L3), BOOM_SPEED);
//    mapper.addInputChannel(UserInputMapper::BOOM_OUT, makeInput(L1), BOOM_SPEED);
//
//    mapper.addInputChannel(UserInputMapper::VERTICAL_UP, makeInput(R3), BUTTON_MOVE_SPEED);
//    mapper.addInputChannel(UserInputMapper::VERTICAL_DOWN, makeInput(R1), BUTTON_MOVE_SPEED);
//
//    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(L2));
//    mapper.addInputChannel(UserInputMapper::SHIFT, makeInput(R2));
//
//    mapper.addInputChannel(UserInputMapper::ACTION1, makeInput(L4));
//    mapper.addInputChannel(UserInputMapper::ACTION2, makeInput(R4));
//
//    // FIXME
////    mapper.addInputChannel(UserInputMapper::LEFT_HAND, makeInput(LEFT_HAND));
////    mapper.addInputChannel(UserInputMapper::RIGHT_HAND, makeInput(RIGHT_HAND));
//
//    mapper.addInputChannel(UserInputMapper::LEFT_HAND_CLICK, makeInput(LT));
//    mapper.addInputChannel(UserInputMapper::RIGHT_HAND_CLICK, makeInput(RT));
//
//    // TODO find a mechanism to allow users to navigate the context menu via
//    mapper.addInputChannel(UserInputMapper::CONTEXT_MENU, makeInput(L0));
//    mapper.addInputChannel(UserInputMapper::TOGGLE_MUTE, makeInput(R0));
//
//}

// virtual
void SixenseManager::saveSettings() const {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.setVec3Value(QString("avatarPosition"), _avatarPosition);
        settings.setQuatValue(QString("avatarRotation"), _avatarRotation);
        settings.setValue(QString("reachLength"), QVariant(_reachLength));
    }
    settings.endGroup();
}

void SixenseManager::loadSettings() {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.getVec3ValueIfValid(QString("avatarPosition"), _avatarPosition);
        settings.getQuatValueIfValid(QString("avatarRotation"), _avatarRotation);
        settings.getFloatValueIfValid(QString("reachLength"), _reachLength);
    }
    settings.endGroup();
}
