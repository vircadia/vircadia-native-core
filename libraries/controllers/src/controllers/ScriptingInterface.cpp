//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ScriptingInterface.h"

#include <mutex>
#include <set>

#include <QtCore/QRegularExpression>

#include <QEventLoop>
#include <QThread>

#include <GLMHelpers.h>
#include <DependencyManager.h>

#include <ResourceManager.h>

#include "impl/MappingBuilderProxy.h"
#include "Logging.h"
#include "InputDevice.h"


static QRegularExpression SANITIZE_NAME_EXPRESSION{ "[\\(\\)\\.\\s]" };

static QVariantMap createDeviceMap(const controller::DeviceProxy::Pointer device) {
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    QVariantMap deviceMap;
    for (const auto& inputMapping : device->getAvailabeInputs()) {
        const auto& input = inputMapping.first;
        const auto inputName = QString(inputMapping.second).remove(SANITIZE_NAME_EXPRESSION);
        qCDebug(controllers) << "\tInput " << input.getChannel() << (int)input.getType()
            << QString::number(input.getID(), 16) << ": " << inputName;
        deviceMap.insert(inputName, input.getID());
    }
    return deviceMap;
}

// FIXME this throws a hissy fit on MSVC if I put it in the main controller namespace block
controller::ScriptingInterface::ScriptingInterface() {
    auto userInputMapper = DependencyManager::get<UserInputMapper>();

    connect(userInputMapper.data(), &UserInputMapper::actionEvent, this, &controller::ScriptingInterface::actionEvent);
    connect(userInputMapper.data(), &UserInputMapper::inputEvent, this, &controller::ScriptingInterface::inputEvent);

    // FIXME make this thread safe
    connect(userInputMapper.data(), &UserInputMapper::hardwareChanged, [=] {
        updateMaps();
        emit hardwareChanged();
    });


    qCDebug(controllers) << "Setting up standard controller abstraction";
    _standard = createDeviceMap(userInputMapper->getStandardDevice());

    // FIXME allow custom user actions?
    auto actionNames = userInputMapper->getActionNames();
    qCDebug(controllers) << "Setting up standard actions";
    for (const auto& namedInput : userInputMapper->getActionInputs()) {
        const QString& actionName = namedInput.second;
        const Input& actionInput = namedInput.first;
        qCDebug(controllers) << "\tAction: " << actionName << " " << actionInput.getChannel();

        // Expose the IDs to JS
        QString cleanActionName = QString(actionName).remove(SANITIZE_NAME_EXPRESSION);
        _actions.insert(cleanActionName, actionInput.getID());
    }

    updateMaps();
}

namespace controller {

    QObject* ScriptingInterface::newMapping(const QString& mappingName) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        return new MappingBuilderProxy(*userInputMapper, userInputMapper->newMapping(mappingName));
    }

    void ScriptingInterface::enableMapping(const QString& mappingName, bool enable) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->enableMapping(mappingName, enable);
    }

    float ScriptingInterface::getValue(const int& source) const {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        return userInputMapper->getValue(Input((uint32_t)source));
    }

    float ScriptingInterface::getButtonValue(StandardButtonChannel source, uint16_t device) const {
        return getValue(Input(device, source, ChannelType::BUTTON).getID());
    }

    float ScriptingInterface::getAxisValue(StandardAxisChannel source, uint16_t device) const {
        return getValue(Input(device, source, ChannelType::AXIS).getID());
    }

    Pose ScriptingInterface::getPoseValue(const int& source) const {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        return userInputMapper->getPose(Input((uint32_t)source)); 
    }
    
    Pose ScriptingInterface::getPoseValue(StandardPoseChannel source, uint16_t device) const {
        return getPoseValue(Input(device, source, ChannelType::POSE).getID());
    }

    //bool ScriptingInterface::isPrimaryButtonPressed() const {
    //    return isButtonPressed(StandardButtonChannel::A);
    //}
    //    
    //glm::vec2 ScriptingInterface::getPrimaryJoystickPosition() const {
    //    return getJoystickPosition(0);
    //}

    //int ScriptingInterface::getNumberOfButtons() const {
    //    return StandardButtonChannel::NUM_STANDARD_BUTTONS;
    //}

    //bool ScriptingInterface::isButtonPressed(int buttonIndex) const {
    //    return getButtonValue((StandardButtonChannel)buttonIndex) == 0.0 ? false : true;
    //}

    //int ScriptingInterface::getNumberOfTriggers() const {
    //    return StandardCounts::TRIGGERS;
    //}

    //float ScriptingInterface::getTriggerValue(int triggerIndex) const {
    //    return getAxisValue(triggerIndex == 0 ? StandardAxisChannel::LT : StandardAxisChannel::RT);
    //}

    //int ScriptingInterface::getNumberOfJoysticks() const {
    //    return StandardCounts::ANALOG_STICKS;
    //}

    //glm::vec2 ScriptingInterface::getJoystickPosition(int joystickIndex) const {
    //    StandardAxisChannel xid = StandardAxisChannel::LX; 
    //    StandardAxisChannel yid = StandardAxisChannel::LY;
    //    if (joystickIndex != 0) {
    //        xid = StandardAxisChannel::RX;
    //        yid = StandardAxisChannel::RY;
    //    }
    //    vec2 result;
    //    result.x = getAxisValue(xid);
    //    result.y = getAxisValue(yid);
    //    return result;
    //}

    //int ScriptingInterface::getNumberOfSpatialControls() const {
    //    return StandardCounts::POSES;
    //}

    //glm::vec3 ScriptingInterface::getSpatialControlPosition(int controlIndex) const {
    //    // FIXME extract the position from the standard pose
    //    return vec3();
    //}

    //glm::vec3 ScriptingInterface::getSpatialControlVelocity(int controlIndex) const {
    //    // FIXME extract the velocity from the standard pose
    //    return vec3();
    //}

    //glm::vec3 ScriptingInterface::getSpatialControlNormal(int controlIndex) const {
    //    // FIXME extract the normal from the standard pose
    //    return vec3();
    //}
    //
    //glm::quat ScriptingInterface::getSpatialControlRawRotation(int controlIndex) const {
    //    // FIXME extract the rotation from the standard pose
    //    return quat();
    //}

    QVector<Action> ScriptingInterface::getAllActions() {
        return DependencyManager::get<UserInputMapper>()->getAllActions();
    }

    QString ScriptingInterface::getDeviceName(unsigned int device) {
        return DependencyManager::get<UserInputMapper>()->getDeviceName((unsigned short)device);
    }

    QVector<InputPair> ScriptingInterface::getAvailableInputs(unsigned int device) {
        return DependencyManager::get<UserInputMapper>()->getAvailableInputs((unsigned short)device);
    }

    int ScriptingInterface::findDevice(QString name) {
        return DependencyManager::get<UserInputMapper>()->findDevice(name);
    }

    QVector<QString> ScriptingInterface::getDeviceNames() {
        return DependencyManager::get<UserInputMapper>()->getDeviceNames();
    }

    float ScriptingInterface::getActionValue(int action) {
        return DependencyManager::get<UserInputMapper>()->getActionState(Action(action));
    }

    int ScriptingInterface::findAction(QString actionName) {
        return DependencyManager::get<UserInputMapper>()->findAction(actionName);
    }

    QVector<QString> ScriptingInterface::getActionNames() const {
        return DependencyManager::get<UserInputMapper>()->getActionNames();
    }

    void ScriptingInterface::updateMaps() {
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        auto devices = userInputMapper->getDevices();
        QSet<QString> foundDevices;
        for (const auto& deviceMapping : devices) {
            auto deviceID = deviceMapping.first;
            if (deviceID != userInputMapper->getStandardDeviceID()) {
                auto device = deviceMapping.second;
                auto deviceName = QString(device->getName()).remove(SANITIZE_NAME_EXPRESSION);
                qCDebug(controllers) << "Device" << deviceMapping.first << ":" << deviceName;
                foundDevices.insert(device->getName());
                if (_hardware.contains(deviceName)) {
                    continue;
                }

                // Expose the IDs to JS
                _hardware.insert(deviceName, createDeviceMap(device));
            }

        }
    }


    QObject* ScriptingInterface::parseMapping(const QString& json) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        auto mapping = userInputMapper->parseMapping(json);
        return new MappingBuilderProxy(*userInputMapper, mapping);
    }

    QObject* ScriptingInterface::loadMapping(const QString& jsonUrl) {
        return nullptr;
    }


} // namespace controllers


