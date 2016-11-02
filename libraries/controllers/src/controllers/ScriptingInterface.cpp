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

static QVariantMap createDeviceMap(const controller::InputDevice::Pointer device) {
    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    QVariantMap deviceMap;
    for (const auto& inputMapping : userInputMapper->getAvailableInputs(device->getDeviceID())) {
        const auto& input = inputMapping.first;
        const auto inputName = QString(inputMapping.second).remove(SANITIZE_NAME_EXPRESSION);
#ifdef DEBUG
        qCDebug(controllers) << "\tInput " << input.getChannel() << (int)input.getType()
            << QString::number(input.getID(), 16) << ": " << inputName;
#endif
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
    connect(userInputMapper.data(), &UserInputMapper::hardwareChanged, this, [=] {
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

    float ScriptingInterface::getAxisValue(int source) const {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        return userInputMapper->getValue(Input((uint32_t)source));
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

    QVector<Action> ScriptingInterface::getAllActions() {
        return DependencyManager::get<UserInputMapper>()->getAllActions();
    }

    QString ScriptingInterface::getDeviceName(unsigned int device) {
        return DependencyManager::get<UserInputMapper>()->getDeviceName((unsigned short)device);
    }

    QVector<Input::NamedPair> ScriptingInterface::getAvailableInputs(unsigned int device) {
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

    bool ScriptingInterface::triggerHapticPulse(float strength, float duration, controller::Hand hand) const {
        return DependencyManager::get<UserInputMapper>()->triggerHapticPulse(strength, duration, hand);
    }

    bool ScriptingInterface::triggerShortHapticPulse(float strength, controller::Hand hand) const {
        const float SHORT_HAPTIC_DURATION_MS = 250.0f;
        return DependencyManager::get<UserInputMapper>()->triggerHapticPulse(strength, SHORT_HAPTIC_DURATION_MS, hand);
    }

    bool ScriptingInterface::triggerHapticPulseOnDevice(unsigned int device, float strength, float duration, controller::Hand hand) const {
        return DependencyManager::get<UserInputMapper>()->triggerHapticPulseOnDevice(device, strength, duration, hand);
    }

    bool ScriptingInterface::triggerShortHapticPulseOnDevice(unsigned int device, float strength, controller::Hand hand) const {
        const float SHORT_HAPTIC_DURATION_MS = 250.0f;
        return DependencyManager::get<UserInputMapper>()->triggerHapticPulseOnDevice(device, strength, SHORT_HAPTIC_DURATION_MS, hand);
    }

    void ScriptingInterface::updateMaps() {
        QVariantMap newHardware;
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        const auto& devices = userInputMapper->getDevices();
        for (const auto& deviceMapping : devices) {
            auto deviceID = deviceMapping.first;
            if (deviceID != userInputMapper->getStandardDeviceID()) {
                auto device = deviceMapping.second;
                auto deviceName = QString(device->getName()).remove(SANITIZE_NAME_EXPRESSION);
                qCDebug(controllers) << "Device" << deviceMapping.first << ":" << deviceName;
                if (newHardware.contains(deviceName)) {
                    continue;
                }

                // Expose the IDs to JS
                newHardware.insert(deviceName, createDeviceMap(device));
            }
        }
        _hardware = newHardware;
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


