//
//  NeuronPlugin.h
//  input-plugins/src/input-plugins
//
//  Created by Anthony Thibault on 12/18/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "NeuronPlugin.h"

#include <QLoggingCategory>
#include <PathUtils.h>

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

const QString NeuronPlugin::NAME = "Neuron";
const QString NeuronPlugin::NEURON_ID_STRING = "Perception Neuron";

bool NeuronPlugin::isSupported() const {
    // TODO:
    return true;
}

void NeuronPlugin::activate() {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::activate";
}

void NeuronPlugin::deactivate() {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::deactivate";
}

void NeuronPlugin::pluginUpdate(float deltaTime, bool jointsCaptured) {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::pluginUpdate";
}

void NeuronPlugin::saveSettings() const {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::saveSettings";
}

void NeuronPlugin::loadSettings() {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::loadSettings";
}

controller::Input::NamedVector NeuronPlugin::InputDevice::getAvailableInputs() const {
    // TODO:
    static const controller::Input::NamedVector availableInputs {
        makePair(controller::LEFT_HAND, "LeftHand"),
        makePair(controller::RIGHT_HAND, "RightHand")
    };
    return availableInputs;
}

QString NeuronPlugin::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/neuron.json";
    return MAPPING_JSON;
}

void NeuronPlugin::InputDevice::update(float deltaTime, bool jointsCaptured) {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::InputDevice::update";
}

void NeuronPlugin::InputDevice::focusOutEvent() {
    // TODO:
    qCDebug(inputplugins) << "NeuronPlugin::InputDevice::focusOutEvent";
}
