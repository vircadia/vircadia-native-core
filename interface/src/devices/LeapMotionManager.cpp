//
//  LeapMotionManager.cpp
//  interface/src/devices
//
//  Created by Sam Cake on 6/2/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTimer>
#include <QtDebug>

#include <FBXReader.h>

#include "Application.h"
#include "LeapMotionManager.h"
#include "ui/TextRenderer.h"


#ifdef HAVE_LEAPMOTION


LeapMotionManager::SampleListener::SampleListener() : ::Leap::Listener()
{
//    std::cout << __FUNCTION__ << std::endl;
}

LeapMotionManager::SampleListener::~SampleListener()
{
//    std::cout << __FUNCTION__ << std::endl;
}

void LeapMotionManager::SampleListener::onConnect(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void LeapMotionManager::SampleListener::onDisconnect(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void LeapMotionManager::SampleListener::onExit(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void LeapMotionManager::SampleListener::onFocusGained(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void LeapMotionManager::SampleListener::onFocusLost(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void LeapMotionManager::SampleListener::onFrame(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void LeapMotionManager::SampleListener::onInit(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void LeapMotionManager::SampleListener::onServiceConnect(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void LeapMotionManager::SampleListener::onServiceDisconnect(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}

/*const unsigned int SERIAL_LIST[] = { 0x00000001, 0x00000000, 0x00000008, 0x00000009, 0x0000000A,
    0x0000000C, 0x0000000D, 0x0000000E, 0x00000004, 0x00000005, 0x00000010, 0x00000011 };
const unsigned char AXIS_LIST[] = { 9, 43, 37, 37, 37, 13, 13, 13, 52, 52, 28, 28 };
const int LIST_LENGTH = sizeof(SERIAL_LIST) / sizeof(SERIAL_LIST[0]);

const char* JOINT_NAMES[] = { "Neck", "Spine", "LeftArm", "LeftForeArm", "LeftHand", "RightArm",
    "RightForeArm", "RightHand", "LeftUpLeg", "LeftLeg", "RightUpLeg", "RightLeg" };  

static int indexOfHumanIKJoint(const char* jointName) {
    for (int i = 0;; i++) {
        QByteArray humanIKJoint = HUMANIK_JOINTS[i];
        if (humanIKJoint.isEmpty()) {
            return -1;
        }
        if (humanIKJoint == jointName) {
            return i;
        }
    }
}

static void setPalm(float deltaTime, int index) {
    MyAvatar* avatar = Application::getInstance()->getAvatar();
    Hand* hand = avatar->getHand();
    PalmData* palm;
    bool foundHand = false;
    for (size_t j = 0; j < hand->getNumPalms(); j++) {
        if (hand->getPalms()[j].getSixenseID() == index) {
            palm = &(hand->getPalms()[j]);
            foundHand = true;
        }
    }
    if (!foundHand) {
        PalmData newPalm(hand);
        hand->getPalms().push_back(newPalm);
        palm = &(hand->getPalms()[hand->getNumPalms() - 1]);
        palm->setSixenseID(index);
    }
    
    palm->setActive(true);
    
    // Read controller buttons and joystick into the hand
    if (!Application::getInstance()->getJoystickManager()->getJoystickStates().isEmpty()) {
        const JoystickState& state = Application::getInstance()->getJoystickManager()->getJoystickStates().at(0);
        if (state.axes.size() >= 4 && state.buttons.size() >= 4) {
            if (index == LEFT_HAND_INDEX) {
                palm->setControllerButtons(state.buttons.at(1) ? BUTTON_FWD : 0);
                palm->setTrigger(state.buttons.at(0) ? 1.0f : 0.0f);
                palm->setJoystick(state.axes.at(0), -state.axes.at(1));
                
            } else {
                palm->setControllerButtons(state.buttons.at(3) ? BUTTON_FWD : 0);
                palm->setTrigger(state.buttons.at(2) ? 1.0f : 0.0f);
                palm->setJoystick(state.axes.at(2), -state.axes.at(3)); 
            }
        }
    }
    
    glm::vec3 position;
    glm::quat rotation;
    
    SkeletonModel* skeletonModel = &Application::getInstance()->getAvatar()->getSkeletonModel();
    int jointIndex;
    glm::quat inverseRotation = glm::inverse(Application::getInstance()->getAvatar()->getOrientation());
    if (index == LEFT_HAND_INDEX) {
        jointIndex = skeletonModel->getLeftHandJointIndex();
        skeletonModel->getJointRotation(jointIndex, rotation, true);      
        rotation = inverseRotation * rotation * glm::quat(glm::vec3(0.0f, PI_OVER_TWO, 0.0f));
        
    } else {
        jointIndex = skeletonModel->getRightHandJointIndex();
        skeletonModel->getJointRotation(jointIndex, rotation, true);
        rotation = inverseRotation * rotation * glm::quat(glm::vec3(0.0f, -PI_OVER_TWO, 0.0f));
    }
    skeletonModel->getJointPosition(jointIndex, position);
    position = inverseRotation * (position - skeletonModel->getTranslation());
    
    palm->setRawRotation(rotation);
    
    //  Compute current velocity from position change
    glm::vec3 rawVelocity;
    if (deltaTime > 0.f) {
        rawVelocity = (position - palm->getRawPosition()) / deltaTime; 
    } else {
        rawVelocity = glm::vec3(0.0f);
    }
    palm->setRawVelocity(rawVelocity);
    palm->setRawPosition(position);
    
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
*/

// default (expected) location of neck in sixense space
const float LEAP_X = 0.25f; // meters
const float LEAP_Y = 0.3f;  // meters
const float LEAP_Z = 0.3f;  // meters
#endif

LeapMotionManager::LeapMotionManager() {
#ifdef HAVE_LEAPMOTION

    // Have the sample listener receive events from the controller
    _controller.addListener(_listener);

    // By default we assume the _neckBase (in orb frame) is as high above the orb 
    // as the "torso" is below it.
    _leapBasePos = glm::vec3(0, -LEAP_Y, LEAP_Z);

    glm::vec3 xAxis(1.f, 0.f, 0.f);
    glm::vec3 yAxis(0.f, 1.f, 0.f);
    glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, yAxis));
    xAxis = glm::normalize(glm::cross(yAxis, zAxis));
    _leapBaseOri = glm::inverse(glm::quat_cast(glm::mat3(xAxis, yAxis, zAxis)));

#endif
}

LeapMotionManager::~LeapMotionManager() {
#ifdef HAVE_LEAPMOTION
    // Remove the sample listener when done
    _controller.removeListener(_listener);
#endif
}

const int HEAD_ROTATION_INDEX = 0;

glm::vec3 LeapMotionManager::getHandPos( unsigned int handNb ) const
{
    if ( handNb < _hands.size() )
    {
        return _hands[ handNb ];
    }
    else
        return glm::vec3(0.f);
}

void LeapMotionManager::update(float deltaTime) {
#ifdef HAVE_LEAPMOTION

    if ( !_controller.isConnected() )
        return;


    // Get the most recent frame and report some basic information
    const Leap::Frame frame = _controller.frame();
    static _int64 lastFrame = -1;
    _hands.clear();
    _int64 newFrameNb = frame.id();

    if ( (lastFrame >= newFrameNb) )
        return;

    glm::vec3 delta(0.f);
    glm::quat handOri;
    if (!frame.hands().isEmpty())
    {
        // Get the first hand
        const Leap::Hand hand = frame.hands()[0];
        Leap::Vector lp = hand.palmPosition();
        glm::vec3 p(lp.x * METERS_PER_MILLIMETER, lp.y * METERS_PER_MILLIMETER, lp.z * METERS_PER_MILLIMETER );

        Leap::Vector n = hand.palmNormal();
        glm::vec3 xAxis(n.x, n.y, n.z);
        glm::vec3 yAxis(0.f, 1.f, 0.f);
        glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, yAxis));
        xAxis = glm::normalize(glm::cross(yAxis, zAxis));
        handOri = glm::inverse(glm::quat_cast(glm::mat3(xAxis, yAxis, zAxis)));

        _hands.push_back( p );


        //Leap::Vector dp = hand.translation( _controller.frame( lastFrame ) );
        Leap::Vector dp = hand.palmVelocity();
        delta = glm::vec3( dp.x * METERS_PER_MILLIMETER, dp.y * METERS_PER_MILLIMETER, dp.z * METERS_PER_MILLIMETER);
    }
    lastFrame = newFrameNb;

    MyAvatar* avatar = Application::getInstance()->getAvatar();
    Hand* hand = avatar->getHand();

//    for ( int h = 0; h < frame.hands().count(); h++ )
    if ( _hands.size() )
    {

        //  Set palm position and normal based on Hydra position/orientation
        
        // Either find a palm matching the sixense controller, or make a new one
        PalmData* palm;
        bool foundHand = false;
        for (size_t j = 0; j < hand->getNumPalms(); j++) {
            if (hand->getPalms()[j].getSixenseID() == 28) {
                palm = &(hand->getPalms()[j]);
                foundHand = true;
            }
        }
        if (!foundHand) {
            PalmData newPalm(hand);
            hand->getPalms().push_back(newPalm);
            palm = &(hand->getPalms()[hand->getNumPalms() - 1]);
            palm->setSixenseID(28);
            qDebug("Found new LeapMotion hand, ID %i", 28);
        }
        
        palm->setActive(true);
        
        //  Read controller buttons and joystick into the hand
        //palm->setControllerButtons(data->buttons);
        //palm->setTrigger(data->trigger);
        //palm->setJoystick(data->joystick_x, data->joystick_y);

        glm::vec3 position(_hands[0]);

        // Transform the measured position into body frame.  
        glm::vec3 neck = _leapBasePos;
        // Zeroing y component of the "neck" effectively raises the measured position a little bit.
        //neck.y = 0.f;
        position = _leapBaseOri * (position - neck);

        //  Rotation of Palm
        glm::quat rotation(handOri[3], -handOri[0], handOri[1], -handOri[2]);
        rotation = glm::angleAxis(PI, glm::vec3(0.f, 1.f, 0.f)) * _leapBaseOri * rotation;

        //  Compute current velocity from position change
        glm::vec3 rawVelocity;
        if (deltaTime > 0.f) {
           // rawVelocity = (position - palm->getRawPosition()) / deltaTime; 
            rawVelocity = delta / deltaTime; 
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
      /*  const float MOVEMENT_DISTANCE_THRESHOLD = 0.003f;
        _amountMoved += rawVelocity * deltaTime;
        if (glm::length(_amountMoved) > MOVEMENT_DISTANCE_THRESHOLD && foundHand) {
            _lastMovement = usecTimestampNow();
            _amountMoved = glm::vec3(0.0f);
        }*/
        
        // Store the one fingertip in the palm structure so we can track velocity
    /*    const float FINGER_LENGTH = 0.3f;   //  meters
        const glm::vec3 FINGER_VECTOR(0.0f, 0.0f, FINGER_LENGTH);
        const glm::vec3 newTipPosition = position + rotation * FINGER_VECTOR;
        glm::vec3 oldTipPosition = palm->getTipRawPosition();
        if (deltaTime > 0.f) {
            palm->setTipVelocity((newTipPosition - oldTipPosition) / deltaTime);
        } else {
            palm->setTipVelocity(glm::vec3(0.f));
        }
        palm->setTipPosition(newTipPosition);*/
    }
    else
    {
        // Either find a palm matching the sixense controller, or make a new one
        PalmData* palm;
        bool foundHand = false;
        for (size_t j = 0; j < hand->getNumPalms(); j++) {
            if (hand->getPalms()[j].getSixenseID() == 28) {
                palm = &(hand->getPalms()[j]);
                foundHand = true;
            }
        }
        if (foundHand) {
            palm->setRawPosition(palm->getRawPosition());
            palm->setRawRotation(palm->getRawRotation());
            palm->setActive(false);
        }
    }


 /*   if (numActiveControllers == 2) {
        updateCalibration(controllers);
    }
    */

   // }

    if ( false )
    {

        std::cout << "Frame id: " << frame.id()
                  << ", timestamp: " << frame.timestamp()
                  << ", hands: " << frame.hands().count()
                  << ", fingers: " << frame.fingers().count()
                  << ", tools: " << frame.tools().count() << std::endl;

        if (!frame.hands().isEmpty()) {
            // Get the first hand
            const Leap::Hand hand = frame.hands()[0];

            // Check if the hand has any fingers
            const Leap::FingerList fingers = hand.fingers();
            if (!fingers.isEmpty()) {
                // Calculate the hand's average finger tip position
                Leap::Vector avgPos;
                for (int i = 0; i < fingers.count(); ++i) {
                    avgPos += fingers[i].tipPosition();
                }

                avgPos /= (float)fingers.count();
                std::cout << "Hand has " << fingers.count()
                    << " fingers, average finger tip position" << avgPos << std::endl;
            }

            // Get the hand's sphere radius and palm position
            std::cout << "Hand sphere radius: " << hand.sphereRadius()
                      << " mm, palm position: " << hand.palmPosition() << std::endl;

            // Get the hand's normal vector and direction
            const Leap::Vector normal = hand.palmNormal();
            const Leap::Vector direction = hand.direction();

            // Calculate the hand's pitch, roll, and yaw angles
            const float RAD_TO_DEG = 180.0 / 3.1415;
            std::cout << "Hand pitch: " << direction.pitch() * RAD_TO_DEG << " degrees, "
                      << "roll: " << normal.roll() * RAD_TO_DEG << " degrees, "
                      << "yaw: " << direction.yaw() * RAD_TO_DEG << " degrees" << std::endl << std::endl;


        }
    }

#endif
}

void LeapMotionManager::reset() {
#ifdef HAVE_LEAPMOTION
    if (!_controller.isConnected()) {
        return;
    }
  /*  connect(Application::getInstance(), SIGNAL(renderingOverlay()), SLOT(renderCalibrationCountdown()));
    _calibrationCountdownStarted = QDateTime::currentDateTime();
    */
#endif
}

void LeapMotionManager::renderCalibrationCountdown() {
#ifdef HAVE_LEAPMOTION
  /*  const int COUNTDOWN_SECONDS = 3;
    int secondsRemaining = COUNTDOWN_SECONDS - _calibrationCountdownStarted.secsTo(QDateTime::currentDateTime());
    if (secondsRemaining == 0) {
        yei_tareSensors(_skeletalDevice);
        Application::getInstance()->disconnect(this);
        return;
    }
    static TextRenderer textRenderer(MONO_FONT_FAMILY, 18, QFont::Bold, false, TextRenderer::OUTLINE_EFFECT, 2);
    QByteArray text = "Assume T-Pose in " + QByteArray::number(secondsRemaining) + "...";
    textRenderer.draw((Application::getInstance()->getGLWidget()->width() - textRenderer.computeWidth(text.constData())) / 2,
        Application::getInstance()->getGLWidget()->height() / 2,
        text);
        */
#endif  
}
