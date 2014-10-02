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

#include <PerfStat.h>

#include "Application.h"
#include "SixenseManager.h"
#include "devices/OculusManager.h"
#include "UserActivityLogger.h"

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
        _lastMovement = 0;
        _amountMoved = glm::vec3(0.0f);
        _lowVelocityFilter = false;
        
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
            QString frameworkSixenseLibrary = QCoreApplication::applicationDirPath() + "../Frameworks/"
                + SIXENSE_LIBRARY_NAME;
        
            _sixenseLibrary = new QLibrary(frameworkSixenseLibrary);
#endif
        }
        
        if (_sixenseLibrary->load()){
            qDebug() << "Loaded sixense library for hydra support -" << _sixenseLibrary->fileName();
        } else {
            qDebug() << "Sixense library at" << _sixenseLibrary->fileName() << "failed to load."
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
    if (_isInitialized && _isEnabled) {
        // if the controllers haven't been moved in a while, disable
        const unsigned int MOVEMENT_DISABLE_SECONDS = 3;
        if (usecTimestampNow() - _lastMovement > (MOVEMENT_DISABLE_SECONDS * USECS_PER_SECOND)) {
            Hand* hand = Application::getInstance()->getAvatar()->getHand();
            for (std::vector<PalmData>::iterator it = hand->getPalms().begin(); it != hand->getPalms().end(); it++) {
                it->setActive(false);
            }
            _lastMovement = usecTimestampNow();
        }
        
#ifdef __APPLE__
        SixenseBaseFunction sixenseGetNumActiveControllers =
        (SixenseBaseFunction) _sixenseLibrary->resolve("sixenseGetNumActiveControllers");
#endif
        
        if (sixenseGetNumActiveControllers() == 0) {
            _hydrasConnected = false;
            return;
        }
        
        PerformanceTimer perfTimer("sixense");
        if (!_hydrasConnected) {
            _hydrasConnected = true;
            UserActivityLogger::getInstance().connectedDevice("spatial_controller", "hydra");
        }
        MyAvatar* avatar = Application::getInstance()->getAvatar();
        Hand* hand = avatar->getHand();
        
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
            
            //  Set palm position and normal based on Hydra position/orientation
            
            // Either find a palm matching the sixense controller, or make a new one
            PalmData* palm;
            bool foundHand = false;
            for (size_t j = 0; j < hand->getNumPalms(); j++) {
                if (hand->getPalms()[j].getSixenseID() == data->controller_index) {
                    palm = &(hand->getPalms()[j]);
                    foundHand = true;
                }
            }
            if (!foundHand) {
                PalmData newPalm(hand);
                hand->getPalms().push_back(newPalm);
                palm = &(hand->getPalms()[hand->getNumPalms() - 1]);
                palm->setSixenseID(data->controller_index);
                qDebug("Found new Sixense controller, ID %i", data->controller_index);
            }
            
            palm->setActive(true);
            
            //  Read controller buttons and joystick into the hand
            palm->setControllerButtons(data->buttons);
            palm->setTrigger(data->trigger);
            palm->setJoystick(data->joystick_x, data->joystick_y);
            
            
            // Emulate the mouse so we can use scripts
            if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseMouseInput)) {
                emulateMouse(palm, numActiveControllers - 1);
            }
            
            // NOTE: Sixense API returns pos data in millimeters but we IMMEDIATELY convert to meters.
            glm::vec3 position(data->pos[0], data->pos[1], data->pos[2]);
            position *= METERS_PER_MILLIMETER;
            
            // Transform the measured position into body frame.
            glm::vec3 neck = _neckBase;
            // Zeroing y component of the "neck" effectively raises the measured position a little bit.
            neck.y = 0.f;
            position = _orbRotation * (position - neck);
            
            //  Rotation of Palm
            glm::quat rotation(data->rot_quat[3], -data->rot_quat[0], data->rot_quat[1], -data->rot_quat[2]);
            rotation = glm::angleAxis(PI, glm::vec3(0.f, 1.f, 0.f)) * _orbRotation * rotation;
            
            //  Compute current velocity from position change
            glm::vec3 rawVelocity;
            if (deltaTime > 0.f) {
                rawVelocity = (position - palm->getRawPosition()) / deltaTime;
            } else {
                rawVelocity = glm::vec3(0.0f);
            }
            palm->setRawVelocity(rawVelocity);   //  meters/sec
            
            // adjustment for hydra controllers fit into hands
            float sign = (i == 0) ? -1.0f : 1.0f;
            rotation *= glm::angleAxis(sign * PI/4.0f, glm::vec3(0.0f, 0.0f, 1.0f));
            
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
            
            // use the velocity to determine whether there's any movement (if the hand isn't new)
            const float MOVEMENT_DISTANCE_THRESHOLD = 0.003f;
            _amountMoved += rawVelocity * deltaTime;
            if (glm::length(_amountMoved) > MOVEMENT_DISTANCE_THRESHOLD && foundHand) {
                _lastMovement = usecTimestampNow();
                _amountMoved = glm::vec3(0.0f);
            }
            
            // Store the one fingertip in the palm structure so we can track velocity
            const float FINGER_LENGTH = 0.3f;   //  meters
            const glm::vec3 FINGER_VECTOR(0.0f, 0.0f, FINGER_LENGTH);
            const glm::vec3 newTipPosition = position + rotation * FINGER_VECTOR;
            glm::vec3 oldTipPosition = palm->getTipRawPosition();
            if (deltaTime > 0.f) {
                palm->setTipVelocity((newTipPosition - oldTipPosition) / deltaTime);
            } else {
                palm->setTipVelocity(glm::vec3(0.f));
            }
            palm->setTipPosition(newTipPosition);
        }
        
        if (numActiveControllers == 2) {
            updateCalibration(controllers);
        }

    }
#endif  // HAVE_SIXENSE
}

//Constants for getCursorPixelRangeMultiplier()
const float MIN_PIXEL_RANGE_MULT = 0.4f;
const float MAX_PIXEL_RANGE_MULT = 2.0f;
const float RANGE_MULT = (MAX_PIXEL_RANGE_MULT - MIN_PIXEL_RANGE_MULT) * 0.01;

//Returns a multiplier to be applied to the cursor range for the controllers
float SixenseManager::getCursorPixelRangeMult() const {
    //scales (0,100) to (MINIMUM_PIXEL_RANGE_MULT, MAXIMUM_PIXEL_RANGE_MULT)
    return Menu::getInstance()->getSixenseReticleMoveSpeed() * RANGE_MULT + MIN_PIXEL_RANGE_MULT;
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
                glm::vec3 yAxis(0.f, 1.f, 0.f);
                glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, yAxis));
                xAxis = glm::normalize(glm::cross(yAxis, zAxis));
                _orbRotation = glm::inverse(glm::quat_cast(glm::mat3(xAxis, yAxis, zAxis)));
                qDebug("succeess: sixense calibration");
            }
            break;
            default:
                qDebug("failed: sixense calibration");
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
            qDebug("started: sixense calibration");
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
        if (fabs(distance - _lastDistance) > MAXIMUM_NOISE_LEVEL) {
            // distance is increasing so acquire the data and push the expiry out
            _reachLeft = _averageLeft;
            _reachRight = _averageRight;
            _lastDistance = distance;
            _lockExpiry = now + LOCK_DURATION;
        } else if (now > _lockExpiry) {
            // lock has expired so clamp the data and move on
            _lockExpiry = now + LOCK_DURATION;
            _lastDistance = 0.f;
            _reachUp = 0.5f * (_reachLeft + _reachRight);
            _calibrationState = CALIBRATION_STATE_Y;
            qDebug("success: sixense calibration: left");
        }
    }
    else if (_calibrationState == CALIBRATION_STATE_Y) {
        glm::vec3 torso = 0.5f * (_reachLeft + _reachRight);
        glm::vec3 averagePosition = 0.5f * (_averageLeft + _averageRight);
        float distance = (averagePosition - torso).y;
        if (fabs(distance) > fabs(_lastDistance) + MAXIMUM_NOISE_LEVEL) {
            // distance is increasing so acquire the data and push the expiry out
            _reachUp = averagePosition;
            _lastDistance = distance;
            _lockExpiry = now + LOCK_DURATION;
        } else if (now > _lockExpiry) {
            if (_lastDistance > MINIMUM_ARM_REACH) {
                // lock has expired so clamp the data and move on
                _reachForward = _reachUp;
                _lastDistance = 0.f;
                _lockExpiry = now + LOCK_DURATION;
                _calibrationState = CALIBRATION_STATE_Z;
                qDebug("success: sixense calibration: up");
            }
        }
    }
    else if (_calibrationState == CALIBRATION_STATE_Z) {
        glm::vec3 xAxis = glm::normalize(_reachRight - _reachLeft);
        glm::vec3 torso = 0.5f * (_reachLeft + _reachRight);
        //glm::vec3 yAxis = glm::normalize(_reachUp - torso);
        glm::vec3 yAxis(0.f, 1.f, 0.f);
        glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, yAxis));

        glm::vec3 averagePosition = 0.5f * (_averageLeft + _averageRight);
        float distance = glm::dot((averagePosition - torso), zAxis);
        if (fabs(distance) > fabs(_lastDistance)) {
            // distance is increasing so acquire the data and push the expiry out
            _reachForward = averagePosition;
            _lastDistance = distance;
            _lockExpiry = now + LOCK_DURATION;
        } else if (now > _lockExpiry) {
            if (fabs(_lastDistance) > 0.05f * MINIMUM_ARM_REACH) {
                // lock has expired so clamp the data and move on
                _calibrationState = CALIBRATION_STATE_COMPLETE;
                qDebug("success: sixense calibration: forward");
                // TODO: it is theoretically possible to detect that the controllers have been 
                // accidentally switched (left hand is holding right controller) and to swap the order.
            }
        }
    }
}

//Injecting mouse movements and clicks
void SixenseManager::emulateMouse(PalmData* palm, int index) {
    Application* application = Application::getInstance();
    MyAvatar* avatar = application->getAvatar();
    GLCanvas* widget = application->getGLWidget();
    QPoint pos;
    
    Qt::MouseButton bumperButton;
    Qt::MouseButton triggerButton;

    unsigned int deviceID = index == 0 ? CONTROLLER_0_EVENT : CONTROLLER_1_EVENT;

    if (Menu::getInstance()->getInvertSixenseButtons()) {
        bumperButton = Qt::LeftButton;
        triggerButton = Qt::RightButton;
    } else {
        bumperButton = Qt::RightButton;
        triggerButton = Qt::LeftButton;
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseLasers)) {
        pos = application->getApplicationOverlay().getPalmClickLocation(palm);
    } else {
        // Get directon relative to avatar orientation
        glm::vec3 direction = glm::inverse(avatar->getOrientation()) * palm->getFingerDirection();

        // Get the angles, scaled between (-0.5,0.5)
        float xAngle = (atan2(direction.z, direction.x) + M_PI_2);
        float yAngle = 0.5f - ((atan2(direction.z, direction.y) + M_PI_2));

        // Get the pixel range over which the xAngle and yAngle are scaled
        float cursorRange = widget->width() * getCursorPixelRangeMult();

        pos.setX(widget->width() / 2.0f + cursorRange * xAngle);
        pos.setY(widget->height() / 2.0f + cursorRange * yAngle);

    }

    //If we are off screen then we should stop processing, and if a trigger or bumper is pressed,
    //we should unpress them.
    if (pos.x() == INT_MAX) {
        if (_bumperPressed[index]) {
            QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, bumperButton, bumperButton, 0);

            application->mouseReleaseEvent(&mouseEvent, deviceID);

            _bumperPressed[index] = false;
        }
        if (_triggerPressed[index]) {
            QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, triggerButton, triggerButton, 0);

            application->mouseReleaseEvent(&mouseEvent, deviceID);

            _triggerPressed[index] = false;
        }
        return;
    }

    //If position has changed, emit a mouse move to the application
    if (pos.x() != _oldX[index] || pos.y() != _oldY[index]) {
        QMouseEvent mouseEvent(QEvent::MouseMove, pos, Qt::NoButton, Qt::NoButton, 0);

        //Only send the mouse event if the opposite left button isnt held down.
        //This is specifically for edit voxels
        if (triggerButton == Qt::LeftButton) {
            if (!_triggerPressed[(int)(!index)]) {
                application->mouseMoveEvent(&mouseEvent, deviceID);
            }
        } else {
            if (!_bumperPressed[(int)(!index)]) {
                application->mouseMoveEvent(&mouseEvent, deviceID);
            }
        } 
    }
    _oldX[index] = pos.x();
    _oldY[index] = pos.y();
    

    //We need separate coordinates for clicks, since we need to check if
    //a magnification window was clicked on
    int clickX = pos.x();
    int clickY = pos.y();
    //Checks for magnification window click
    application->getApplicationOverlay().getClickLocation(clickX, clickY);
    //Set pos to the new click location, which may be the same if no magnification window is open
    pos.setX(clickX);
    pos.setY(clickY);

    //Check for bumper press ( Right Click )
    if (palm->getControllerButtons() & BUTTON_FWD) {
        if (!_bumperPressed[index]) {
            _bumperPressed[index] = true;
        
            QMouseEvent mouseEvent(QEvent::MouseButtonPress, pos, bumperButton, bumperButton, 0);

            application->mousePressEvent(&mouseEvent, deviceID);
        }
    } else if (_bumperPressed[index]) {
        QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, bumperButton, bumperButton, 0);

        application->mouseReleaseEvent(&mouseEvent, deviceID);

        _bumperPressed[index] = false;
    }

    //Check for trigger press ( Left Click )
    if (palm->getTrigger() == 1.0f) {
        if (!_triggerPressed[index]) {
            _triggerPressed[index] = true;

            QMouseEvent mouseEvent(QEvent::MouseButtonPress, pos, triggerButton, triggerButton, 0);

            application->mousePressEvent(&mouseEvent, deviceID);
        }
    } else if (_triggerPressed[index]) {
        QMouseEvent mouseEvent(QEvent::MouseButtonRelease, pos, triggerButton, triggerButton, 0);

        application->mouseReleaseEvent(&mouseEvent, deviceID);

        _triggerPressed[index] = false;
    }
}

#endif  // HAVE_SIXENSE

