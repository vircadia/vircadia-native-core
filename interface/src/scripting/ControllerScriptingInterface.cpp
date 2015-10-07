//
//  ControllerScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 12/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <avatar/AvatarManager.h>
#include <avatar/MyAvatar.h>
#include <HandData.h>
#include <HFBackEvent.h>

#include "Application.h"
#include "devices/MotionTracker.h"
#include "ControllerScriptingInterface.h"

// TODO: this needs to be removed, as well as any related controller-specific information
#include <input-plugins/SixenseManager.h>


ControllerScriptingInterface::ControllerScriptingInterface() :
    _mouseCaptured(false),
    _touchCaptured(false),
    _wheelCaptured(false),
    _actionsCaptured(false)
{

}

static int actionMetaTypeId = qRegisterMetaType<UserInputMapper::Action>();
static int inputChannelMetaTypeId = qRegisterMetaType<UserInputMapper::InputChannel>();
static int inputMetaTypeId = qRegisterMetaType<UserInputMapper::Input>();
static int inputPairMetaTypeId = qRegisterMetaType<UserInputMapper::InputPair>();

QScriptValue inputToScriptValue(QScriptEngine* engine, const UserInputMapper::Input& input);
void inputFromScriptValue(const QScriptValue& object, UserInputMapper::Input& input);
QScriptValue inputChannelToScriptValue(QScriptEngine* engine, const UserInputMapper::InputChannel& inputChannel);
void inputChannelFromScriptValue(const QScriptValue& object, UserInputMapper::InputChannel& inputChannel);
QScriptValue actionToScriptValue(QScriptEngine* engine, const UserInputMapper::Action& action);
void actionFromScriptValue(const QScriptValue& object, UserInputMapper::Action& action);
QScriptValue inputPairToScriptValue(QScriptEngine* engine, const UserInputMapper::InputPair& inputPair);
void inputPairFromScriptValue(const QScriptValue& object, UserInputMapper::InputPair& inputPair);

QScriptValue inputToScriptValue(QScriptEngine* engine, const UserInputMapper::Input& input) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("device", input.getDevice());
    obj.setProperty("channel", input.getChannel());
    obj.setProperty("type", (unsigned short) input.getType());
    obj.setProperty("id", input.getID());
    return obj;
}

void inputFromScriptValue(const QScriptValue& object, UserInputMapper::Input& input) {
    input.setDevice(object.property("device").toUInt16());
    input.setChannel(object.property("channel").toUInt16());
    input.setType(object.property("type").toUInt16());
    input.setID(object.property("id").toInt32());
}

QScriptValue inputChannelToScriptValue(QScriptEngine* engine, const UserInputMapper::InputChannel& inputChannel) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("input", inputToScriptValue(engine, inputChannel.getInput()));
    obj.setProperty("modifier", inputToScriptValue(engine, inputChannel.getModifier()));
    obj.setProperty("action", inputChannel.getAction());
    obj.setProperty("scale", inputChannel.getScale());
    return obj;
}

void inputChannelFromScriptValue(const QScriptValue& object, UserInputMapper::InputChannel& inputChannel) {
    UserInputMapper::Input input;
    UserInputMapper::Input modifier;
    inputFromScriptValue(object.property("input"), input);
    inputChannel.setInput(input);
    inputFromScriptValue(object.property("modifier"), modifier);
    inputChannel.setModifier(modifier);
    inputChannel.setAction(UserInputMapper::Action(object.property("action").toVariant().toInt()));
    inputChannel.setScale(object.property("scale").toVariant().toFloat());
}

QScriptValue actionToScriptValue(QScriptEngine* engine, const UserInputMapper::Action& action) {
    QScriptValue obj = engine->newObject();
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    QVector<UserInputMapper::InputChannel> inputChannels = userInputMapper->getInputChannelsForAction(action);
    QScriptValue _inputChannels = engine->newArray(inputChannels.size());
    for (int i = 0; i < inputChannels.size(); i++) {
        _inputChannels.setProperty(i, inputChannelToScriptValue(engine, inputChannels[i]));
    }
    obj.setProperty("action", (int) action);
    obj.setProperty("actionName", userInputMapper->getActionName(action));
    obj.setProperty("inputChannels", _inputChannels);
    return obj;
}

void actionFromScriptValue(const QScriptValue& object, UserInputMapper::Action& action) {
    action = UserInputMapper::Action(object.property("action").toVariant().toInt());
}

QScriptValue inputPairToScriptValue(QScriptEngine* engine, const UserInputMapper::InputPair& inputPair) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("input", inputToScriptValue(engine, inputPair.first));
    obj.setProperty("inputName", inputPair.second);
    return obj;
}

void inputPairFromScriptValue(const QScriptValue& object, UserInputMapper::InputPair& inputPair) {
    inputFromScriptValue(object.property("input"), inputPair.first);
    inputPair.second = QString(object.property("inputName").toVariant().toString());
}

void ControllerScriptingInterface::registerControllerTypes(QScriptEngine* engine) {
    qScriptRegisterSequenceMetaType<QVector<UserInputMapper::Action> >(engine);
    qScriptRegisterSequenceMetaType<QVector<UserInputMapper::InputChannel> >(engine);
    qScriptRegisterSequenceMetaType<QVector<UserInputMapper::InputPair> >(engine);
    qScriptRegisterMetaType(engine, actionToScriptValue, actionFromScriptValue);
    qScriptRegisterMetaType(engine, inputChannelToScriptValue, inputChannelFromScriptValue);
    qScriptRegisterMetaType(engine, inputToScriptValue, inputFromScriptValue);
    qScriptRegisterMetaType(engine, inputPairToScriptValue, inputPairFromScriptValue);
}

void ControllerScriptingInterface::handleMetaEvent(HFMetaEvent* event) {
    if (event->type() == HFActionEvent::startType()) {
        emit actionStartEvent(static_cast<HFActionEvent&>(*event));
    } else if (event->type() == HFActionEvent::endType()) {
        emit actionEndEvent(static_cast<HFActionEvent&>(*event));
    } else if (event->type() == HFBackEvent::startType()) {
        emit backStartEvent();
    } else if (event->type() == HFBackEvent::endType()) {
        emit backEndEvent();
    }
}

const PalmData* ControllerScriptingInterface::getPrimaryPalm() const {
    int leftPalmIndex, rightPalmIndex;

    const HandData* handData = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHandData();
    handData->getLeftRightPalmIndices(leftPalmIndex, rightPalmIndex);
    
    if (rightPalmIndex != -1) {
        return &handData->getPalms()[rightPalmIndex];
    }

    return NULL;
}

int ControllerScriptingInterface::getNumberOfActivePalms() const {
    const HandData* handData = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHandData();
    int numberOfPalms = handData->getNumPalms();
    int numberOfActivePalms = 0;
    for (int i = 0; i < numberOfPalms; i++) {
        if (getPalm(i)->isActive()) {
            numberOfActivePalms++;
        }
    }
    return numberOfActivePalms;
}

const PalmData* ControllerScriptingInterface::getPalm(int palmIndex) const {
    const HandData* handData = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHandData();
    return &handData->getPalms()[palmIndex];
}

const PalmData* ControllerScriptingInterface::getActivePalm(int palmIndex) const {
    const HandData* handData = DependencyManager::get<AvatarManager>()->getMyAvatar()->getHandData();
    int numberOfPalms = handData->getNumPalms();
    int numberOfActivePalms = 0;
    for (int i = 0; i < numberOfPalms; i++) {
        if (getPalm(i)->isActive()) {
            if (numberOfActivePalms == palmIndex) {
                return &handData->getPalms()[i];
            }
            numberOfActivePalms++;
        }
    }
    return NULL;
}

bool ControllerScriptingInterface::isPrimaryButtonPressed() const {
    const PalmData* primaryPalm = getPrimaryPalm();
    if (primaryPalm) {
        if (primaryPalm->getControllerButtons() & BUTTON_FWD) {
            return true;
        }
    }
    
    return false;
}

glm::vec2 ControllerScriptingInterface::getPrimaryJoystickPosition() const {
    const PalmData* primaryPalm = getPrimaryPalm();
    if (primaryPalm) {
        return glm::vec2(primaryPalm->getJoystickX(), primaryPalm->getJoystickY());
    }

    return glm::vec2(0);
}

int ControllerScriptingInterface::getNumberOfButtons() const {
    return getNumberOfActivePalms() * NUMBER_OF_BUTTONS_PER_PALM;
}

bool ControllerScriptingInterface::isButtonPressed(int buttonIndex) const {
    int palmIndex = buttonIndex / NUMBER_OF_BUTTONS_PER_PALM;
    int buttonOnPalm = buttonIndex % NUMBER_OF_BUTTONS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (buttonOnPalm) {
            case 0:
                return palmData->getControllerButtons() & BUTTON_0;
            case 1:
                return palmData->getControllerButtons() & BUTTON_1;
            case 2:
                return palmData->getControllerButtons() & BUTTON_2;
            case 3:
                return palmData->getControllerButtons() & BUTTON_3;
            case 4:
                return palmData->getControllerButtons() & BUTTON_4;
            case 5:
                return palmData->getControllerButtons() & BUTTON_FWD;
        }
    }
    return false;
}

int ControllerScriptingInterface::getNumberOfTriggers() const {
    return getNumberOfActivePalms() * NUMBER_OF_TRIGGERS_PER_PALM;
}

float ControllerScriptingInterface::getTriggerValue(int triggerIndex) const {
    // we know there's one trigger per palm, so the triggerIndex is the palm Index
    int palmIndex = triggerIndex;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        return palmData->getTrigger();
    }
    return 0.0f;
}

int ControllerScriptingInterface::getNumberOfJoysticks() const {
    return getNumberOfActivePalms() * NUMBER_OF_JOYSTICKS_PER_PALM;
}

glm::vec2 ControllerScriptingInterface::getJoystickPosition(int joystickIndex) const {
    // we know there's one joystick per palm, so the joystickIndex is the palm Index
    int palmIndex = joystickIndex;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        return glm::vec2(palmData->getJoystickX(), palmData->getJoystickY());
    }
    return glm::vec2(0);
}

int ControllerScriptingInterface::getNumberOfSpatialControls() const {
    return getNumberOfActivePalms() * NUMBER_OF_SPATIALCONTROLS_PER_PALM;
}

glm::vec3 ControllerScriptingInterface::getSpatialControlPosition(int controlIndex) const {
    int palmIndex = controlIndex / NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    int controlOfPalm = controlIndex % NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (controlOfPalm) {
            case PALM_SPATIALCONTROL:
                return palmData->getPosition();
            case TIP_SPATIALCONTROL:
                return palmData->getTipPosition();
        }
    }
    return glm::vec3(0); // bad index
}

glm::vec3 ControllerScriptingInterface::getSpatialControlVelocity(int controlIndex) const {
    int palmIndex = controlIndex / NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    int controlOfPalm = controlIndex % NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (controlOfPalm) {
            case PALM_SPATIALCONTROL:
                return palmData->getVelocity();
            case TIP_SPATIALCONTROL:
                return palmData->getTipVelocity();
        }
    }
    return glm::vec3(0); // bad index
}

glm::quat ControllerScriptingInterface::getSpatialControlRawRotation(int controlIndex) const {
    int palmIndex = controlIndex / NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    int controlOfPalm = controlIndex % NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (controlOfPalm) {
            case PALM_SPATIALCONTROL:
                return palmData->getRawRotation();
            case TIP_SPATIALCONTROL:
                return palmData->getRawRotation(); // currently the tip doesn't have a unique rotation, use the palm rotation
        }
    }
    return glm::quat(); // bad index
}
    
glm::vec3 ControllerScriptingInterface::getSpatialControlRawAngularVelocity(int controlIndex) const {
    int palmIndex = controlIndex / NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    int controlOfPalm = controlIndex % NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (controlOfPalm) {
            case PALM_SPATIALCONTROL:
                return palmData->getRawAngularVelocity();
            case TIP_SPATIALCONTROL:
                return palmData->getRawAngularVelocity();  //  Tip = palm angular velocity        
        }
    }
    return glm::vec3(0); // bad index
}

glm::vec3 ControllerScriptingInterface::getSpatialControlNormal(int controlIndex) const {
    int palmIndex = controlIndex / NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    int controlOfPalm = controlIndex % NUMBER_OF_SPATIALCONTROLS_PER_PALM;
    const PalmData* palmData = getActivePalm(palmIndex);
    if (palmData) {
        switch (controlOfPalm) {
            case PALM_SPATIALCONTROL:
                return palmData->getNormal();
            case TIP_SPATIALCONTROL:
                return palmData->getFingerDirection();
        }
    }
    return glm::vec3(0); // bad index
}

bool ControllerScriptingInterface::isKeyCaptured(QKeyEvent* event) const {
    return isKeyCaptured(KeyEvent(*event));
}

bool ControllerScriptingInterface::isKeyCaptured(const KeyEvent& event) const {
    // if we've captured some combination of this key it will be in the map
    return _capturedKeys.contains(event.key, event);
}

void ControllerScriptingInterface::captureKeyEvents(const KeyEvent& event) {
    // if it's valid
    if (event.isValid) {
        // and not already captured
        if (!isKeyCaptured(event)) {
            // then add this KeyEvent record to the captured combos for this key
            _capturedKeys.insert(event.key, event);
        }
    }
}

void ControllerScriptingInterface::releaseKeyEvents(const KeyEvent& event) {
    if (event.isValid) {
        // and not already captured
        if (isKeyCaptured(event)) {
            _capturedKeys.remove(event.key, event);
        }
    }
}

bool ControllerScriptingInterface::isJoystickCaptured(int joystickIndex) const {
    return _capturedJoysticks.contains(joystickIndex);
}

void ControllerScriptingInterface::captureJoystick(int joystickIndex) {
    if (!isJoystickCaptured(joystickIndex)) {
        _capturedJoysticks.insert(joystickIndex);
    }
}

void ControllerScriptingInterface::releaseJoystick(int joystickIndex) {
    if (isJoystickCaptured(joystickIndex)) {
        _capturedJoysticks.remove(joystickIndex);
    }
}

glm::vec2 ControllerScriptingInterface::getViewportDimensions() const {
    return qApp->getUiSize();
}

AbstractInputController* ControllerScriptingInterface::createInputController(const QString& deviceName, const QString& tracker) {
    // This is where we retreive the Device Tracker category and then the sub tracker within it
    //TODO C++11 auto icIt = _inputControllers.find(0);
    InputControllerMap::iterator icIt = _inputControllers.find(0);

    if (icIt != _inputControllers.end()) {
        return (*icIt).second;
    } else {

        // Look for device
        DeviceTracker::ID deviceID = DeviceTracker::getDeviceID(deviceName.toStdString());
        if (deviceID < 0) {
            deviceID = 0;
        }
        // TODO in this current implementation, we just pick the device assuming there is one (normally the Leapmotion)
        // in the near future we need to change that to a real mapping between the devices and the deviceName
        // ALso we need to expand the spec so we can fall back on  the "default" controller per categories

        if (deviceID >= 0) {
            // TODO here again the assumption it's the LeapMotion and so it's a MOtionTracker, this would need to be changed to support different types of devices
            MotionTracker* motionTracker = dynamic_cast< MotionTracker* > (DeviceTracker::getDevice(deviceID));
            if (motionTracker) {
                MotionTracker::Index trackerID = motionTracker->findJointIndex(tracker.toStdString());
                if (trackerID >= 0) {
                    AbstractInputController* inputController = new InputController(deviceID, trackerID, this);

                    _inputControllers.insert(InputControllerMap::value_type(inputController->getKey(), inputController));

                    return inputController;
                }
            }
        }

        return 0;
    }
}

void ControllerScriptingInterface::releaseInputController(AbstractInputController* input) {
    _inputControllers.erase(input->getKey());
}

void ControllerScriptingInterface::updateInputControllers() {
    //TODO C++11 for (auto it = _inputControllers.begin(); it != _inputControllers.end(); it++) {
    for (InputControllerMap::iterator it = _inputControllers.begin(); it != _inputControllers.end(); it++) {
        (*it).second->update();
    }
}

QVector<UserInputMapper::Action> ControllerScriptingInterface::getAllActions() {
    return DependencyManager::get<UserInputMapper>()->getAllActions();
}

QVector<UserInputMapper::InputChannel> ControllerScriptingInterface::getInputChannelsForAction(UserInputMapper::Action action) {
    return DependencyManager::get<UserInputMapper>()->getInputChannelsForAction(action);
}

QString ControllerScriptingInterface::getDeviceName(unsigned int device) {
    return DependencyManager::get<UserInputMapper>()->getDeviceName((unsigned short)device);
}

QVector<UserInputMapper::InputChannel> ControllerScriptingInterface::getAllInputsForDevice(unsigned int device) {
    return DependencyManager::get<UserInputMapper>()->getAllInputsForDevice(device);
}

bool ControllerScriptingInterface::addInputChannel(UserInputMapper::InputChannel inputChannel) {
    return DependencyManager::get<UserInputMapper>()->addInputChannel(inputChannel._action, inputChannel._input, inputChannel._modifier, inputChannel._scale);
}

bool ControllerScriptingInterface::removeInputChannel(UserInputMapper::InputChannel inputChannel) {
    return DependencyManager::get<UserInputMapper>()->removeInputChannel(inputChannel);
}

QVector<UserInputMapper::InputPair> ControllerScriptingInterface::getAvailableInputs(unsigned int device) {
    return DependencyManager::get<UserInputMapper>()->getAvailableInputs((unsigned short)device);
}

void ControllerScriptingInterface::resetAllDeviceBindings() {
    DependencyManager::get<UserInputMapper>()->resetAllDeviceBindings();
}

void ControllerScriptingInterface::resetDevice(unsigned int device) {
    DependencyManager::get<UserInputMapper>()->resetDevice(device);
}

int ControllerScriptingInterface::findDevice(QString name) {
    return DependencyManager::get<UserInputMapper>()->findDevice(name);
}

QVector<QString> ControllerScriptingInterface::getDeviceNames() {
    return DependencyManager::get<UserInputMapper>()->getDeviceNames();
}

float ControllerScriptingInterface::getActionValue(int action) {
    return DependencyManager::get<UserInputMapper>()->getActionState(UserInputMapper::Action(action));
}

int ControllerScriptingInterface::findAction(QString actionName) {
    return DependencyManager::get<UserInputMapper>()->findAction(actionName);
}

QVector<QString> ControllerScriptingInterface::getActionNames() const {
    return DependencyManager::get<UserInputMapper>()->getActionNames();
}

InputController::InputController(int deviceTrackerId, int subTrackerId, QObject* parent) :
    AbstractInputController(),
    _deviceTrackerId(deviceTrackerId),
    _subTrackerId(subTrackerId),
    _isActive(false)
{
}

void InputController::update() {
    _isActive = false;

    // TODO for now the InputController is only supporting a JointTracker from a MotionTracker
    MotionTracker* motionTracker = dynamic_cast< MotionTracker*> (DeviceTracker::getDevice(_deviceTrackerId));
    if (motionTracker) {
        if ((int)_subTrackerId < motionTracker->numJointTrackers()) {
            const MotionTracker::JointTracker* joint = motionTracker->getJointTracker(_subTrackerId);

            if (joint->isActive()) {
                joint->getAbsFrame().getTranslation(_eventCache.absTranslation);
                joint->getAbsFrame().getRotation(_eventCache.absRotation);
                joint->getLocFrame().getTranslation(_eventCache.locTranslation);
                joint->getLocFrame().getRotation(_eventCache.locRotation);

                _isActive = true;
                emit spatialEvent(_eventCache);
            }
        }
    }
}

const unsigned int INPUTCONTROLLER_KEY_DEVICE_OFFSET = 16;
const unsigned int INPUTCONTROLLER_KEY_DEVICE_MASK = 16;

InputController::Key InputController::getKey() const {
    return (((_deviceTrackerId & INPUTCONTROLLER_KEY_DEVICE_MASK) << INPUTCONTROLLER_KEY_DEVICE_OFFSET) | _subTrackerId);
}
