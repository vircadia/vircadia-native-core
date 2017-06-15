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

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

const char* LeapMotionPlugin::NAME = "Leap Motion";

void LeapMotionPlugin::pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) {
    // TODO
}

controller::Input::NamedVector LeapMotionPlugin::InputDevice::getAvailableInputs() const {
    static controller::Input::NamedVector availableInputs;

    // TODO

    return availableInputs;
}
