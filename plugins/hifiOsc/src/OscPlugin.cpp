//
//  OscPlugin.cpp
//
//  Created by Anthony Thibault on 2019/8/24
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OscPlugin.h"

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

const char* OscPlugin::NAME = "Open Sound Control (OSC)";
const char* OscPlugin::OSC_ID_STRING = "Open Sound Control (OSC)";
const bool DEFAULT_ENABLED = false;

enum class FaceCap {
    BrowsU_C = 0,
    BrowsD_L,
    BrowsD_R,
    BrowsU_L,
    BrowsU_R,
    EyeUp_L,
    EyeUp_R,
    EyeDown_L,
    EyeDown_R,
    EyeIn_L,
    EyeIn_R,
    EyeOut_L,
    EyeOut_R,
    EyeBlink_L,
    EyeBlink_R,
    EyeSquint_L,
    EyeSquint_R,
    EyeOpen_L,
    EyeOpen_R,
    Puff,
    CheekSquint_L,
    CheekSquint_R,
    NoseSneer_L,
    NoseSneer_R,
    JawOpen,
    JawFwd,
    JawLeft,
    JawRight,
    LipsFunnel,
    LipsPucker,
    MouthLeft,
    MouthRight,
    LipsUpperClose,
    LipsLowerClose,
    MouthShrugUpper,
    MouthShrugLower,
    MouthClose,
    MouthSmile_L,
    MouthSmile_R,
    MouthFrown_L,
    MouthFrown_R,
    MouthDimple_L,
    MouthDimple_R,
    MouthUpperUp_L,
    MouthUpperUp_R,
    MouthLowerDown_L,
    MouthLowerDown_R,
    MouthPress_L,
    MouthPress_R,
    LipsStretch_L,
    LipsStretch_R,
    TongueOut,
    BlendshapeCount
};

// used to mirror left/right shapes from FaceCap.
// i.e. right and left shapes are swapped.
FaceCap faceMirrorMap[(int)FaceCap::BlendshapeCount] = {
    FaceCap::BrowsU_C,
    FaceCap::BrowsD_R,
    FaceCap::BrowsD_L,
    FaceCap::BrowsU_R,
    FaceCap::BrowsU_L,
    FaceCap::EyeUp_R,
    FaceCap::EyeUp_L,
    FaceCap::EyeDown_R,
    FaceCap::EyeDown_L,
    FaceCap::EyeIn_R,
    FaceCap::EyeIn_L,
    FaceCap::EyeOut_R,
    FaceCap::EyeOut_L,
    FaceCap::EyeBlink_R,
    FaceCap::EyeBlink_L,
    FaceCap::EyeSquint_R,
    FaceCap::EyeSquint_L,
    FaceCap::EyeOpen_R,
    FaceCap::EyeOpen_L,
    FaceCap::Puff,
    FaceCap::CheekSquint_R,
    FaceCap::CheekSquint_L,
    FaceCap::NoseSneer_R,
    FaceCap::NoseSneer_L,
    FaceCap::JawOpen,
    FaceCap::JawFwd,
    FaceCap::JawRight,
    FaceCap::JawLeft,
    FaceCap::LipsFunnel,
    FaceCap::LipsPucker,
    FaceCap::MouthRight,
    FaceCap::MouthLeft,
    FaceCap::LipsUpperClose,
    FaceCap::LipsLowerClose,
    FaceCap::MouthShrugUpper,
    FaceCap::MouthShrugLower,
    FaceCap::MouthClose,
    FaceCap::MouthSmile_R,
    FaceCap::MouthSmile_L,
    FaceCap::MouthFrown_R,
    FaceCap::MouthFrown_L,
    FaceCap::MouthDimple_R,
    FaceCap::MouthDimple_L,
    FaceCap::MouthUpperUp_R,
    FaceCap::MouthUpperUp_L,
    FaceCap::MouthLowerDown_R,
    FaceCap::MouthLowerDown_L,
    FaceCap::MouthPress_R,
    FaceCap::MouthPress_L,
    FaceCap::LipsStretch_R,
    FaceCap::LipsStretch_L,
    FaceCap::TongueOut
};

static const char* STRINGS[FaceCap::BlendshapeCount] = {
    "BrowsU_C",
    "BrowsD_L",
    "BrowsD_R",
    "BrowsU_L",
    "BrowsU_R",
    "EyeUp_L",
    "EyeUp_R",
    "EyeDown_L",
    "EyeDown_R",
    "EyeIn_L",
    "EyeIn_R",
    "EyeOut_L",
    "EyeOut_R",
    "EyeBlink_L",
    "EyeBlink_R",
    "EyeSquint_L",
    "EyeSquint_R",
    "EyeOpen_L",
    "EyeOpen_R",
    "Puff",
    "CheekSquint_L",
    "CheekSquint_R",
    "NoseSneer_L",
    "NoseSneer_R",
    "JawOpen",
    "JawFwd",
    "JawLeft",
    "JawRight",
    "LipsFunnel",
    "LipsPucker",
    "MouthLeft",
    "MouthRight",
    "LipsUpperClose",
    "LipsLowerClose",
    "MouthShrugUpper",
    "MouthShrugLower",
    "MouthClose",
    "MouthSmile_L",
    "MouthSmile_R",
    "MouthFrown_L",
    "MouthFrown_R",
    "MouthDimple_L",
    "MouthDimple_R",
    "MouthUpperUp_L",
    "MouthUpperUp_R",
    "MouthLowerDown_L",
    "MouthLowerDown_R",
    "MouthPress_L",
    "MouthPress_R",
    "LipsStretch_L",
    "LipsStretch_R",
    "TongueOut"
};

static enum controller::StandardAxisChannel CHANNELS[FaceCap::BlendshapeCount] = {
    controller::BROWSU_C,
    controller::BROWSD_L,
    controller::BROWSD_R,
    controller::BROWSU_L,
    controller::BROWSU_R,
    controller::EYEUP_L,
    controller::EYEUP_R,
    controller::EYEDOWN_L,
    controller::EYEDOWN_R,
    controller::EYEIN_L,
    controller::EYEIN_R,
    controller::EYEOUT_L,
    controller::EYEOUT_R,
    controller::EYEBLINK_L,
    controller::EYEBLINK_R,
    controller::EYESQUINT_L,
    controller::EYESQUINT_R,
    controller::EYEOPEN_L,
    controller::EYEOPEN_R,
    controller::PUFF,
    controller::CHEEKSQUINT_L,
    controller::CHEEKSQUINT_R,
    controller::NOSESNEER_L,
    controller::NOSESNEER_R,
    controller::JAWOPEN,
    controller::JAWFWD,
    controller::JAWLEFT,
    controller::JAWRIGHT,
    controller::LIPSFUNNEL,
    controller::LIPSPUCKER,
    controller::MOUTHLEFT,
    controller::MOUTHRIGHT,
    controller::LIPSUPPERCLOSE,
    controller::LIPSLOWERCLOSE,
    controller::MOUTHSHRUGUPPER,
    controller::MOUTHSHRUGLOWER,
    controller::MOUTHCLOSE,
    controller::MOUTHSMILE_L,
    controller::MOUTHSMILE_R,
    controller::MOUTHFROWN_L,
    controller::MOUTHFROWN_R,
    controller::MOUTHDIMPLE_L,
    controller::MOUTHDIMPLE_R,
    controller::MOUTHUPPERUP_L,
    controller::MOUTHUPPERUP_R,
    controller::MOUTHLOWERDOWN_L,
    controller::MOUTHLOWERDOWN_R,
    controller::MOUTHPRESS_L,
    controller::MOUTHPRESS_R,
    controller::LIPSSTRETCH_L,
    controller::LIPSSTRETCH_R,
    controller::TONGUEOUT
};


void OscPlugin::init() {

    _inputDevice = std::make_shared<InputDevice>();
    _inputDevice->setContainer(this);

    {
        std::lock_guard<std::mutex> guard(_dataMutex);
        _blendshapeValues.assign((int)FaceCap::BlendshapeCount, 0.0f);
        _blendshapeValidFlags.assign((int)FaceCap::BlendshapeCount, false);
        _headRot = glm::quat();
        _headRotValid = false;
        _headTransTarget = extractTranslation(_lastInputCalibrationData.defaultHeadMat);
        _headTransSmoothed = extractTranslation(_lastInputCalibrationData.defaultHeadMat);
        _headTransValid = false;
        _eyeLeftRot = glm::quat();
        _eyeLeftRotValid = false;
        _eyeRightRot = glm::quat();
        _eyeRightRotValid = false;
    }

    loadSettings();

    auto preferences = DependencyManager::get<Preferences>();
    static const QString OSC_PLUGIN { OscPlugin::NAME };
    {
        auto getter = [this]()->bool { return _enabled; };
        auto setter = [this](bool value) {
            _enabled = value;
            saveSettings();
            if (!_enabled) {
                auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
                userInputMapper->withLock([&, this]() {
                    _inputDevice->clearState();
                    restartServer();
                });
            }
        };
        auto preference = new CheckPreference(OSC_PLUGIN, "Enabled", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto debugGetter = [this]()->bool { return _debug; };
        auto debugSetter = [this](bool value) {
            _debug = value;
            saveSettings();
        };
        auto preference = new CheckPreference(OSC_PLUGIN, "Extra Debugging", debugGetter, debugSetter);
        preferences->addPreference(preference);
    }

    {
        static const int MIN_PORT_NUMBER { 0 };
        static const int MAX_PORT_NUMBER { 65535 };

        auto getter = [this]()->float { return (float)_serverPort; };
        auto setter = [this](float value) {
            _serverPort = (int)value;
            saveSettings();
            restartServer();
        };
        auto preference = new SpinnerPreference(OSC_PLUGIN, "Server Port", getter, setter);

        preference->setMin(MIN_PORT_NUMBER);
        preference->setMax(MAX_PORT_NUMBER);
        preference->setStep(1);
        preferences->addPreference(preference);
    }
}

bool OscPlugin::isSupported() const {
    // networking/UDP is pretty much always available...
    return true;
}

static void errorHandlerFunc(int num, const char* msg, const char* path) {
    qDebug(inputplugins) << "OscPlugin: server error" << num << "in path" << path << ":" << msg;
}

static int genericHandlerFunc(const char* path, const char* types, lo_arg** argv,
                              int argc, void* data, void* user_data) {

    OscPlugin* container = reinterpret_cast<OscPlugin*>(user_data);
    assert(container);

    QString key(path);
    std::lock_guard<std::mutex> guard(container->_dataMutex);

    // Special case: decode blendshapes from face-cap iPhone app.
    // http://www.bannaflak.com/face-cap/livemode.html
    if (path[0] == '/' && path[1] == 'W' && argc == 2 && types[0] == 'i' && types[1] == 'f') {
        int index = argv[0]->i;
        if (index >= 0 && index < (int)FaceCap::BlendshapeCount) {
            int mirroredIndex = (int)faceMirrorMap[index];
            container->_blendshapeValues[mirroredIndex] = argv[1]->f;
            container->_blendshapeValidFlags[mirroredIndex] = true;
        }
    }

    // map /HT to head translation
    if (path[0] == '/' && path[1] == 'H' && path[2] == 'T' &&
        types[0] == 'f' && types[1] == 'f' && types[2] == 'f') {
        glm::vec3 trans(-argv[0]->f, -argv[1]->f, argv[2]->f); // in cm

        // convert trans into a delta (in meters) from the sweet spot of the iphone camera.
        const float CM_TO_METERS = 0.01f;
        const glm::vec3 FACE_CAP_HEAD_SWEET_SPOT(0.0f, 0.0f, -45.0f);
        glm::vec3 delta = (trans - FACE_CAP_HEAD_SWEET_SPOT) * CM_TO_METERS;

        container->_headTransTarget = extractTranslation(container->_lastInputCalibrationData.defaultHeadMat) + delta;
        container->_headTransValid = true;
    }

    // map /HR to head rotation
    if (path[0] == '/' && path[1] == 'H' && path[2] == 'R' && path[3] == 0 &&
        types[0] == 'f' && types[1] == 'f' && types[2] == 'f') {
        glm::vec3 euler(-argv[0]->f, -argv[1]->f, argv[2]->f);
        container->_headRot = glm::quat(glm::radians(euler)) * Quaternions::Y_180;
        container->_headRotValid = true;
    }

    // map /ELR to left eye rot
    if (path[0] == '/' && path[1] == 'E' && path[2] == 'L' && path[3] == 'R' &&
        types[0] == 'f' && types[1] == 'f') {
        glm::vec3 euler(argv[0]->f, -argv[1]->f, 0.0f);
        container->_eyeLeftRot = container->_headRot * glm::quat(glm::radians(euler));
        container->_eyeLeftRotValid = true;
    }

    // map /ERR to right eye rot
    if (path[0] == '/' && path[1] == 'E' && path[2] == 'R' && path[3] == 'R' &&
        types[0] == 'f' && types[1] == 'f') {
        glm::vec3 euler((float)argv[0]->f, (float)-argv[1]->f, 0.0f);
        container->_eyeRightRot = container->_headRot * glm::quat(glm::radians(euler));
        container->_eyeRightRotValid = true;
    }

    // AJT: TODO map /STRINGS[i] to _blendshapeValues[i]

    if (container->_debug) {
        for (int i = 0; i < argc; i++) {
            switch (types[i]) {
            case 'i':
                // int32
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] =" << argv[i]->i;
                break;
            case 'f':
                // float32
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] =" << argv[i]->f32;
                break;
            case 's':
                // OSC-string
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <string>";
                break;
            case 'b':
                // OSC-blob
                break;
            case 'h':
                // 64 bit big-endian two's complement integer
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] =" << argv[i]->h;
                break;
            case 't':
                // OSC-timetag
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <OSC-timetag>";
                break;
            case 'd':
                // 64 bit ("double") IEEE 754 floating point number
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] =" << argv[i]->d;
                break;
            case 'S':
                // Alternate type represented as an OSC-string (for example, for systems that differentiate "symbols" from "strings")
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <OSC-symbol>";
                break;
            case 'c':
                // an ascii character, sent as 32 bits
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] =" << argv[i]->c;
                break;
            case 'r':
                // 32 bit RGBA color
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <color>";
                break;
            case 'm':
                // 4 byte MIDI message. Bytes from MSB to LSB are: port id, status byte, data1, data2
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <midi>";
                break;
            case 'T':
                // true
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <true>";
                break;
            case 'F':
                // false
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <false>";
                break;
            case 'N':
                // nil
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <nil>";
                break;
            case 'I':
                // inf
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <inf>";
                break;
            case '[':
                // Indicates the beginning of an array. The tags following are for data in the Array until a close brace tag is reached.
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <begin-array>";
                break;
            case ']':
                // Indicates the end of an array.
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <end-array>";
                break;
            default:
                qDebug(inputplugins) << "OscPlugin: " << path << "[" << i << "] = <unknown-type>" << types[i];
                break;
            }
        }
    }

    return 1;
}


bool OscPlugin::activate() {
    InputPlugin::activate();

    loadSettings();

    if (_enabled) {

        qDebug(inputplugins) << "OscPlugin: activated";

        // register with userInputMapper
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->registerDevice(_inputDevice);

        return startServer();
    }
    return false;
}

void OscPlugin::deactivate() {
    qDebug(inputplugins) << "OscPlugin: deactivated, _oscServerThread =" << _oscServerThread;

    if (_oscServerThread) {
        stopServer();
    }
}

void OscPlugin::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    if (!_enabled) {
        return;
    }

    _lastInputCalibrationData = inputCalibrationData;

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->withLock([&, this]() {
        _inputDevice->update(deltaTime, inputCalibrationData);
    });
}

void OscPlugin::saveSettings() const {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.setValue(QString("enabled"), _enabled);
        settings.setValue(QString("extraDebug"), _debug);
        settings.setValue(QString("serverPort"), _serverPort);
    }
    settings.endGroup();
}

void OscPlugin::loadSettings() {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        _enabled = settings.value("enabled", QVariant(DEFAULT_ENABLED)).toBool();
        _debug = settings.value("extraDebug", QVariant(DEFAULT_ENABLED)).toBool();
        _serverPort = settings.value("serverPort", QVariant(DEFAULT_OSC_SERVER_PORT)).toInt();
    }
    settings.endGroup();
}

bool OscPlugin::startServer() {
    if (_oscServerThread) {
        qWarning(inputplugins) << "OscPlugin: (startServer) server is already running, _oscServerThread =" << _oscServerThread;
        return true;
    }

    // start a new server on specified port
    const size_t BUFFER_SIZE = 64;
    char serverPortString[BUFFER_SIZE];
    snprintf(serverPortString, BUFFER_SIZE, "%d", _serverPort);
    _oscServerThread = lo_server_thread_new(serverPortString, errorHandlerFunc);

    qDebug(inputplugins) << "OscPlugin: server started on port" << serverPortString << ", _oscServerThread =" << _oscServerThread;

    // add method that will match any path and args
    // NOTE: callback function will be called on the OSC thread, not the appliation thread.
    lo_server_thread_add_method(_oscServerThread, NULL, NULL, genericHandlerFunc, (void*)this);

    lo_server_thread_start(_oscServerThread);

    return true;
}

void OscPlugin::stopServer() {
    if (!_oscServerThread) {
        qWarning(inputplugins) << "OscPlugin: (stopServer) server is already shutdown.";
    }

    // stop and free server
    lo_server_thread_stop(_oscServerThread);
    lo_server_thread_free(_oscServerThread);
    _oscServerThread = nullptr;
}

void OscPlugin::restartServer() {
    if (_oscServerThread) {
        stopServer();
    }
    startServer();
}

//
// InputDevice
//

controller::Input::NamedVector OscPlugin::InputDevice::getAvailableInputs() const {
    static controller::Input::NamedVector availableInputs;
    if (availableInputs.size() == 0) {
        for (int i = 0; i < (int)FaceCap::BlendshapeCount; i++) {
            availableInputs.push_back(makePair(CHANNELS[i], STRINGS[i]));
        }
    }
    availableInputs.push_back(makePair(controller::HEAD, "Head"));
    availableInputs.push_back(makePair(controller::LEFT_EYE, "LeftEye"));
    availableInputs.push_back(makePair(controller::RIGHT_EYE, "RightEye"));
    return availableInputs;
}

QString OscPlugin::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/osc.json";
    return MAPPING_JSON;
}

void OscPlugin::InputDevice::update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    glm::mat4 sensorToAvatarMat = glm::inverse(inputCalibrationData.avatarMat) * inputCalibrationData.sensorToWorldMat;
    std::lock_guard<std::mutex> guard(_container->_dataMutex);
    for (int i = 0; i < (int)FaceCap::BlendshapeCount; i++) {
        if (_container->_blendshapeValidFlags[i]) {
            _axisStateMap[CHANNELS[i]] = controller::AxisValue(_container->_blendshapeValues[i], 0, true);
        }
    }
    if (_container->_headRotValid && _container->_headTransValid) {
        const float SMOOTH_TIMESCALE = 2.0f;
        float tau = deltaTime / SMOOTH_TIMESCALE;
        _container->_headTransSmoothed = lerp(_container->_headTransSmoothed, _container->_headTransTarget, tau);
        glm::vec3 delta = _container->_headTransSmoothed - _container->_headTransTarget;
        glm::vec3 trans = extractTranslation(inputCalibrationData.defaultHeadMat) + delta;

        controller::Pose sensorSpacePose(trans, _container->_headRot);
        _poseStateMap[controller::HEAD] = sensorSpacePose.transform(sensorToAvatarMat);
    }
    if (_container->_eyeLeftRotValid) {
        controller::Pose sensorSpacePose(vec3(0.0f), _container->_eyeLeftRot);
        _poseStateMap[controller::LEFT_EYE] = sensorSpacePose.transform(sensorToAvatarMat);
    }
    if (_container->_eyeRightRotValid) {
        controller::Pose sensorSpacePose(vec3(0.0f), _container->_eyeRightRot);
        _poseStateMap[controller::RIGHT_EYE] = sensorSpacePose.transform(sensorToAvatarMat);
    }
}

void OscPlugin::InputDevice::clearState() {
    std::lock_guard<std::mutex> guard(_container->_dataMutex);
    for (int i = 0; i < (int)FaceCap::BlendshapeCount; i++) {
        _axisStateMap[CHANNELS[i]] = controller::AxisValue(0.0f, 0, false);
    }
    _poseStateMap[controller::HEAD] = controller::Pose();
    _poseStateMap[controller::LEFT_EYE] = controller::Pose();
    _poseStateMap[controller::RIGHT_EYE] = controller::Pose();
}

