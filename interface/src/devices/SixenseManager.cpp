//
//  SixenseManager.cpp
//  interface/src/devices
//
//  Created by Andrzej Kapolka on 11/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <vector>

#include <avatar/AvatarManager.h>
#include <PerfStat.h>
#include <NumericalConstants.h>

#include "Application.h"
#include "SixenseManager.h"
#include "UserActivityLogger.h"
#include "InterfaceLogging.h"

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

#ifdef __APPLE__
typedef int (*SixenseBaseFunction)();
typedef int (*SixenseTakeIntFunction)(int);
typedef int (*SixenseTakeIntAndSixenseControllerData)(int, sixenseControllerData*);
#endif

#endif

SixenseManager& SixenseManager::getInstance() {
    static SixenseManager sharedInstance;
    return sharedInstance;
}

SixenseManager::SixenseManager() :
#if defined(HAVE_SIXENSE) && defined(__APPLE__)
    _sixenseLibrary(NULL),
#endif
    _isInitialized(false),
    _isEnabled(true),
    _hydrasConnected(false)
{
    _triggerPressed[0] = false;
    _bumperPressed[0] = false;
    _oldX[0] = -1;
    _oldY[0] = -1;
    _triggerPressed[1] = false;
    _bumperPressed[1] = false;
    _oldX[1] = -1;
    _oldY[1] = -1;
    _prevPalms[0] = nullptr;
    _prevPalms[1] = nullptr;
}

SixenseManager::~SixenseManager() {
#ifdef HAVE_SIXENSE_

    if (_isInitialized) {
#ifdef __APPLE__
        SixenseBaseFunction sixenseExit = (SixenseBaseFunction) _sixenseLibrary->resolve("sixenseExit");
#endif

        sixenseExit();
    }

#ifdef __APPLE__
    delete _sixenseLibrary;
#endif

#endif
}

void SixenseManager::initialize() {
#ifdef HAVE_SIXENSE

    if (!_isInitialized) {
        _lowVelocityFilter = false;
        _controllersAtBase = true;
        _calibrationState = CALIBRATION_STATE_IDLE;
        // By default we assume the _neckBase (in orb frame) is as high above the orb
        // as the "torso" is below it.
        _neckBase = glm::vec3(NECK_X, -NECK_Y, NECK_Z);

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
            qCDebug(interfaceapp) << "Loaded sixense library for hydra support -" << _sixenseLibrary->fileName();
        } else {
            qCDebug(interfaceapp) << "Sixense library at" << _sixenseLibrary->fileName() << "failed to load."
                << "Continuing without hydra support.";
            return;
        }

        SixenseBaseFunction sixenseInit = (SixenseBaseFunction) _sixenseLibrary->resolve("sixenseInit");
#endif
        sixenseInit();

        _isInitialized = true;
    }

#endif
}

void SixenseManager::setFilter(bool filter) {
#ifdef HAVE_SIXENSE

    if (_isInitialized) {
#ifdef __APPLE__
        SixenseTakeIntFunction sixenseSetFilterEnabled = (SixenseTakeIntFunction) _sixenseLibrary->resolve("sixenseSetFilterEnabled");
#endif

        if (filter) {
            sixenseSetFilterEnabled(1);
        } else {
            sixenseSetFilterEnabled(0);
        }
    }

#endif
}

void SixenseManager::update(float deltaTime) {
#ifdef HAVE_SIXENSE
    Hand* hand = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHand();
    if (_isInitialized && _isEnabled) {
        _buttonPressedMap.clear();
#ifdef __APPLE__
        SixenseBaseFunction sixenseGetNumActiveControllers =
        (SixenseBaseFunction) _sixenseLibrary->resolve("sixenseGetNumActiveControllers");
#endif

        if (sixenseGetNumActiveControllers() == 0) {
            _hydrasConnected = false;
            if (_deviceID != 0) {
                Application::getUserInputMapper()->removeDevice(_deviceID);
                _deviceID = 0;
                if (_prevPalms[0]) {
                    _prevPalms[0]->setActive(false);
                }
                if (_prevPalms[1]) {
                    _prevPalms[1]->setActive(false);
                }
            }
            return;
        }

        PerformanceTimer perfTimer("sixense");
        if (!_hydrasConnected) {
            _hydrasConnected = true;
            registerToUserInputMapper(*Application::getUserInputMapper());
            getInstance().assignDefaultInputMapping(*Application::getUserInputMapper());
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
        int numControllersAtBase = 0;
        int numActiveControllers = 0;
        for (int i = 0; i < maxControllers && numActiveControllers < 2; i++) {
            if (!sixenseIsControllerEnabled(i)) {
                continue;
            }
            sixenseControllerData* data = controllers + numActiveControllers;
            ++numActiveControllers;
            sixenseGetNewestData(i, data);

            //  Set palm position and normal based on Hydra position/orientation

            // Either find a palm matching the sixense controller, or make a new one
            PalmData* palm;
            bool foundHand = false;
            for (size_t j = 0; j < hand->getNumPalms(); j++) {
                if (hand->getPalms()[j].getSixenseID() == data->controller_index) {
                    palm = &(hand->getPalms()[j]);
                    _prevPalms[numActiveControllers - 1] = palm;
                    foundHand = true;
                }
            }
            if (!foundHand) {
                PalmData newPalm(hand);
                hand->getPalms().push_back(newPalm);
                palm = &(hand->getPalms()[hand->getNumPalms() - 1]);
                palm->setSixenseID(data->controller_index);
                _prevPalms[numActiveControllers - 1] = palm;
                qCDebug(interfaceapp, "Found new Sixense controller, ID %i", data->controller_index);
            }

            // Disable the hands (and return to default pose) if both controllers are at base station
            if (foundHand) {
                palm->setActive(!_controllersAtBase);
            } else {
                palm->setActive(false); // if this isn't a Sixsense ID palm, always make it inactive
            }

            //  Read controller buttons and joystick into the hand
            palm->setControllerButtons(data->buttons);
            palm->setTrigger(data->trigger);
            palm->setJoystick(data->joystick_x, data->joystick_y);

            // NOTE: Sixense API returns pos data in millimeters but we IMMEDIATELY convert to meters.
            glm::vec3 position(data->pos[0], data->pos[1], data->pos[2]);
            position *= METERS_PER_MILLIMETER;

            // Check to see if this hand/controller is on the base
            const float CONTROLLER_AT_BASE_DISTANCE = 0.075f;
            if (glm::length(position) < CONTROLLER_AT_BASE_DISTANCE) {
                numControllersAtBase++;
                palm->setActive(false);
            } else {
                handleButtonEvent(data->buttons, numActiveControllers - 1);
                handleAxisEvent(data->joystick_x, data->joystick_y, data->trigger, numActiveControllers - 1);
                
                // Emulate the mouse so we can use scripts
                if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseMouseInput) && !_controllersAtBase) {
                    emulateMouse(palm, numActiveControllers - 1);
                }
            }

            // Transform the measured position into body frame.
            glm::vec3 neck = _neckBase;
            // Zeroing y component of the "neck" effectively raises the measured position a little bit.
            neck.y = 0.0f;
            position = _orbRotation * (position - neck);

            //  Rotation of Palm
            glm::quat rotation(data->rot_quat[3], -data->rot_quat[0], data->rot_quat[1], -data->rot_quat[2]);
            rotation = glm::angleAxis(PI, glm::vec3(0.0f, 1.0f, 0.0f)) * _orbRotation * rotation;

            //  Compute current velocity from position change
            glm::vec3 rawVelocity;
            if (deltaTime > 0.0f) {
                rawVelocity = (position - palm->getRawPosition()) / deltaTime;
            } else {
                rawVelocity = glm::vec3(0.0f);
            }
            palm->setRawVelocity(rawVelocity);   //  meters/sec

            // adjustment for hydra controllers fit into hands
            float sign = (i == 0) ? -1.0f : 1.0f;
            rotation *= glm::angleAxis(sign * PI/4.0f, glm::vec3(0.0f, 0.0f, 1.0f));

            //  Angular Velocity of Palm
            glm::quat deltaRotation = rotation * glm::inverse(palm->getRawRotation());
            glm::vec3 angularVelocity(0.0f);
            float rotationAngle = glm::angle(deltaRotation);
            if ((rotationAngle > EPSILON) && (deltaTime > 0.0f)) {
                angularVelocity = glm::normalize(glm::axis(deltaRotation));
                angularVelocity *= (rotationAngle / deltaTime);
                palm->setRawAngularVelocity(angularVelocity);
            } else {
                palm->setRawAngularVelocity(glm::vec3(0.0f));
            }

            if (_lowVelocityFilter) {
                //  Use a velocity sensitive filter to damp small motions and preserve large ones with
                //  no latency.
                float velocityFilter = glm::clamp(1.0f - glm::length(rawVelocity), 0.0f, 1.0f);
                position = palm->getRawPosition() * velocityFilter + position * (1.0f - velocityFilter);
                rotation = safeMix(palm->getRawRotation(), rotation, 1.0f - velocityFilter);
                palm->setRawPosition(position);
                palm->setRawRotation(rotation);
            } else {
                palm->setRawPosition(position);
                palm->setRawRotation(rotation);
            }

            // Store the one fingertip in the palm structure so we can track velocity
            const float FINGER_LENGTH = 0.3f;   //  meters
            const glm::vec3 FINGER_VECTOR(0.0f, 0.0f, FINGER_LENGTH);
            const glm::vec3 newTipPosition = position + rotation * FINGER_VECTOR;
            glm::vec3 oldTipPosition = palm->getTipRawPosition();
            if (deltaTime > 0.0f) {
                palm->setTipVelocity((newTipPosition - oldTipPosition) / deltaTime);
            } else {
                palm->setTipVelocity(glm::vec3(0.0f));
            }
            palm->setTipPosition(newTipPosition);
        }

        if (numActiveControllers == 2) {
            updateCalibration(controllers);
        }
        _controllersAtBase = (numControllersAtBase == 2);
    }
    
    for (auto axisState : _axisStateMap) {
        if (fabsf(axisState.second) < CONTROLLER_THRESHOLD) {
            _axisStateMap[axisState.first] = 0.0f;
        }
    }
#endif  // HAVE_SIXENSE
}

//Constants for getCursorPixelRangeMultiplier()
const float MIN_PIXEL_RANGE_MULT = 0.4f;
const float MAX_PIXEL_RANGE_MULT = 2.0f;
const float RANGE_MULT = (MAX_PIXEL_RANGE_MULT - MIN_PIXEL_RANGE_MULT) * 0.01f;

//Returns a multiplier to be applied to the cursor range for the controllers
float SixenseManager::getCursorPixelRangeMult() const {
    //scales (0,100) to (MINIMUM_PIXEL_RANGE_MULT, MAXIMUM_PIXEL_RANGE_MULT)
    return _reticleMoveSpeed * RANGE_MULT + MIN_PIXEL_RANGE_MULT;
}

void SixenseManager::toggleSixense(bool shouldEnable) {
    if (shouldEnable && !isInitialized()) {
        initialize();
        setFilter(Menu::getInstance()->isOptionChecked(MenuOption::FilterSixense));
        setLowVelocityFilter(Menu::getInstance()->isOptionChecked(MenuOption::LowVelocityFilter));
    }
    setIsEnabled(shouldEnable);
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

void SixenseManager::updateCalibration(const sixenseControllerData* controllers) {
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
                qCDebug(interfaceapp, "succeess: sixense calibration");
            }
            break;
            default:
                qCDebug(interfaceapp, "failed: sixense calibration");
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
            qCDebug(interfaceapp, "started: sixense calibration");
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
            qCDebug(interfaceapp, "success: sixense calibration: left");
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
                qCDebug(interfaceapp, "success: sixense calibration: up");
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
                qCDebug(interfaceapp, "success: sixense calibration: forward");
                // TODO: it is theoretically possible to detect that the controllers have been
                // accidentally switched (left hand is holding right controller) and to swap the order.
            }
        }
    }
}

//Injecting mouse movements and clicks
void SixenseManager::emulateMouse(PalmData* palm, int index) {
    MyAvatar* avatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
    QPoint pos;

    Qt::MouseButton bumperButton;
    Qt::MouseButton triggerButton;

    unsigned int deviceID = index == 0 ? CONTROLLER_0_EVENT : CONTROLLER_1_EVENT;

    if (_invertButtons) {
        bumperButton = Qt::LeftButton;
        triggerButton = Qt::RightButton;
    } else {
        bumperButton = Qt::RightButton;
        triggerButton = Qt::LeftButton;
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseLasers)
        || Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode)) {
        pos = qApp->getApplicationCompositor().getPalmClickLocation(palm);
    } else {
        // Get directon relative to avatar orientation
        glm::vec3 direction = glm::inverse(avatar->getOrientation()) * palm->getFingerDirection();

        // Get the angles, scaled between (-0.5,0.5)
        float xAngle = (atan2(direction.z, direction.x) + PI_OVER_TWO);
        float yAngle = 0.5f - ((atan2f(direction.z, direction.y) + (float)PI_OVER_TWO));
        auto canvasSize = qApp->getCanvasSize();
        // Get the pixel range over which the xAngle and yAngle are scaled
        float cursorRange = canvasSize.x * getCursorPixelRangeMult();

        pos.setX(canvasSize.x / 2.0f + cursorRange * xAngle);
        pos.setY(canvasSize.y / 2.0f + cursorRange * yAngle);

    }

    //If we are off screen then we should stop processing, and if a trigger or bumper is pressed,
    //we should unpress them.
    if (pos.x() == INT_MAX) {
        if (_bumperPressed[index]) {
            QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, bumperButton, bumperButton, 0);

            qApp->mouseReleaseEvent(&mouseEvent, deviceID);

            _bumperPressed[index] = false;
        }
        if (_triggerPressed[index]) {
            QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, triggerButton, triggerButton, 0);

            qApp->mouseReleaseEvent(&mouseEvent, deviceID);

            _triggerPressed[index] = false;
        }
        return;
    }

    //If position has changed, emit a mouse move to the application
    if (pos.x() != _oldX[index] || pos.y() != _oldY[index]) {
        QMouseEvent mouseEvent(QEvent::MouseMove, pos, Qt::NoButton, Qt::NoButton, 0);

        //Only send the mouse event if the opposite left button isnt held down.
        if (triggerButton == Qt::LeftButton) {
            if (!_triggerPressed[(int)(!index)]) {
                qApp->mouseMoveEvent(&mouseEvent, deviceID);
            }
        } else {
            if (!_bumperPressed[(int)(!index)]) {
                qApp->mouseMoveEvent(&mouseEvent, deviceID);
            }
        }
    }
    _oldX[index] = pos.x();
    _oldY[index] = pos.y();


    //We need separate coordinates for clicks, since we need to check if
    //a magnification window was clicked on
    int clickX = pos.x();
    int clickY = pos.y();
    //Set pos to the new click location, which may be the same if no magnification window is open
    pos.setX(clickX);
    pos.setY(clickY);

    //Check for bumper press ( Right Click )
    if (palm->getControllerButtons() & BUTTON_FWD) {
        if (!_bumperPressed[index]) {
            _bumperPressed[index] = true;

            QMouseEvent mouseEvent(QEvent::MouseButtonPress, pos, bumperButton, bumperButton, 0);

            qApp->mousePressEvent(&mouseEvent, deviceID);
        }
    } else if (_bumperPressed[index]) {
        QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, bumperButton, bumperButton, 0);

        qApp->mouseReleaseEvent(&mouseEvent, deviceID);

        _bumperPressed[index] = false;
    }

    //Check for trigger press ( Left Click )
    if (palm->getTrigger() == 1.0f) {
        if (!_triggerPressed[index]) {
            _triggerPressed[index] = true;

            QMouseEvent mouseEvent(QEvent::MouseButtonPress, pos, triggerButton, triggerButton, 0);

            qApp->mousePressEvent(&mouseEvent, deviceID);
        }
    } else if (_triggerPressed[index]) {
        QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, triggerButton, triggerButton, 0);

        qApp->mouseReleaseEvent(&mouseEvent, deviceID);

        _triggerPressed[index] = false;
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

void SixenseManager::registerToUserInputMapper(UserInputMapper& mapper) {
    // Grab the current free device ID
    _deviceID = mapper.getFreeDeviceID();
    
    auto proxy = std::make_shared<UserInputMapper::DeviceProxy>("Hydra");
    proxy->getButton = [this] (const UserInputMapper::Input& input, int timestamp) -> bool { return this->getButton(input.getChannel()); };
    proxy->getAxis = [this] (const UserInputMapper::Input& input, int timestamp) -> float { return this->getAxis(input.getChannel()); };
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

}

float SixenseManager::getButton(int channel) const {
    if (!_buttonPressedMap.empty()) {
        if (_buttonPressedMap.find(channel) != _buttonPressedMap.end()) {
            return 1.0f;
        } else {
            return 0.0f;
        }
    }
    return 0.0f;
}

float SixenseManager::getAxis(int channel) const {
    auto axis = _axisStateMap.find(channel);
    if (axis != _axisStateMap.end()) {
        return (*axis).second;
    } else {
        return 0.0f;
    }
}

UserInputMapper::Input SixenseManager::makeInput(unsigned int button, int index) {
    return UserInputMapper::Input(_deviceID, button | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::BUTTON);
}

UserInputMapper::Input SixenseManager::makeInput(SixenseManager::JoystickAxisChannel axis, int index) {
    return UserInputMapper::Input(_deviceID, axis | (index == 0 ? LEFT_MASK : RIGHT_MASK), UserInputMapper::ChannelType::AXIS);
}
