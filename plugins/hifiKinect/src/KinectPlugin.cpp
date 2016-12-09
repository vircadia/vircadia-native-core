//
//  KinectPlugin.cpp
//
//  Created by Brad Hefta-Gaub on 2016/12/7
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "KinectPlugin.h"

#include <controllers/UserInputMapper.h>
#include <QLoggingCategory>
#include <PathUtils.h>
#include <DebugDraw.h>
#include <cassert>
#include <NumericalConstants.h>
#include <StreamUtils.h>
#include <Preferences.h>
#include <SettingHandle.h>

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")


const char* KinectPlugin::NAME = "Kinect";
const char* KinectPlugin::KINECT_ID_STRING = "Kinect";

QStringList jointNames = {
    "SpineBase",
    "SpineMid",
    "Neck",
    "Head",
    "ShoulderLeft",
    "ElbowLeft",
    "WristLeft",
    "HandLeft",
    "ShoulderRight",
    "ElbowRight",
    "WristRight",
    "HandRight",
    "HipLeft",
    "KneeLeft",
    "AnkleLeft",
    "FootLeft",
    "HipRight",
    "KneeRight",
    "AnkleRight",
    "FootRight",
    "SpineShoulder",
    "HandTipLeft",
    "ThumbLeft",
    "HandTipRight",
    "ThumbRight"
};

const bool DEFAULT_ENABLED = false;

// indices of joints of the Kinect standard skeleton.
// This is 'almost' the same as the High Fidelity standard skeleton.
// It is missing a thumb joint.
enum KinectJointIndex {
    SpineBase = 0,
    SpineMid,
    Neck,
    Head,

    ShoulderLeft,
    ElbowLeft,
    WristLeft,
    HandLeft,

    ShoulderRight,
    ElbowRight,
    WristRight,
    HandRight,

    HipLeft,
    KneeLeft,
    AnkleLeft,
    FootLeft,

    HipRight,
    KneeRight,
    AnkleRight,
    FootRight,

    SpineShoulder,

    HandTipLeft,
    ThumbLeft,

    HandTipRight,
    ThumbRight,

    Size
};

// -1 or 0???
#define UNKNOWN_JOINT (controller::StandardPoseChannel)0 

// Almost a direct mapping except for LEFT_HAND_THUMB1 and RIGHT_HAND_THUMB1,
// which are not present in the Kinect standard skeleton.
static controller::StandardPoseChannel KinectJointIndexToPoseIndexMap[KinectJointIndex::Size] = {
    controller::HIPS,
    controller::SPINE,
    controller::NECK,
    controller::HEAD,

    controller::LEFT_SHOULDER,
    controller::LEFT_ARM,
    controller::LEFT_FORE_ARM,
    controller::LEFT_HAND,

    controller::RIGHT_SHOULDER,
    controller::RIGHT_ARM,
    controller::RIGHT_FORE_ARM,
    controller::RIGHT_HAND,

    controller::RIGHT_UP_LEG,   // hip socket
    controller::RIGHT_LEG,      // knee?
    controller::RIGHT_FOOT,     // ankle?
    UNKNOWN_JOINT,              // ????

    controller::LEFT_UP_LEG,   // hip socket
    controller::LEFT_LEG,      // knee?
    controller::LEFT_FOOT,     // ankle?
    UNKNOWN_JOINT,              // ????

    UNKNOWN_JOINT, /* SpineShoulder */

    controller::LEFT_HAND_INDEX4,
    controller::LEFT_HAND_THUMB4,

    controller::RIGHT_HAND_INDEX4,
    controller::RIGHT_HAND_THUMB4,


    /**
    controller::SPINE1,
    controller::SPINE2,
    controller::SPINE3,

    controller::RIGHT_HAND_THUMB2,
    controller::RIGHT_HAND_THUMB3,
    controller::RIGHT_HAND_THUMB4,
    controller::RIGHT_HAND_INDEX1,
    controller::RIGHT_HAND_INDEX2,
    controller::RIGHT_HAND_INDEX3,
    controller::RIGHT_HAND_INDEX4,
    controller::RIGHT_HAND_MIDDLE1,
    controller::RIGHT_HAND_MIDDLE2,
    controller::RIGHT_HAND_MIDDLE3,
    controller::RIGHT_HAND_MIDDLE4,
    controller::RIGHT_HAND_RING1,
    controller::RIGHT_HAND_RING2,
    controller::RIGHT_HAND_RING3,
    controller::RIGHT_HAND_RING4,
    controller::RIGHT_HAND_PINKY1,
    controller::RIGHT_HAND_PINKY2,
    controller::RIGHT_HAND_PINKY3,
    controller::RIGHT_HAND_PINKY4,

    controller::LEFT_HAND_THUMB2,
    controller::LEFT_HAND_THUMB3,
    controller::LEFT_HAND_THUMB4,
    controller::LEFT_HAND_INDEX1,
    controller::LEFT_HAND_INDEX2,
    controller::LEFT_HAND_INDEX3,
    controller::LEFT_HAND_INDEX4,
    controller::LEFT_HAND_MIDDLE1,
    controller::LEFT_HAND_MIDDLE2,
    controller::LEFT_HAND_MIDDLE3,
    controller::LEFT_HAND_MIDDLE4,
    controller::LEFT_HAND_RING1,
    controller::LEFT_HAND_RING2,
    controller::LEFT_HAND_RING3,
    controller::LEFT_HAND_RING4,
    controller::LEFT_HAND_PINKY1,
    controller::LEFT_HAND_PINKY2,
    controller::LEFT_HAND_PINKY3,
    controller::LEFT_HAND_PINKY4
    **/
};

// in rig frame
static glm::vec3 rightHandThumb1DefaultAbsTranslation(-2.155500650405884, -0.7610001564025879, 2.685631036758423);
static glm::vec3 leftHandThumb1DefaultAbsTranslation(2.1555817127227783, -0.7603635787963867, 2.6856393814086914);

static controller::StandardPoseChannel KinectJointIndexToPoseIndex(KinectJointIndex i) {
    assert(i >= 0 && i < KinectJointIndex::Size);
    if (i >= 0 && i < KinectJointIndex::Size) {
        return KinectJointIndexToPoseIndexMap[i];
    } else {
        return UNKNOWN_JOINT; // not sure what to do here, but don't crash!
    }
}

static const char* controllerJointName(controller::StandardPoseChannel i) {
    switch (i) {
        case controller::HIPS: return "Hips";
        case controller::RIGHT_UP_LEG: return "RightUpLeg";
        case controller::RIGHT_LEG: return "RightLeg";
        case controller::RIGHT_FOOT: return "RightFoot";
        case controller::LEFT_UP_LEG: return "LeftUpLeg";
        case controller::LEFT_LEG: return "LeftLeg";
        case controller::LEFT_FOOT: return "LeftFoot";
        case controller::SPINE: return "Spine";
        case controller::SPINE1: return "Spine1";
        case controller::SPINE2: return "Spine2";
        case controller::SPINE3: return "Spine3";
        case controller::NECK: return "Neck";
        case controller::HEAD: return "Head";
        case controller::RIGHT_SHOULDER: return "RightShoulder";
        case controller::RIGHT_ARM: return "RightArm";
        case controller::RIGHT_FORE_ARM: return "RightForeArm";
        case controller::RIGHT_HAND: return "RightHand";
        case controller::RIGHT_HAND_THUMB1: return "RightHandThumb1";
        case controller::RIGHT_HAND_THUMB2: return "RightHandThumb2";
        case controller::RIGHT_HAND_THUMB3: return "RightHandThumb3";
        case controller::RIGHT_HAND_THUMB4: return "RightHandThumb4";
        case controller::RIGHT_HAND_INDEX1: return "RightHandIndex1";
        case controller::RIGHT_HAND_INDEX2: return "RightHandIndex2";
        case controller::RIGHT_HAND_INDEX3: return "RightHandIndex3";
        case controller::RIGHT_HAND_INDEX4: return "RightHandIndex4";
        case controller::RIGHT_HAND_MIDDLE1: return "RightHandMiddle1";
        case controller::RIGHT_HAND_MIDDLE2: return "RightHandMiddle2";
        case controller::RIGHT_HAND_MIDDLE3: return "RightHandMiddle3";
        case controller::RIGHT_HAND_MIDDLE4: return "RightHandMiddle4";
        case controller::RIGHT_HAND_RING1: return "RightHandRing1";
        case controller::RIGHT_HAND_RING2: return "RightHandRing2";
        case controller::RIGHT_HAND_RING3: return "RightHandRing3";
        case controller::RIGHT_HAND_RING4: return "RightHandRing4";
        case controller::RIGHT_HAND_PINKY1: return "RightHandPinky1";
        case controller::RIGHT_HAND_PINKY2: return "RightHandPinky2";
        case controller::RIGHT_HAND_PINKY3: return "RightHandPinky3";
        case controller::RIGHT_HAND_PINKY4: return "RightHandPinky4";
        case controller::LEFT_SHOULDER: return "LeftShoulder";
        case controller::LEFT_ARM: return "LeftArm";
        case controller::LEFT_FORE_ARM: return "LeftForeArm";
        case controller::LEFT_HAND: return "LeftHand";
        case controller::LEFT_HAND_THUMB1: return "LeftHandThumb1";
        case controller::LEFT_HAND_THUMB2: return "LeftHandThumb2";
        case controller::LEFT_HAND_THUMB3: return "LeftHandThumb3";
        case controller::LEFT_HAND_THUMB4: return "LeftHandThumb4";
        case controller::LEFT_HAND_INDEX1: return "LeftHandIndex1";
        case controller::LEFT_HAND_INDEX2: return "LeftHandIndex2";
        case controller::LEFT_HAND_INDEX3: return "LeftHandIndex3";
        case controller::LEFT_HAND_INDEX4: return "LeftHandIndex4";
        case controller::LEFT_HAND_MIDDLE1: return "LeftHandMiddle1";
        case controller::LEFT_HAND_MIDDLE2: return "LeftHandMiddle2";
        case controller::LEFT_HAND_MIDDLE3: return "LeftHandMiddle3";
        case controller::LEFT_HAND_MIDDLE4: return "LeftHandMiddle4";
        case controller::LEFT_HAND_RING1: return "LeftHandRing1";
        case controller::LEFT_HAND_RING2: return "LeftHandRing2";
        case controller::LEFT_HAND_RING3: return "LeftHandRing3";
        case controller::LEFT_HAND_RING4: return "LeftHandRing4";
        case controller::LEFT_HAND_PINKY1: return "LeftHandPinky1";
        case controller::LEFT_HAND_PINKY2: return "LeftHandPinky2";
        case controller::LEFT_HAND_PINKY3: return "LeftHandPinky3";
        case controller::LEFT_HAND_PINKY4: return "LeftHandPinky4";
        default: return "???";
    }
}

// convert between YXZ Kinect euler angles in degrees to quaternion
// this is the default setting in the Axis Kinect server.
static quat eulerToQuat(const vec3& e) {
    // euler.x and euler.y are swaped, WTF.
    return (glm::angleAxis(e.x * RADIANS_PER_DEGREE, Vectors::UNIT_Y) *
            glm::angleAxis(e.y * RADIANS_PER_DEGREE, Vectors::UNIT_X) *
            glm::angleAxis(e.z * RADIANS_PER_DEGREE, Vectors::UNIT_Z));
}

//
// KinectPlugin
//

void KinectPlugin::init() {
    loadSettings();

    auto preferences = DependencyManager::get<Preferences>();
    static const QString KINECT_PLUGIN { "Kinect" };
    {
        auto getter = [this]()->bool { return _enabled; };
        auto setter = [this](bool value) { _enabled = value; saveSettings(); };
        auto preference = new CheckPreference(KINECT_PLUGIN, "Enabled", getter, setter);
        preferences->addPreference(preference);
    }
}

bool KinectPlugin::isSupported() const {
    // FIXME -- check to see if there's a camera or not...
    return true;
}

bool KinectPlugin::activate() {
    InputPlugin::activate();

    loadSettings();

    if (_enabled) {

        // register with userInputMapper
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->registerDevice(_inputDevice);

        return initializeDefaultSensor();
    } else {
        return false;
    }
}

bool KinectPlugin::initializeDefaultSensor() {
#ifdef HAVE_KINECT
    HRESULT hr;

    hr = GetDefaultKinectSensor(&_kinectSensor);
    if (FAILED(hr)) {
        return false;
    }

    if (_kinectSensor) {
        // Initialize the Kinect and get coordinate mapper and the body reader
        IBodyFrameSource* bodyFrameSource = NULL;

        hr = _kinectSensor->Open();

        if (SUCCEEDED(hr)) {
            hr = _kinectSensor->get_CoordinateMapper(&_coordinateMapper);
        }

        if (SUCCEEDED(hr)) {
            hr = _kinectSensor->get_BodyFrameSource(&bodyFrameSource);
        }

        if (SUCCEEDED(hr)) {
            hr = bodyFrameSource->OpenReader(&_bodyFrameReader);
        }

        SafeRelease(bodyFrameSource);
    }

    if (!_kinectSensor || FAILED(hr)) {
        qDebug() << "No ready Kinect found!";
        return false;
    }

    qDebug() << "Kinect found WOOT!";

    return true;
#else
    return false;
#endif
}

/// <summary>
/// Converts a body point to screen space
/// </summary>
/// <param name="bodyPoint">body point to tranform</param>
/// <param name="width">width (in pixels) of output buffer</param>
/// <param name="height">height (in pixels) of output buffer</param>
/// <returns>point in screen-space</returns>
#if 0 
D2D1_POINT_2F KinectPlugin::BodyToScreen(const CameraSpacePoint& bodyPoint, int width, int height) {
    // Calculate the body's position on the screen
    DepthSpacePoint depthPoint = { 0 };
    _coordinateMapper->MapCameraPointToDepthSpace(bodyPoint, &depthPoint);

    float screenPointX = static_cast<float>(depthPoint.X * width) / cDepthWidth;
    float screenPointY = static_cast<float>(depthPoint.Y * height) / cDepthHeight;

    return D2D1::Point2F(screenPointX, screenPointY);
}
#endif

void KinectPlugin::updateBody() {
#ifndef HAVE_KINECT
    return;
#else
    if (!_bodyFrameReader) {
        return;
    }

    IBodyFrame* pBodyFrame = NULL;

    HRESULT hr = _bodyFrameReader->AcquireLatestFrame(&pBodyFrame);

    if (SUCCEEDED(hr)) {

        //qDebug() << __FUNCTION__ << "line:" << __LINE__;

        INT64 nTime = 0;

        hr = pBodyFrame->get_RelativeTime(&nTime);

        IBody* ppBodies[BODY_COUNT] = {0};

        if (SUCCEEDED(hr)) {
            hr = pBodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
        }

        if (SUCCEEDED(hr)) {
            ProcessBody(nTime, BODY_COUNT, ppBodies);
        }

        for (int i = 0; i < _countof(ppBodies); ++i) {
            SafeRelease(ppBodies[i]);
        }
    }

    SafeRelease(pBodyFrame);
#endif
}

#ifdef HAVE_KINECT
/// <summary>
/// Handle new body data
/// <param name="nTime">timestamp of frame</param>
/// <param name="nBodyCount">body data count</param>
/// <param name="ppBodies">body data in frame</param>
/// </summary>
void KinectPlugin::ProcessBody(INT64 nTime, int nBodyCount, IBody** ppBodies) {
    bool foundOneBody = false;
    if (_coordinateMapper) {
        for (int i = 0; i < nBodyCount; ++i) {
            if (foundOneBody) {
                break;
            }
            IBody* pBody = ppBodies[i];
            if (pBody) {
                BOOLEAN tracked = false;
                HRESULT hr = pBody->get_IsTracked(&tracked);

                if (SUCCEEDED(hr) && tracked) {
                    foundOneBody = true;

                    if (_joints.size() != JointType_Count) {
                        _joints.resize(JointType_Count, { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } });
                    }


                    Joint joints[JointType_Count];
                    JointOrientation jointOrientations[JointType_Count];
                    HandState leftHandState = HandState_Unknown;
                    HandState rightHandState = HandState_Unknown;

                    pBody->get_HandLeftState(&leftHandState);
                    pBody->get_HandRightState(&rightHandState);

                    hr = pBody->GetJoints(_countof(joints), joints);
                    hr = pBody->GetJointOrientations(_countof(jointOrientations), jointOrientations);

                    if (SUCCEEDED(hr)) {
                        auto jointCount = _countof(joints);
                        //qDebug() << __FUNCTION__ << "nBodyCount:" << nBodyCount << "body:" << i << "jointCount:" << jointCount;
                        QString state;
                        for (int j = 0; j < jointCount; ++j) {
                            QString jointName = jointNames[joints[j].JointType];

                            switch(joints[j].TrackingState) {
                                case TrackingState_NotTracked: 
                                    state = "Not Tracked"; 
                                    break;
                                case TrackingState_Inferred:
                                    state = "Inferred";
                                    break;
                                case TrackingState_Tracked:
                                    state = "Tracked";
                                    break;
                            }
                            glm::vec3 jointPosition { joints[j].Position.X,
                                                      joints[j].Position.Y,
                                                      joints[j].Position.Z };

                            if (joints[j].TrackingState != TrackingState_NotTracked) {
                                _joints[j].pos = jointPosition;

                                if (joints[j].JointType == JointType_HandLeft ||
                                    joints[j].JointType == JointType_HandRight) {
                                    //qDebug() << __FUNCTION__ << jointName << ":" << state << "pos:" << jointPosition;
                                }

                            }


                            // FIXME - do something interesting with the joints...
                            //jointPoints[j] = BodyToScreen(joints[j].Position, width, height);
                        }
                    }
                }
            }
        }
    }
}
#endif


void KinectPlugin::deactivate() {
    // unregister from userInputMapper
    if (_inputDevice->_deviceID != controller::Input::INVALID_DEVICE) {
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->removeDevice(_inputDevice->_deviceID);
    }

    InputPlugin::deactivate();
    saveSettings();

#ifdef HAVE_KINECT
    // done with body frame reader
    SafeRelease(_bodyFrameReader);

    // done with coordinate mapper
    SafeRelease(_coordinateMapper);

    // close the Kinect Sensor
    if (_kinectSensor)
    {
        _kinectSensor->Close();
    }

    SafeRelease(_kinectSensor);
#endif // HAVE_KINECT

}

void KinectPlugin::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {

    if (!_enabled) {
        return;
    }

    //qDebug() << __FUNCTION__ << "deltaTime:" << deltaTime;

    updateBody(); // updates _joints

    std::vector<KinectJoint> joints = _joints;

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData, joints, _prevJoints);
    });

    _prevJoints = joints;
}

void KinectPlugin::saveSettings() const {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.setValue(QString("enabled"), _enabled);
    }
    settings.endGroup();
    qDebug() << __FUNCTION__ << "_enabled:" << _enabled;
}

void KinectPlugin::loadSettings() {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        // enabled
        _enabled = settings.value("enabled", QVariant(DEFAULT_ENABLED)).toBool();
    }
    settings.endGroup();
    qDebug() << __FUNCTION__ << "_enabled:" << _enabled;
}

//
// InputDevice
//

// FIXME - we probably should only return the inputs we ACTUALLY have
controller::Input::NamedVector KinectPlugin::InputDevice::getAvailableInputs() const {
    static controller::Input::NamedVector availableInputs;
    if (availableInputs.size() == 0) {
        for (int i = 0; i < controller::NUM_STANDARD_POSES; i++) {
            auto channel = static_cast<controller::StandardPoseChannel>(i);
            availableInputs.push_back(makePair(channel, controllerJointName(channel)));
        }
    };
    return availableInputs;
}

QString KinectPlugin::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/kinect.json";
    return MAPPING_JSON;
}

void KinectPlugin::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, 
                                        const std::vector<KinectPlugin::KinectJoint>& joints, const std::vector<KinectPlugin::KinectJoint>& prevJoints) {
    for (size_t i = 0; i < joints.size(); i++) {
        int poseIndex = KinectJointIndexToPoseIndex((KinectJointIndex)i);
        glm::vec3 linearVel, angularVel;
        const glm::vec3& pos = joints[i].pos;
        const glm::vec3& rotEuler = joints[i].euler;

        if (Vectors::ZERO == pos && Vectors::ZERO == rotEuler) {
            _poseStateMap[poseIndex] = controller::Pose();
            continue;
        }


        glm::quat rot = eulerToQuat(rotEuler);
        if (i < prevJoints.size()) {
            linearVel = (pos - (prevJoints[i].pos * METERS_PER_CENTIMETER)) / deltaTime;  // m/s
            // quat log imaginary part points along the axis of rotation, with length of one half the angle of rotation.
            glm::quat d = glm::log(rot * glm::inverse(eulerToQuat(prevJoints[i].euler)));
            angularVel = glm::vec3(d.x, d.y, d.z) / (0.5f * deltaTime); // radians/s
        }
        _poseStateMap[poseIndex] = controller::Pose(pos, rot, linearVel, angularVel);

        if (poseIndex == controller::LEFT_HAND ||
            poseIndex == controller::RIGHT_HAND) {
            //qDebug() << __FUNCTION__ << "_poseStateMap[]=" << _poseStateMap[poseIndex].translation;
        }

    }

    _poseStateMap[controller::RIGHT_HAND_THUMB1] = controller::Pose(rightHandThumb1DefaultAbsTranslation, glm::quat(), glm::vec3(), glm::vec3());
    _poseStateMap[controller::LEFT_HAND_THUMB1] = controller::Pose(leftHandThumb1DefaultAbsTranslation, glm::quat(), glm::vec3(), glm::vec3());
}

