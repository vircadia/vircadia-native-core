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

QStringList kinectJointNames = {
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

QStringList controllerJointNames = {
    "Hips",
    "RightUpLeg",
    "RightLeg",
    "RightFoot",
    "LeftUpLeg",
    "LeftLeg",
    "LeftFoot",
    "Spine",
    "Spine1",
    "Spine2",
    "Spine3",
    "Neck",
    "Head",
    "RightShoulder",
    "RightArm",
    "RightForeArm",
    "RightHand",
    "RightHandThumb1",
    "RightHandThumb2",
    "RightHandThumb3",
    "RightHandThumb4",
    "RightHandIndex1",
    "RightHandIndex2",
    "RightHandIndex3",
    "RightHandIndex4",
    "RightHandMiddle1",
    "RightHandMiddle2",
    "RightHandMiddle3",
    "RightHandMiddle4",
    "RightHandRing1",
    "RightHandRing2",
    "RightHandRing3",
    "RightHandRing4",
    "RightHandPinky1",
    "RightHandPinky2",
    "RightHandPinky3",
    "RightHandPinky4",
    "LeftShoulder",
    "LeftArm",
    "LeftForeArm",
    "LeftHand",
    "LeftHandThumb1",
    "LeftHandThumb2",
    "LeftHandThumb3",
    "LeftHandThumb4",
    "LeftHandIndex1",
    "LeftHandIndex2",
    "LeftHandIndex3",
    "LeftHandIndex4",
    "LeftHandMiddle1",
    "LeftHandMiddle2",
    "LeftHandMiddle3",
    "LeftHandMiddle4",
    "LeftHandRing1",
    "LeftHandRing2",
    "LeftHandRing3",
    "LeftHandRing4",
    "LeftHandPinky1",
    "LeftHandPinky2",
    "LeftHandPinky3",
    "LeftHandPinky4"
};

static const char* getControllerJointName(controller::StandardPoseChannel i) {
    if (i >= 0 && i < controller::NUM_STANDARD_POSES) {
        return qPrintable(controllerJointNames[i]);
    }
    return "unknown";
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
                        for (int j = 0; j < jointCount; ++j) {
                            //QString jointName = kinectJointNames[joints[j].JointType];

                            glm::vec3 jointPosition { joints[j].Position.X,
                                                      joints[j].Position.Y,
                                                      joints[j].Position.Z };

                            // filling in the _joints data...
                            if (joints[j].TrackingState != TrackingState_NotTracked) {
                                _joints[j].pos = jointPosition;
                            }
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
    if (_kinectSensor) {
        _kinectSensor->Close();
    }

    SafeRelease(_kinectSensor);
#endif // HAVE_KINECT

}

void KinectPlugin::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {

    if (!_enabled) {
        return;
    }

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
}

//
// InputDevice
//

// FIXME - we probably should only return the inputs we ACTUALLY have
controller::Input::NamedVector KinectPlugin::InputDevice::getAvailableInputs() const {
    static controller::Input::NamedVector availableInputs;
    if (availableInputs.size() == 0) {
        for (int i = 0; i < KinectJointIndex::Size; i++) {
            auto channel = KinectJointIndexToPoseIndex(static_cast<KinectJointIndex>(i));
            availableInputs.push_back(makePair(channel, getControllerJointName(channel)));
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

        // FIXME - left over from neuron, needs to be turned into orientations from kinect SDK
        const glm::vec3& rotEuler = joints[i].euler;

        if (Vectors::ZERO == pos && Vectors::ZERO == rotEuler) {
            _poseStateMap[poseIndex] = controller::Pose();
            continue;
        }


        // FIXME - left over from neuron, needs to be turned into orientations from kinect SDK
        glm::quat rot = eulerToQuat(rotEuler);
        if (i < prevJoints.size()) {
            linearVel = (pos - (prevJoints[i].pos * METERS_PER_CENTIMETER)) / deltaTime;  // m/s
            // quat log imaginary part points along the axis of rotation, with length of one half the angle of rotation.
            glm::quat d = glm::log(rot * glm::inverse(eulerToQuat(prevJoints[i].euler)));
            angularVel = glm::vec3(d.x, d.y, d.z) / (0.5f * deltaTime); // radians/s
        }
        _poseStateMap[poseIndex] = controller::Pose(pos, rot, linearVel, angularVel);
    }

    // FIXME - left over from neuron
    _poseStateMap[controller::RIGHT_HAND_THUMB1] = controller::Pose(rightHandThumb1DefaultAbsTranslation, glm::quat(), glm::vec3(), glm::vec3());
    _poseStateMap[controller::LEFT_HAND_THUMB1] = controller::Pose(leftHandThumb1DefaultAbsTranslation, glm::quat(), glm::vec3(), glm::vec3());
}

