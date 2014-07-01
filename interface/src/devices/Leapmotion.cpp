//
//  Leapmotion.cpp
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
#include "Leapmotion.h"
#include "ui/TextRenderer.h"


#ifdef HAVE_LEAPMOTION


Leapmotion::SampleListener::SampleListener() : ::Leap::Listener()
{
//    std::cout << __FUNCTION__ << std::endl;
}

Leapmotion::SampleListener::~SampleListener()
{
//    std::cout << __FUNCTION__ << std::endl;
}

void Leapmotion::SampleListener::onConnect(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void Leapmotion::SampleListener::onDisconnect(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void Leapmotion::SampleListener::onExit(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void Leapmotion::SampleListener::onFocusGained(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void Leapmotion::SampleListener::onFocusLost(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void Leapmotion::SampleListener::onFrame(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void Leapmotion::SampleListener::onInit(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void Leapmotion::SampleListener::onServiceConnect(const ::Leap::Controller &)
{
//    std::cout << __FUNCTION__ << std::endl;
}
void Leapmotion::SampleListener::onServiceDisconnect(const ::Leap::Controller &)
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

const int FINGER_NUM_JOINTS = 4;
const int HAND_NUM_JOINTS = FINGER_NUM_JOINTS*5+1;

Leapmotion::Leapmotion() :
     MotionTracker(),
    _enabled(false),
    _active(false)
{
#ifdef HAVE_LEAPMOTION

    // Have the sample listener receive events from the controller
    _controller.addListener(_listener);

    reset();

#endif

    // Create the Leapmotion joint hierarchy
    {
        std::vector< Semantic > hands;
        hands.push_back( "Left" );
        hands.push_back( "Right" );

        std::vector< Semantic > fingers;
        fingers.push_back( "Thumb" );
        fingers.push_back( "Index" );
        fingers.push_back( "Middle" );
        fingers.push_back( "Ring" );
        fingers.push_back( "Pinky" );

        std::vector< Semantic > fingerBones;
/*      fingerBones.push_back( "Metacarpal" );
        fingerBones.push_back( "Proximal" );
        fingerBones.push_back( "Intermediate" );
        fingerBones.push_back( "Distal" );
*/
        fingerBones.push_back( "1" );
        fingerBones.push_back( "2" );
        fingerBones.push_back( "3" );
        fingerBones.push_back( "4" );

        std::vector< Index > palms;
        for ( int h = 0; h < hands.size(); h++ ) {
            Index rootJoint = addJoint( hands[h] + "Hand", 0 );
            palms.push_back( rootJoint );

            for ( int f = 0; f < fingers.size(); f++ ) {
                for ( int b = 0; b < fingerBones.size(); b++ ) {
                    rootJoint  = addJoint( hands[h] + "Hand" + fingers[f] + fingerBones[b], rootJoint );
                }
            }
        }
    }
}

Leapmotion::~Leapmotion() {
#ifdef HAVE_LEAPMOTION
    // Remove the sample listener when done
    _controller.removeListener(_listener);
#endif
}


void Leapmotion::init() {
 //   connect(Application::getInstance()->getFaceshift(), SIGNAL(connectionStateChanged()), SLOT(updateEnabled()));
    updateEnabled();
}

#ifdef HAVE_LEAPMOTION
glm::quat quatFromLeapBase( float sideSign, const Leap::Matrix& basis ) {

    glm::vec3 xAxis = glm::normalize( sideSign * glm::vec3( basis.xBasis.x, basis.xBasis.y, basis.xBasis.z) );
    glm::vec3 yAxis = glm::normalize( glm::vec3( basis.yBasis.x, basis.yBasis.y, basis.yBasis.z) );
    glm::vec3 zAxis = glm::normalize( glm::vec3( basis.zBasis.x, basis.zBasis.y, basis.zBasis.z) );

    glm::quat orientation = /* glm::inverse*/ (glm::quat_cast(glm::mat3(xAxis, yAxis, zAxis)));

    return orientation;
}

glm::vec3 vec3FromLeapVector( const Leap::Vector& vec ) {
    return glm::vec3( vec.x * METERS_PER_MILLIMETER, vec.y * METERS_PER_MILLIMETER, vec.z * METERS_PER_MILLIMETER );
}

#endif

void Leapmotion::update() {
#ifdef HAVE_LEAPMOTION
    _active = _controller.isConnected();
    if (!_active) {
        return;
    }

    // go through all the joints and increment their counter since last update
    for ( auto jointIt = _jointsArray.begin(); jointIt != _jointsArray.end(); jointIt++ ) {
        (*jointIt).tickNewFrame();
    }

    float deltaTime = 0.001f;

    // Get the most recent frame and report some basic information
    const Leap::Frame frame = _controller.frame();
    static _int64 lastFrame = -1;
    _int64 newFrameNb = frame.id();

    if ( (lastFrame >= newFrameNb) )
        return;

    glm::vec3 delta(0.f);
    glm::quat handOri;
    if ( !frame.hands().isEmpty() ) {
        for ( int handNum = 0; handNum < frame.hands().count(); handNum++ ) {
            // Get the first hand
            const Leap::Hand hand = frame.hands()[handNum];
            int side = ( hand.isRight() ? -1 : 1 );
            Index handIndex = 1 + ((1 - side)/2) * HAND_NUM_JOINTS;

            glm::vec3 pos = vec3FromLeapVector(hand.palmPosition());
            glm::quat ori = quatFromLeapBase(float(side), hand.basis() );

            JointTracker* palmJoint = editJointTracker( handIndex );
            palmJoint->editLocFrame().setTranslation( pos );
            palmJoint->editLocFrame().setRotation( ori );
            palmJoint->editAbsFrame().setTranslation( pos );
            palmJoint->editAbsFrame().setRotation( ori );
            palmJoint->activeFrame();

            // Transform the measured position into body frame.  
            glm::vec3 neck = _leapBasePos;
            // Zeroing y component of the "neck" effectively raises the measured position a little bit.
            //neck.y = 0.f;
            pos = _leapBaseOri * (pos - neck);


            // Check if the hand has any fingers
            const Leap::FingerList fingers = hand.fingers();
            if (!fingers.isEmpty()) {
                // For every fingers in the list
                for (int i = 0; i < fingers.count(); ++i) {
                    // Reset the parent joint to the palmJoint for every finger traversal
                    JointTracker* parentJointTracker = palmJoint;

                    // surprisingly, Leap::Finger::Type start at 0 for thumb a until 4 for the pinky
                    Index fingerIndex = handIndex + 1 + Index(fingers[i].type()) * FINGER_NUM_JOINTS;

                    // let's update the finger's joints
                    for ( int b = 0; b < FINGER_NUM_JOINTS; b++ ) {
                        Leap::Bone::Type type = Leap::Bone::Type(b + Leap::Bone::TYPE_METACARPAL);
                        Leap::Bone bone = fingers[i].bone( type );
                        JointTracker* ljointTracker = editJointTracker( fingerIndex + b );
                        if ( bone.isValid() ) {
                            Leap::Vector bp = bone.nextJoint();

                            ljointTracker->editAbsFrame().setTranslation( vec3FromLeapVector( bp ) );
                            ljointTracker->editAbsFrame().setRotation(quatFromLeapBase( float(side), bone.basis() ) );
                            ljointTracker->updateLocFromAbsTransform( parentJointTracker );
                            ljointTracker->activeFrame();
                        }
                        parentJointTracker = ljointTracker;
                    }
                }
            }
        }
    }

    lastFrame = newFrameNb;



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

void Leapmotion::reset() {
   // By default we assume the _neckBase (in orb frame) is as high above the orb 
    // as the "torso" is below it.
    _leapBasePos = glm::vec3(0, -LEAP_Y, LEAP_Z);

    glm::vec3 xAxis(1.f, 0.f, 0.f);
    glm::vec3 yAxis(0.f, 1.f, 0.f);
    glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, yAxis));
    xAxis = glm::normalize(glm::cross(yAxis, zAxis));
    _leapBaseOri = glm::inverse(glm::quat_cast(glm::mat3(xAxis, yAxis, zAxis)));
}

void Leapmotion::updateEnabled() {
  /*  setEnabled(Menu::getInstance()->isOptionChecked(MenuOption::Visage) &&
        !Menu::getInstance()->isOptionChecked(MenuOption::Faceplus) && 
        !(Menu::getInstance()->isOptionChecked(MenuOption::Faceshift) &&
            Application::getInstance()->getFaceshift()->isConnectedOrConnecting()));*/
}

void Leapmotion::setEnabled(bool enabled) {
#ifdef HAVE_LEAPMOTION
    if (_enabled == enabled) {
        return;
    }
#endif
}
