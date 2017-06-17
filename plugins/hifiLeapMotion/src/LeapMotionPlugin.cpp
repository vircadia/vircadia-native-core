//
//  LeapMotionPlugin.cpp
//
//  Created by David Rowe on 15 Jun 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LeapMotionPlugin.h"

#include <QLoggingCategory>

#include <controllers/UserInputMapper.h>
#include <Preferences.h>
#include <SettingHandle.h>

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

const char* LeapMotionPlugin::NAME = "Leap Motion";
const char* LeapMotionPlugin::LEAPMOTION_ID_STRING = "Leap Motion";

const bool DEFAULT_ENABLED = false;
const char* SENSOR_ON_DESKTOP = "Desktop";
const char* SENSOR_ON_HMD = "HMD";
const char* DEFAULT_SENSOR_LOCATION = SENSOR_ON_DESKTOP;

void LeapMotionPlugin::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    if (!_enabled) {
        return;
    }

    const auto frame = _controller.frame();
    const auto frameID = frame.id();
    if (_lastFrameID >= frameID) {
        // Leap Motion not connected or duplicate frame.
        return;
    }

    if (!_hasLeapMotionBeenConnected) {
        emit deviceConnected(getName());
        _hasLeapMotionBeenConnected = true;
    }

    // TODO

    _lastFrameID = frameID;
}

controller::Input::NamedVector LeapMotionPlugin::InputDevice::getAvailableInputs() const {
    static controller::Input::NamedVector availableInputs;

    // TODO

    return availableInputs;
}

void LeapMotionPlugin::init() {
    loadSettings();

    auto preferences = DependencyManager::get<Preferences>();
    static const QString LEAPMOTION_PLUGIN { "Leap Motion" };
    {
        auto getter = [this]()->bool { return _enabled; };
        auto setter = [this](bool value) {
            _enabled = value;
            saveSettings();
            if (!_enabled) {
                auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
                userInputMapper->withLock([&, this]() {
                    _inputDevice->clearState();
                });
            }
        };
        auto preference = new CheckPreference(LEAPMOTION_PLUGIN, "Enabled", getter, setter);
        preferences->addPreference(preference);
    }
    {
        auto getter = [this]()->QString { return _sensorLocation; };
        auto setter = [this](QString value) {
            _sensorLocation = value;
            saveSettings();
            applySensorLocation();
        };
        auto preference = new ComboBoxPreference(LEAPMOTION_PLUGIN, "Sensor location", getter, setter);
        QStringList list = { SENSOR_ON_DESKTOP, SENSOR_ON_HMD };
        preference->setItems(list);
        preferences->addPreference(preference);
    }
}

bool LeapMotionPlugin::activate() {
    InputPlugin::activate();

    if (_enabled) {
        // Nothing required to be done to start up Leap Motion.
        return true;
    }

    return false;
}

void LeapMotionPlugin::deactivate() {
    InputPlugin::deactivate();
}

const char* SETTINGS_ENABLED_KEY = "enabled";
const char* SETTINGS_SENSOR_LOCATION_KEY = "sensorLocation";

void LeapMotionPlugin::saveSettings() const {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.setValue(QString(SETTINGS_ENABLED_KEY), _enabled);
        settings.setValue(QString(SETTINGS_SENSOR_LOCATION_KEY), _sensorLocation);
    }
    settings.endGroup();
}

void LeapMotionPlugin::loadSettings() {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        _enabled = settings.value(SETTINGS_ENABLED_KEY, QVariant(DEFAULT_ENABLED)).toBool();
        _sensorLocation = settings.value(SETTINGS_SENSOR_LOCATION_KEY, QVariant(DEFAULT_SENSOR_LOCATION)).toString();
        applySensorLocation();
    }
    settings.endGroup();
}

void LeapMotionPlugin::InputDevice::clearState() {
    // TODO
}

void LeapMotionPlugin::applySensorLocation() {
    if (_sensorLocation == SENSOR_ON_HMD) {
        _controller.setPolicyFlags(Leap::Controller::POLICY_OPTIMIZE_HMD);
    } else {
        _controller.setPolicyFlags(Leap::Controller::POLICY_DEFAULT);
    }
}
