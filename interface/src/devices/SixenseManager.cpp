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

#include "Application.h"
#include "SixenseManager.h"

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
#endif

SixenseManager::SixenseManager() {
#ifdef HAVE_SIXENSE
    _lastMovement = 0;
    _amountMoved = glm::vec3(0.0f);

    _calibrationState = CALIBRATION_STATE_IDLE;
    // By default we assume the _neckBase (in orb frame) is as high above the orb 
    // as the "torso" is below it.
    _neckBase = glm::vec3(NECK_X, -NECK_Y, NECK_Z);

    sixenseInit();
#endif
}

SixenseManager::~SixenseManager() {
#ifdef HAVE_SIXENSE
    sixenseExit();
#endif
}

void SixenseManager::setFilter(bool filter) {
#ifdef HAVE_SIXENSE
    if (filter) {
        qDebug("Sixense Filter ON");
        sixenseSetFilterEnabled(1);
    } else {
        qDebug("Sixense Filter OFF");
        sixenseSetFilterEnabled(0);
    }
#endif
}

void SixenseManager::update(float deltaTime) {
#ifdef HAVE_SIXENSE
    if (sixenseGetNumActiveControllers() == 0) {
        return;
    }
    MyAvatar* avatar = Application::getInstance()->getAvatar();
    Hand* hand = avatar->getHand();
    
    int maxControllers = sixenseGetMaxControllers();

    // we only support two controllers
    sixenseControllerData controllers[2];

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
    
        //  Use a velocity sensitive filter to damp small motions and preserve large ones with
        //  no latency.
        float velocityFilter = glm::clamp(1.0f - glm::length(rawVelocity), 0.0f, 1.0f);
        palm->setRawPosition(palm->getRawPosition() * velocityFilter + position * (1.0f - velocityFilter));
        palm->setRawRotation(safeMix(palm->getRawRotation(), rotation, 1.0f - velocityFilter));
        
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

    // if the controllers haven't been moved in a while, disable
    const unsigned int MOVEMENT_DISABLE_SECONDS = 3;
    if (usecTimestampNow() - _lastMovement > (MOVEMENT_DISABLE_SECONDS * USECS_PER_SECOND)) {
        for (std::vector<PalmData>::iterator it = hand->getPalms().begin(); it != hand->getPalms().end(); it++) {
            it->setActive(false);
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
#endif  // HAVE_SIXENSE

